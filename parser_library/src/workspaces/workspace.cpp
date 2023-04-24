/*
 * Copyright (c) 2019 Broadcom.
 * The term "Broadcom" refers to Broadcom Inc. and/or its subsidiaries.
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 *
 * Contributors:
 *   Broadcom, Inc. - initial API and implementation
 */

#include "workspace.h"

#include <algorithm>
#include <cassert>
#include <map>
#include <memory>
#include <unordered_set>

#include "analyzer.h"
#include "context/instruction.h"
#include "file.h"
#include "file_manager.h"
#include "lsp/document_symbol_item.h"
#include "lsp/item_convertors.h"
#include "lsp/lsp_context.h"
#include "macro_cache.h"
#include "processing/statement_analyzers/hit_count_analyzer.h"
#include "semantics/highlighting_info.h"
#include "utils/bk_tree.h"
#include "utils/factory.h"
#include "utils/levenshtein_distance.h"
#include "utils/path.h"
#include "utils/transform_inserter.h"

using hlasm_plugin::utils::resource::resource_location;
using hlasm_plugin::utils::resource::resource_location_hasher;

namespace hlasm_plugin::parser_library::workspaces {

struct parsing_results
{
    semantics::lines_info hl_info;
    std::shared_ptr<lsp::lsp_context> lsp_context;
    std::shared_ptr<const std::vector<fade_message_s>> fade_messages;
    performance_metrics metrics;
    std::vector<std::pair<virtual_file_handle, utils::resource::resource_location>> vf_handles;
    processing::hit_count_map hc_opencode_map;
    processing::hit_count_map hc_macro_map;

    std::vector<diagnostic_s> opencode_diagnostics;
    std::vector<diagnostic_s> macro_diagnostics;
};

[[nodiscard]] utils::value_task<parsing_results> parse_one_file(std::shared_ptr<context::id_storage> ids,
    std::shared_ptr<file> file,
    parse_lib_provider& lib_provider,
    asm_option asm_opts,
    std::vector<preprocessor_options> pp,
    virtual_file_monitor* vfm)
{
    auto fms = std::make_shared<std::vector<fade_message_s>>();
    analyzer a(file->get_text(),
        analyzer_options {
            file->get_location(),
            &lib_provider,
            std::move(asm_opts),
            collect_highlighting_info::yes,
            file_is_opencode::yes,
            std::move(ids),
            std::move(pp),
            vfm,
            fms,
        });

    processing::hit_count_analyzer hc_analyzer(a.hlasm_ctx());
    a.register_stmt_analyzer(&hc_analyzer);

    co_await a.co_analyze();

    a.collect_diags();

    parsing_results result;
    result.opencode_diagnostics = std::move(a.diags());
    result.hl_info = a.take_semantic_tokens();
    result.lsp_context = a.context().lsp_ctx;
    result.fade_messages = std::move(fms);
    result.metrics = a.get_metrics();
    result.vf_handles = a.take_vf_handles();
    result.hc_opencode_map = hc_analyzer.take_hit_count_map();

    co_return result;
}

struct workspace_parse_lib_provider final : public parse_lib_provider
{
    workspace& ws;
    std::vector<std::shared_ptr<library>> libraries;
    workspace::processor_file_compoments& pfc;

    std::map<resource_location,
        std::variant<std::shared_ptr<workspace::dependency_cache>, virtual_file_handle>,
        std::less<>>
        next_dependencies;
    std::map<std::string, resource_location, std::less<>> next_member_map;
    std::unordered_map<resource_location, std::shared_ptr<file>, resource_location_hasher, std::equal_to<>>
        current_file_map;

    workspace_parse_lib_provider(workspace& ws, workspace::processor_file_compoments& pfc)
        : ws(ws)
        , libraries(ws.get_proc_grp(pfc.m_file->get_location()).libraries())
        , pfc(pfc)
    {}

    void append_files_to_close(std::set<resource_location>& files_to_close)
    {
        std::set_difference(pfc.m_dependencies.begin(),
            pfc.m_dependencies.end(),
            next_dependencies.begin(),
            next_dependencies.end(),
            utils::transform_inserter(
                files_to_close, [](const auto& v) -> const auto& { return v.first; }),
            [](const auto& l, const auto& r) { return l.first < r.first; });
    }

    resource_location get_url(std::string_view library)
    {
        if (auto it = next_member_map.find(library); it != next_member_map.end())
        {
            return it->second;
        }
        else if (resource_location url; std::none_of(libraries.begin(),
                     libraries.end(),
                     [&url, &library](const auto& lib) { return lib->has_file(library, &url); }))
        {
            next_member_map.emplace(library, resource_location());
            return resource_location();
        }
        else
        {
            next_member_map.emplace(library, url);
            return url;
        }
    }

    [[nodiscard]] utils::value_task<std::shared_ptr<file>> get_file(const resource_location& url)
    {
        if (auto it = current_file_map.find(url); it != current_file_map.end())
            co_return it->second;
        else
            co_return current_file_map.try_emplace(url, co_await ws.file_manager_.add_file(url)).first->second;
    }

    auto& get_cache(const resource_location& url, const std::shared_ptr<file>& file)
    {
        return std::get<std::shared_ptr<workspace::dependency_cache>>(
            next_dependencies
                .try_emplace(url, utils::factory([&url, &file, this]() {
                    auto version = file->get_version();
                    if (auto it = pfc.m_dependencies.find(url); it != pfc.m_dependencies.end()
                        && std::get<std::shared_ptr<workspace::dependency_cache>>(it->second)->version == version)
                        return std::get<std::shared_ptr<workspace::dependency_cache>>(it->second);

                    return std::make_shared<workspace::dependency_cache>(version, ws.get_file_manager(), file);
                }))
                .first->second)
            ->cache;
    }

    // Inherited via parse_lib_provider
    [[nodiscard]] utils::value_task<bool> parse_library(
        std::string library, analyzing_context ctx, library_data data) override
    {
        resource_location url = get_url(library);
        if (url.empty())
            co_return false;

        std::shared_ptr<file> file = co_await get_file(url);
        // TODO: if file is in error do something?

        auto& macro_pfc = co_await ws.add_processor_file_impl(file);

        auto cache_key = macro_cache_key::create_from_context(*ctx.hlasm_ctx, data);

        auto& mc = get_cache(url, file);

        if (mc.load_from_cache(cache_key, ctx))
            co_return true;

        const bool collect_hl = file->get_lsp_editing() || macro_pfc.m_last_opencode_analyzer_with_lsp
            || macro_pfc.m_last_macro_analyzer_with_lsp || ctx.hlasm_ctx->processing_stack().parent().empty();
        analyzer a(file->get_text(),
            analyzer_options {
                std::move(url),
                this,
                std::move(ctx),
                data,
                collect_hl ? collect_highlighting_info::yes : collect_highlighting_info::no,
            });

        processing::hit_count_analyzer hc_analyzer(a.hlasm_ctx());
        a.register_stmt_analyzer(&hc_analyzer);

        co_await a.co_analyze();
        a.collect_diags();

        macro_pfc.m_last_results->macro_diagnostics = std::move(a.diags());

        mc.save_macro(cache_key, a);
        macro_pfc.m_last_macro_analyzer_with_lsp = collect_hl;
        if (collect_hl)
            macro_pfc.m_last_results->hl_info = a.take_semantic_tokens();

        macro_pfc.m_last_results->hc_macro_map = hc_analyzer.take_hit_count_map();

        co_return true;
    }

    bool has_library(std::string_view library, resource_location* loc) override
    {
        auto url = get_url(library);
        bool result = !url.empty();
        if (loc)
            *loc = std::move(url);
        return result;
    }

    [[nodiscard]] utils::value_task<std::optional<std::pair<std::string, utils::resource::resource_location>>>
    get_library(std::string library) override
    {
        if (auto url = get_url(library); url.empty())
            co_return std::nullopt;
        else
            co_return std::make_pair((co_await get_file(url))->get_text(), std::move(url));
    }

    [[nodiscard]] utils::task prefetch_libraries() const
    {
        std::vector<utils::task> pending_prefetches;
        for (const auto& lib : libraries)
            if (auto p = lib->prefetch(); p.valid() && !p.done())
                pending_prefetches.emplace_back(std::move(p));

        if (pending_prefetches.empty())
            return {};

        return utils::task::wait_all(std::move(pending_prefetches));
    }
};

workspace::workspace(const resource_location& location,
    const std::string& name,
    file_manager& file_manager,
    const lib_config& global_config,
    const shared_json& global_settings)
    : name_(name)
    , location_(location.lexically_normal())
    , file_manager_(file_manager)
    , fm_vfm_(file_manager_, location)
    , implicit_proc_grp("pg_implicit", {}, {})
    , global_config_(global_config)
    , m_configuration(file_manager, location_, global_settings)
{}

workspace::workspace(const resource_location& location,
    file_manager& file_manager,
    const lib_config& global_config,
    const shared_json& global_settings)
    : workspace(location, location.get_uri(), file_manager, global_config, global_settings)
{}

workspace::workspace(file_manager& file_manager,
    const lib_config& global_config,
    const shared_json& global_settings,
    std::shared_ptr<library> implicit_library)
    : workspace(resource_location(""), file_manager, global_config, global_settings)
{
    opened_ = true;
    if (implicit_library)
        implicit_proc_grp.add_library(std::move(implicit_library));
}

workspace::~workspace() = default;

void workspace::collect_diags() const
{
    std::unordered_set<resource_location, resource_location_hasher> used_b4g_configs;

    for (const auto& [_, component] : m_processor_files)
        if (component.m_opened)
            used_b4g_configs.emplace(component.m_alternative_config);

    m_configuration.copy_diagnostics(*this, used_b4g_configs);

    for (const auto& [url, pfc] : m_processor_files)
    {
        if (is_dependency(url))
            diags().insert(diags().end(),
                pfc.m_last_results->macro_diagnostics.begin(),
                pfc.m_last_results->macro_diagnostics.end());
        else
            diags().insert(diags().end(),
                pfc.m_last_results->opencode_diagnostics.begin(),
                pfc.m_last_results->opencode_diagnostics.end());
    }
}

namespace {
struct mac_cpybook_definition_details
{
    bool cpy_book = false;
    size_t end_line;
    size_t prototype_line = 0;
};

using mac_cpy_definitions_map = std::map<size_t, mac_cpybook_definition_details, std::greater<size_t>>;
using rl_mac_cpy_map = std::unordered_map<resource_location, mac_cpy_definitions_map, resource_location_hasher>;

void generate_merged_fade_messages(const resource_location& rl,
    const processing::hit_count_entry& hc_entry,
    const rl_mac_cpy_map& active_rl_mac_cpy_map,
    std::vector<fade_message_s>& fms)
{
    const auto& [has_sections, line_hits, encountered_macro_defs] = hc_entry;
    if (!has_sections)
        return;

    const mac_cpy_definitions_map* active_mac_cpy_defs_map = nullptr;
    if (const auto active_rl_mac_cpy_map_it = active_rl_mac_cpy_map.find(rl);
        active_rl_mac_cpy_map_it != active_rl_mac_cpy_map.end())
        active_mac_cpy_defs_map = &active_rl_mac_cpy_map_it->second;

    const auto line_details_it_b = line_hits.line_details.begin();
    const auto line_details_it_e = std::next(line_details_it_b, line_hits.max_lineno + 1);

    const auto faded_line_predicate = [&active_mac_cpy_defs_map,
                                          line_details_addr = std::to_address(line_details_it_b),
                                          &encountered_mac_defs = encountered_macro_defs](
                                          const processing::line_detail& e) {
        if (e.macro_definition)
        {
            if (!active_mac_cpy_defs_map)
                return false;

            auto lineno = static_cast<size_t>(std::distance(line_details_addr, &e));
            auto active_mac_cpy_it_e = active_mac_cpy_defs_map->end();

            auto active_mac_cpy_it = std::find_if(active_mac_cpy_defs_map->lower_bound(lineno),
                active_mac_cpy_it_e,
                [lineno](const std::pair<size_t, mac_cpybook_definition_details>& mac_cpy_def) {
                    const auto& [active_mac_cpy_start_line, active_mac_cpy_def_detail] = mac_cpy_def;
                    return lineno >= active_mac_cpy_start_line && lineno <= active_mac_cpy_def_detail.end_line;
                });

            if (active_mac_cpy_it == active_mac_cpy_it_e
                || (!active_mac_cpy_it->second.cpy_book && !encountered_mac_defs.contains(active_mac_cpy_it->first)))
                return false;
        }

        return e.contains_statement && e.count == 0;
    };

    const auto& uri = rl.get_uri();

    const auto it_b = std::find_if(
        line_details_it_b, line_details_it_e, [](const processing::line_detail& e) { return e.contains_statement; });
    auto faded_line_it = std::find_if(it_b, line_details_it_e, faded_line_predicate);

    while (faded_line_it != line_details_it_e)
    {
        auto active_line = std::find_if_not(std::next(faded_line_it), line_details_it_e, faded_line_predicate);
        fms.emplace_back(fade_message_s::inactive_statement(uri,
            range(position(std::distance(line_details_it_b, faded_line_it), 0),
                position(std::distance(line_details_it_b, std::prev(active_line)), 80))));

        faded_line_it = std::find_if(active_line, line_details_it_e, faded_line_predicate);
    }
}

void filter_and_emplace_hc_map(
    processing::hit_count_map& to, const processing::hit_count_map& from, const resource_location& rl)
{
    auto from_it = from.find(rl);
    if (from_it == from.end())
        return;

    const auto& from_hc_entry = from_it->second;
    if (auto [to_hc_it, new_element] = to.try_emplace(rl, from_hc_entry); !new_element)
        to_hc_it->second.merge(from_hc_entry);
}

void filter_and_emplace_mac_cpy_definitions(
    rl_mac_cpy_map& active_rl_mac_cpy_map, const lsp::lsp_context* lsp_ctx, const resource_location& rl)
{
    if (!lsp_ctx)
        return;

    for (const auto& [_, mac_info_ptr] : lsp_ctx->macros())
    {
        if (!mac_info_ptr || !mac_info_ptr->macro_definition)
            continue;

        const auto mac_definition_emplacer = [&active_rl_mac_cpy_map, &rl](const auto& definition,
                                                 bool cpy_book) -> mac_cpybook_definition_details* {
            const auto& def_location = definition->definition_location;
            if (def_location.resource_loc != rl)
                return nullptr;

            const auto& lines = definition->cached_definition;
            if (lines.empty())
                return nullptr;

            const auto& first_line = lines.front().get_base();
            const auto& last_line = lines.back().get_base();
            if (!first_line || !last_line)
                return nullptr;

            return &active_rl_mac_cpy_map[rl]
                        .emplace(def_location.pos.line,
                            mac_cpybook_definition_details { cpy_book, last_line->statement_position().line })
                        .first->second;
        };

        const auto& mac_def = mac_info_ptr->macro_definition;
        if (auto entry = mac_definition_emplacer(mac_def, false); entry != nullptr)
            entry->prototype_line = mac_info_ptr->definition_location.pos.line;

        for (const auto& cpy_member : mac_def->used_copy_members)
        {
            if (cpy_member)
                mac_definition_emplacer(cpy_member, true);
        }
    }
}

void fade_unused_mac_names(const processing::hit_count_map& hc_map,
    const rl_mac_cpy_map& active_rl_mac_cpy_map,
    std::vector<fade_message_s>& fms)
{
    for (const auto& [active_rl, active_mac_cpy_defs] : active_rl_mac_cpy_map)
    {
        auto hc_map_it = hc_map.find(active_rl);
        if (hc_map_it == hc_map.end() || !hc_map_it->second.has_sections)
            continue;

        const auto& encountered_macro_def_lines = hc_map_it->second.macro_definition_lines;
        for (const auto& [mac_cpy_def_start_line, mac_cpy_def_details] : active_mac_cpy_defs)
        {
            if (!mac_cpy_def_details.cpy_book && !encountered_macro_def_lines.contains(mac_cpy_def_start_line))
                fms.emplace_back(fade_message_s::unused_macro(active_rl.get_uri(),
                    range(position(mac_cpy_def_details.prototype_line, 0),
                        position(mac_cpy_def_details.prototype_line, 80))));
        }
    }
}
} // namespace

void workspace::retrieve_fade_messages(std::vector<fade_message_s>& fms) const
{
    processing::hit_count_map hc_map;
    rl_mac_cpy_map active_rl_mac_cpy_map;

    std::unordered_map<std::string, const resource_location*, utils::hashers::string_hasher, std::equal_to<>>
        opened_files_uris;

    for (const auto& [rl, component] : m_processor_files)
        if (component.m_opened)
            opened_files_uris.try_emplace(rl.get_uri(), &rl);

    for (const auto& [_, proc_file_component] : m_processor_files)
    {
        if (const auto& pfm = proc_file_component.m_last_results->fade_messages)
            std::copy_if(pfm->begin(), pfm->end(), std::back_inserter(fms), [&opened_files_uris](const auto& fmsg) {
                return opened_files_uris.contains(fmsg.uri);
            });

        bool take_also_opencode_hc = true;
        if (const auto& pf_rl = proc_file_component.m_file->get_location();
            &get_proc_grp(pf_rl) == &implicit_proc_grp && is_dependency(pf_rl))
            take_also_opencode_hc = false;

        for (const auto& [__, opened_file_rl] : opened_files_uris)
        {
            filter_and_emplace_hc_map(hc_map, proc_file_component.m_last_results->hc_macro_map, *opened_file_rl);
            if (take_also_opencode_hc)
                filter_and_emplace_hc_map(hc_map, proc_file_component.m_last_results->hc_opencode_map, *opened_file_rl);
            filter_and_emplace_mac_cpy_definitions(
                active_rl_mac_cpy_map, proc_file_component.m_last_results->lsp_context.get(), *opened_file_rl);
        }
    }

    fade_unused_mac_names(hc_map, active_rl_mac_cpy_map, fms);

    std::for_each(hc_map.begin(), hc_map.end(), [&active_rl_mac_cpy_map, &fms](const auto& e) {
        generate_merged_fade_messages(e.first, e.second, active_rl_mac_cpy_map, fms);
    });
}

std::vector<const workspace::processor_file_compoments*> workspace::find_related_opencodes(
    const resource_location& document_loc) const
{
    std::vector<const processor_file_compoments*> opencodes;

    if (auto f = find_processor_file_impl(document_loc); f)
        opencodes.push_back(f);

    for (const auto& [_, component] : m_processor_files)
    {
        if (component.m_dependencies.contains(document_loc))
            opencodes.push_back(&component);
    }

    return opencodes;
}

void workspace::delete_diags(processor_file_compoments& pfc)
{
    // TODO:
    // this function just looks wrong, we delete diagnostics for dependencies
    // regardless of in what files they are used
    pfc.m_last_results->opencode_diagnostics.clear();

    for (const auto& [dep, _] : pfc.m_dependencies)
    {
        if (auto dep_file = find_processor_file_impl(dep))
        {
            pfc.m_last_results->macro_diagnostics.clear();
        }
    }

    pfc.m_last_results->opencode_diagnostics.push_back(diagnostic_s::info_SUP(pfc.m_file->get_location()));
}

void workspace::show_message(const std::string& message)
{
    if (message_consumer_)
        message_consumer_->show_message(message.c_str(), message_type::MT_INFO);
}

lib_config workspace::get_config() const { return m_configuration.get_config().fill_missing_settings(global_config_); }

const ws_uri& workspace::uri() const { return location_.get_uri(); }

utils::value_task<parse_file_result> workspace::parse_file(const resource_location& preferred_file)
{
    if (m_parsing_pending.empty())
        return {};

    processor_file_compoments& comp =
        m_processor_files.at(m_parsing_pending.contains(preferred_file) ? preferred_file : *m_parsing_pending.begin());

    assert(comp.m_opened);

    if (!comp.m_last_opencode_id_storage)
        comp.m_last_opencode_id_storage = std::make_shared<context::id_storage>();

    return [](processor_file_compoments& comp, workspace& self) -> utils::value_task<parse_file_result> {
        const auto& url = comp.m_file->get_location();

        comp.m_alternative_config = co_await self.m_configuration.load_alternative_config_if_needed(url);
        workspace_parse_lib_provider ws_lib(self, comp);

        if (auto prefetch = ws_lib.prefetch_libraries(); prefetch.valid())
            co_await std::move(prefetch);

        bool collect_perf_metrics = comp.m_collect_perf_metrics;

        auto results = co_await parse_one_file(comp.m_last_opencode_id_storage,
            comp.m_file,
            ws_lib,
            self.get_asm_options(url),
            self.get_preprocessor_options(url),
            &self.fm_vfm_);
        results.hc_macro_map = std::move(comp.m_last_results->hc_macro_map); // save macro stuff
        results.macro_diagnostics = std::move(comp.m_last_results->macro_diagnostics);
        *comp.m_last_results = std::move(results);

        std::set<resource_location> files_to_close;
        ws_lib.append_files_to_close(files_to_close);

        auto parse_results = self.parse_successful(comp, std::move(ws_lib));

        self.filter_and_close_dependencies(std::move(files_to_close));

        auto [errors, warnings] = std::pair<size_t, size_t>();
        for (const auto& d : comp.m_last_results->opencode_diagnostics)
        {
            errors += d.severity == diagnostic_severity::error;
            warnings += d.severity == diagnostic_severity::warning;
        }

        co_return parse_file_result {
            .filename = url,
            .parse_results = std::move(parse_results),
            .metrics_to_report = collect_perf_metrics ? std::optional<performance_metrics>(comp.m_last_results->metrics)
                                                      : std::optional<performance_metrics>(),
            .errors = errors,
            .warnings = warnings,
        };
    }(comp, *this);
}

namespace {
bool trigger_reparse(const resource_location& file_location) { return !file_location.get_uri().starts_with("hlasm:"); }
} // namespace

void workspace::mark_all_opened_files()
{
    for (const auto& [fname, comp] : m_processor_files)
        if (comp.m_opened)
            m_parsing_pending.emplace(fname);
}

utils::task workspace::mark_file_for_parsing(
    const resource_location& file_location, file_content_state file_content_status)
{
    if (file_content_status == file_content_state::identical)
        return {};

    // TODO: add support for hlasm to vscode (auto detection??) and do the decision based on languageid
    // TODO: what about removing files??? what if depentands_ points to not existing file?
    // TODO: apparently just opening a file without changing it triggers reparse

    if (file_content_status == file_content_state::changed_content && trigger_reparse(file_location))
    {
        for (auto& [_, component] : m_processor_files)
        {
            if (!component.m_opened)
                continue;
            if (component.m_dependencies.contains(file_location))
                m_parsing_pending.emplace(component.m_file->get_location());
        }
    }

    if (auto it = m_processor_files.find(file_location); it != m_processor_files.end() && it->second.m_opened)
    {
        m_parsing_pending.emplace(it->second.m_file->get_location());
        return it->second.update_source_if_needed(file_manager_);
    }

    return {};
}

workspace_file_info workspace::parse_successful(processor_file_compoments& comp, workspace_parse_lib_provider libs)
{
    workspace_file_info ws_file_info;

    comp.m_collect_perf_metrics = false; // only on open/first parsing
    m_parsing_pending.erase(comp.m_file->get_location());

    const processor_group& grp = get_proc_grp(comp.m_file->get_location());
    ws_file_info.processor_group_found = &grp != &implicit_proc_grp;
    if (&grp == &implicit_proc_grp
        && (int64_t)comp.m_last_results->opencode_diagnostics.size() > get_config().diag_supress_limit)
    {
        ws_file_info.diagnostics_suppressed = true;
        delete_diags(comp);
    }

    for (auto&& [vfh, url] : comp.m_last_results->vf_handles)
        libs.next_dependencies.try_emplace(std::move(url), std::move(vfh));
    comp.m_last_results->vf_handles.clear();

    ws_file_info.files_processed = libs.next_dependencies.size() + 1; // TODO: identify error states?

    comp.m_dependencies = std::move(libs.next_dependencies);
    comp.m_member_map = std::move(libs.next_member_map);

    return ws_file_info;
}

utils::task workspace::did_open_file(resource_location file_location, file_content_state file_content_status)
{
    if (!m_configuration.is_configuration_file(file_location))
    {
        auto& file = co_await add_processor_file_impl(co_await file_manager_.add_file(file_location));
        file.m_opened = true;
        file.m_collect_perf_metrics = true;
        m_parsing_pending.emplace(file_location);
        if (auto t = mark_file_for_parsing(file_location, file_content_status); t.valid())
            co_await std::move(t);
    }
    else
    {
        if (co_await m_configuration.parse_configuration_file(file_location) == parse_config_file_result::parsed)
            mark_all_opened_files();
    }
}

utils::task workspace::did_close_file(resource_location file_location)
{
    auto fcomp = m_processor_files.find(file_location);
    if (fcomp == m_processor_files.end())
        co_return; // this indicates some kind of double close or configuration file close

    fcomp->second.m_opened = false;
    m_parsing_pending.erase(file_location);

    bool found_dependency = false;
    // first check whether the file is a dependency
    std::vector<utils::task> pending_updates;
    for (std::shared_ptr<file> file; const auto& [_, component] : m_processor_files)
    {
        auto it = component.m_dependencies.find(file_location);
        if (it == component.m_dependencies.end())
            continue;
        if (!std::holds_alternative<std::shared_ptr<dependency_cache>>(it->second))
            continue;

        found_dependency = true;

        if (!file)
            file = co_await file_manager_.add_file(file_location);

        if (file->get_version() == std::get<std::shared_ptr<dependency_cache>>(it->second)->version)
            continue;

        if (auto t = mark_file_for_parsing(file_location, file_content_state::changed_content); t.valid() && !t.done())
            pending_updates.emplace_back(std::move(t));
        break;
    }
    co_await utils::task::wait_all(std::move(pending_updates));
    if (found_dependency)
        co_return;

    // find if the file is a dependant

    std::set<resource_location> files_to_close;
    for (const auto& [dep, _] : fcomp->second.m_dependencies)
        files_to_close.insert(dep);
    // filter the dependencies that should not be closed
    filter_and_close_dependencies(std::move(files_to_close), &fcomp->second);

    // close the file itself
    m_processor_files.erase(fcomp);
}

utils::task workspace::did_change_file(resource_location file_location, file_content_state file_content_status)
{
    if (m_configuration.is_configuration_file(file_location))
    {
        return m_configuration.parse_configuration_file(file_location).then([this](auto result) {
            if (result == parse_config_file_result::parsed)
                mark_all_opened_files();
        });
    }
    else
        return mark_file_for_parsing(file_location, file_content_status);
}

utils::task workspace::did_change_watched_files(
    std::vector<resource_location> file_locations, std::vector<file_content_state> file_change_status)
{
    assert(file_locations.size() == file_change_status.size());

    auto refreshed = co_await m_configuration.refresh_libraries(file_locations);
    auto cit = file_change_status.begin();

    std::vector<utils::task> pending_updates;
    for (const auto& file_location : file_locations)
    {
        auto change_status = *cit++;
        auto t = mark_file_for_parsing(file_location, refreshed ? file_content_state::changed_content : change_status);
        if (!t.valid() || t.done())
            continue;

        pending_updates.emplace_back(std::move(t));
    }
    if (refreshed)
    {
        for (const auto& [_, comp] : m_processor_files)
        {
            if (!comp.m_opened)
                continue;

            auto loc = comp.m_file->get_location();
            const auto* pg = &get_proc_grp(loc);
            if (std::find(refreshed->begin(), refreshed->end(), pg) != refreshed->end())
                m_parsing_pending.emplace(std::move(loc));
        }
    }
    co_await utils::task::wait_all(std::move(pending_updates));
}

location workspace::definition(const resource_location& document_loc, position pos) const
{
    auto opencodes = find_related_opencodes(document_loc);
    if (opencodes.empty())
        return { pos, document_loc };
    // for now take last opencode
    if (const auto* lsp_context = opencodes.back()->m_last_results->lsp_context.get())
        return lsp_context->definition(document_loc, pos);
    else
        return { pos, document_loc };
}

location_list workspace::references(const resource_location& document_loc, position pos) const
{
    auto opencodes = find_related_opencodes(document_loc);
    if (opencodes.empty())
        return {};
    // for now take last opencode
    if (const auto* lsp_context = opencodes.back()->m_last_results->lsp_context.get())
        return lsp_context->references(document_loc, pos);
    else
        return {};
}

std::string workspace::hover(const resource_location& document_loc, position pos) const
{
    auto opencodes = find_related_opencodes(document_loc);
    if (opencodes.empty())
        return {};
    // for now take last opencode
    if (const auto* lsp_context = opencodes.back()->m_last_results->lsp_context.get())
        return lsp_context->hover(document_loc, pos);
    else
        return {};
}

lsp::completion_list_s workspace::completion(
    const resource_location& document_loc, position pos, const char trigger_char, completion_trigger_kind trigger_kind)
{
    auto opencodes = find_related_opencodes(document_loc);
    if (opencodes.empty())
        return {};
    // for now take last opencode
    const auto* lsp_context = opencodes.back()->m_last_results->lsp_context.get();
    if (!lsp_context)
        return {};

    auto comp = lsp_context->completion(document_loc, pos, trigger_char, trigger_kind);
    if (auto* cli = std::get_if<lsp::completion_list_instructions>(&comp); cli && !cli->completed_text.empty())
    {
        auto raw_suggestions = make_opcode_suggestion(document_loc, cli->completed_text, true);
        cli->additional_instructions.reserve(raw_suggestions.size());
        for (auto&& [suggestion, rank] : raw_suggestions)
            cli->additional_instructions.emplace_back(std::move(suggestion));
    }
    return lsp::generate_completion(comp);
}

lsp::document_symbol_list_s workspace::document_symbol(const resource_location& document_loc, long long limit) const
{
    auto opencodes = find_related_opencodes(document_loc);
    if (opencodes.empty())
        return {};
    // for now take last opencode
    if (const auto* lsp_context = opencodes.back()->m_last_results->lsp_context.get())
        return lsp_context->document_symbol(document_loc, limit);
    else
        return {};
}

std::vector<token_info> workspace::semantic_tokens(const resource_location& document_loc) const
{
    auto comp = find_processor_file_impl(document_loc);
    if (!comp)
        return {};

    return comp->m_last_results->hl_info;
}

std::optional<performance_metrics> workspace::last_metrics(const resource_location& document_loc) const
{
    auto comp = find_processor_file_impl(document_loc);
    if (!comp)
        return {};

    return comp->m_last_results->metrics;
}

utils::task workspace::open()
{
    opened_ = true;

    co_await m_configuration.parse_configuration_file();
}

void workspace::close() { opened_ = false; }

void workspace::set_message_consumer(message_consumer* consumer) { message_consumer_ = consumer; }

file_manager& workspace::get_file_manager() const { return file_manager_; }

utils::value_task<bool> workspace::settings_updated()
{
    bool updated = m_configuration.settings_updated();
    if (updated && co_await m_configuration.parse_configuration_file() == parse_config_file_result::parsed)
        mark_all_opened_files();
    co_return updated;
}

const processor_group& workspace::get_proc_grp(const resource_location& file) const
{
    if (const auto* pgm = m_configuration.get_program(file); pgm)
    {
        if (auto proc_grp = m_configuration.get_proc_grp_by_program(*pgm); proc_grp)
            return *proc_grp;
    }

    return implicit_proc_grp;
}

const processor_group& workspace::get_proc_grp(const proc_grp_id& id) const { return m_configuration.get_proc_grp(id); }

namespace {
auto generate_instruction_bk_tree(instruction_set_version version)
{
    utils::bk_tree<std::string_view, utils::levenshtein_distance_t<16>> result;

    result.reserve(context::get_instruction_sizes(version).total());

    for (const auto& i : context::instruction::all_assembler_instructions())
        result.insert(i.name());
    for (const auto& i : context::instruction::all_ca_instructions())
        result.insert(i.name());
    for (const auto& i : context::instruction::all_machine_instructions())
        if (instruction_available(i.instr_set_affiliation(), version))
            result.insert(i.name());
    for (const auto& i : context::instruction::all_mnemonic_codes())
        if (instruction_available(i.instr_set_affiliation(), version))
            result.insert(i.name());

    return result;
};

template<instruction_set_version instr_set>
const utils::bk_tree<std::string_view, utils::levenshtein_distance_t<16>>& get_instruction_bk_tree()
{
    static const auto tree = generate_instruction_bk_tree(instr_set);

    return tree;
}

constexpr const utils::bk_tree<std::string_view, utils::levenshtein_distance_t<16>>& (*instruction_bk_trees[])() = {
    nullptr,
    &get_instruction_bk_tree<instruction_set_version::ZOP>,
    &get_instruction_bk_tree<instruction_set_version::YOP>,
    &get_instruction_bk_tree<instruction_set_version::Z9>,
    &get_instruction_bk_tree<instruction_set_version::Z10>,
    &get_instruction_bk_tree<instruction_set_version::Z11>,
    &get_instruction_bk_tree<instruction_set_version::Z12>,
    &get_instruction_bk_tree<instruction_set_version::Z13>,
    &get_instruction_bk_tree<instruction_set_version::Z14>,
    &get_instruction_bk_tree<instruction_set_version::Z15>,
    &get_instruction_bk_tree<instruction_set_version::Z16>,
    &get_instruction_bk_tree<instruction_set_version::ESA>,
    &get_instruction_bk_tree<instruction_set_version::XA>,
    &get_instruction_bk_tree<instruction_set_version::_370>,
    &get_instruction_bk_tree<instruction_set_version::DOS>,
    &get_instruction_bk_tree<instruction_set_version::UNI>,
};

std::vector<std::pair<std::string, size_t>> generate_instruction_suggestions(
    std::string_view opcode, instruction_set_version set, bool extended)
{
    const auto iset_id = static_cast<int>(set);
    assert(0 < iset_id && iset_id <= static_cast<int>(instruction_set_version::UNI));

    constexpr auto process = [](std::span<const std::pair<const std::string_view*, size_t>> suggestions) {
        std::vector<std::pair<std::string, size_t>> result;
        for (const auto& [suggestion, distance] : suggestions)
        {
            if (!suggestion)
                break;
            if (distance == 0)
                break;
            result.emplace_back(*suggestion, distance);
        }

        return result;
    };

    if (extended)
    {
        auto suggestion = instruction_bk_trees[iset_id]().find<10>(opcode, 4);
        return process(suggestion);
    }
    else
    {
        auto suggestion = instruction_bk_trees[iset_id]().find<3>(opcode, 3);
        return process(suggestion);
    }
}
} // namespace

std::vector<std::pair<std::string, size_t>> workspace::make_opcode_suggestion(
    const resource_location& file, std::string_view opcode_, bool extended)
{
    std::string opcode(opcode_);
    for (auto& c : opcode)
        c = static_cast<char>(std::toupper((unsigned char)c));

    std::vector<std::pair<std::string, size_t>> result;
    asm_option opts;

    if (auto pgm = m_configuration.get_program(file); !pgm)
        implicit_proc_grp.apply_options_to(opts);
    else
    {
        if (auto proc_grp = m_configuration.get_proc_grp_by_program(*pgm); proc_grp)
        {
            proc_grp->apply_options_to(opts);
            result = proc_grp->suggest(opcode, extended);
        }
        pgm->asm_opts.apply_options_to(opts);
    }

    for (auto&& s : generate_instruction_suggestions(opcode, opts.instr_set, extended))
        result.emplace_back(std::move(s));
    std::stable_sort(result.begin(), result.end(), [](const auto& l, const auto& r) { return l.second < r.second; });

    return result;
}

template<bool Multi = false, typename T, typename U, typename Projector = std::identity>
void erase_ordered(T& from, const U& what, Projector p = Projector())
{
    auto f = from.begin();
    auto w = what.begin();
    while (f != from.end() && w != what.end())
    {
        if (auto c = *f <=> std::invoke(p, *w); c == 0)
        {
            f = from.erase(f);
            if constexpr (!Multi)
                ++w;
        }
        else if (c < 0)
            f = from.lower_bound(std::invoke(p, *w));
        else
            w = what.lower_bound(*f);
    }
}

void workspace::filter_and_close_dependencies(
    std::set<resource_location> files_to_close_candidates, const processor_file_compoments* file_to_ignore)
{
    // filters the files that are dependencies of other dependants and externally open files
    for (const auto& [_, component] : m_processor_files)
    {
        if (files_to_close_candidates.empty())
            return;

        if (&component == file_to_ignore)
            continue;

        if (component.m_opened)
            files_to_close_candidates.erase(component.m_file->get_location());

        erase_ordered(files_to_close_candidates,
            component.m_dependencies,
            &decltype(component.m_dependencies)::value_type::first);
    }

    // close all exclusive dependencies of file
    for (const auto& dep : files_to_close_candidates)
    {
        m_processor_files.erase(dep);
    }
}

bool workspace::is_dependency(const resource_location& file_location) const
{
    for (const auto& [_, component] : m_processor_files)
    {
        if (component.m_dependencies.contains(file_location))
            return true;
    }
    return false;
}

std::vector<std::shared_ptr<library>> workspace::get_libraries(const resource_location& file_location) const
{
    return get_proc_grp(file_location).libraries();
}

asm_option workspace::get_asm_options(const resource_location& file_location) const
{
    asm_option result;

    auto pgm = m_configuration.get_program(file_location);
    if (!pgm)
        implicit_proc_grp.apply_options_to(result);
    else
    {
        if (auto proc_grp = m_configuration.get_proc_grp_by_program(*pgm); proc_grp)
            proc_grp->apply_options_to(result);
        pgm->asm_opts.apply_options_to(result);
    }

    resource_location relative_to_location(file_location.lexically_relative(location_).lexically_normal());

    const auto& sysin_path = !pgm && (relative_to_location.empty() || relative_to_location.lexically_out_of_scope())
        ? file_location
        : relative_to_location;
    result.sysin_member = sysin_path.filename();
    result.sysin_dsn = sysin_path.parent().get_local_path_or_uri();

    return result;
}

std::vector<preprocessor_options> workspace::get_preprocessor_options(const resource_location& file_location) const
{
    return get_proc_grp(file_location).preprocessors();
}

workspace::processor_file_compoments::processor_file_compoments(std::shared_ptr<file> file)
    : m_file(std::move(file))
    , m_last_results(std::make_unique<parsing_results>())
{}
workspace::processor_file_compoments::processor_file_compoments(processor_file_compoments&&) noexcept = default;
workspace::processor_file_compoments& workspace::processor_file_compoments::operator=(
    processor_file_compoments&&) noexcept = default;
workspace::processor_file_compoments::~processor_file_compoments() = default;

utils::task workspace::processor_file_compoments::update_source_if_needed(file_manager& fm)
{
    if (!m_file->up_to_date())
    {
        return fm.add_file(m_file->get_location()).then([this](std::shared_ptr<file> f) {
            m_file = std::move(f);
            *m_last_results = {};
        });
    }
    return {};
}

utils::value_task<workspace::processor_file_compoments&> workspace::add_processor_file_impl(std::shared_ptr<file> f)
{
    const auto& loc = f->get_location();
    if (auto it = m_processor_files.find(loc); it != m_processor_files.end())
    {
        if (auto t = it->second.update_source_if_needed(file_manager_); t.valid())
            co_await std::move(t);
        co_return it->second;
    }

    co_return m_processor_files.try_emplace(loc, std::move(f)).first->second;
}

const workspace::processor_file_compoments* workspace::find_processor_file_impl(
    const resource_location& file_location) const
{
    if (auto it = m_processor_files.find(file_location);
        it != m_processor_files.end() && it->second.m_file->up_to_date())
        return &it->second;
    return nullptr;
}

} // namespace hlasm_plugin::parser_library::workspaces
