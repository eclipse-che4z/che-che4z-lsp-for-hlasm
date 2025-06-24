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
#include "completion_item.h"
#include "completion_trigger_kind.h"
#include "context/hlasm_context.h"
#include "document_symbol_item.h"
#include "fade_messages.h"
#include "file.h"
#include "file_manager.h"
#include "instructions/instruction.h"
#include "lsp/folding.h"
#include "lsp/item_convertors.h"
#include "lsp/lsp_context.h"
#include "macro_cache.h"
#include "output_handler.h"
#include "parse_lib_provider.h"
#include "processing/statement_analyzers/hit_count_analyzer.h"
#include "protocol.h"
#include "semantics/highlighting_info.h"
#include "utils/bk_tree.h"
#include "utils/factory.h"
#include "utils/levenshtein_distance.h"
#include "utils/path_conversions.h"
#include "utils/projectors.h"
#include "utils/transform_inserter.h"

using hlasm_plugin::utils::resource::resource_location;

namespace hlasm_plugin::parser_library::workspaces {

struct workspace::dependency_cache
{
    dependency_cache(version_t version, const file_manager& fm, std::shared_ptr<file> file)
        : version(version)
        , cache(fm, std::move(file))
    {}
    version_t version;
    macro_cache cache;
};

struct workspace::processor_file_compoments
{
    std::shared_ptr<file> m_file;
    std::unique_ptr<parsing_results> m_last_results = std::make_unique<parsing_results>();

    std::map<resource_location, std::variant<std::shared_ptr<dependency_cache>, virtual_file_handle>, std::less<>>
        m_dependencies;
    std::map<std::string, resource_location, std::less<>> m_member_map;

    resource_location m_alternative_config = resource_location();

    bool m_opened = false;
    bool m_collect_perf_metrics = false;

    bool m_last_opencode_analyzer_with_lsp = false;
    bool m_last_macro_analyzer_with_lsp = false;
    std::shared_ptr<context::id_storage> m_last_opencode_id_storage;

    index_t<processor_group, unsigned long long> m_group_id;

    explicit processor_file_compoments(std::shared_ptr<file> file)
        : m_file(std::move(file))
    {}

    [[nodiscard]] utils::task update_source_if_needed(file_manager& fm);
};

struct parsing_results
{
    semantics::lines_info hl_info;
    std::shared_ptr<lsp::lsp_context> lsp_context;
    std::shared_ptr<const std::vector<fade_message>> fade_messages;
    performance_metrics metrics;
    std::vector<std::pair<virtual_file_handle, utils::resource::resource_location>> vf_handles;
    processing::hit_count_map hc_opencode_map;
    processing::hit_count_map hc_macro_map;

    std::vector<diagnostic> opencode_diagnostics;
    std::vector<diagnostic> macro_diagnostics;

    std::vector<output_line> outputs;
};

[[nodiscard]] utils::value_task<parsing_results> parse_one_file(std::shared_ptr<context::id_storage> ids,
    std::shared_ptr<file> file,
    parse_lib_provider& lib_provider,
    asm_option asm_opts,
    std::vector<preprocessor_options> pp,
    virtual_file_monitor* vfm)
{
    struct output_t final : output_handler
    {
        std::vector<output_line> lines;

        void mnote(unsigned char level, std::string_view text) override
        {
            lines.emplace_back(level, std::string(text));
        }
        void punch(std::string_view text) override { lines.emplace_back(-1, std::string(text)); }
    } outputs;

    auto fms = std::make_shared<std::vector<fade_message>>();
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
            &outputs,
        });

    processing::hit_count_analyzer hc_analyzer(a.hlasm_ctx());
    a.register_stmt_analyzer(&hc_analyzer);

    co_await a.co_analyze();
    auto d = a.diags();

    parsing_results result;
    result.opencode_diagnostics.assign(std::make_move_iterator(d.begin()), std::make_move_iterator(d.end()));
    result.hl_info = a.take_semantic_tokens();
    result.lsp_context = a.context().lsp_ctx;
    result.fade_messages = std::move(fms);
    result.metrics = a.get_metrics();
    result.vf_handles = a.take_vf_handles();
    result.hc_opencode_map = hc_analyzer.take_hit_count_map();
    result.outputs = std::move(outputs.lines);

    co_return result;
}

struct workspace_parse_lib_provider final : public parse_lib_provider
{
    file_manager& fm;
    workspace& ws;
    std::vector<std::shared_ptr<library>> libraries;
    workspace::processor_file_compoments& pfc;

    std::map<resource_location,
        std::variant<std::shared_ptr<workspace::dependency_cache>, virtual_file_handle>,
        std::less<>>
        next_dependencies;
    std::map<std::string, resource_location, std::less<>> next_member_map;
    std::unordered_map<resource_location, std::shared_ptr<file>> current_file_map;

    workspace_parse_lib_provider(file_manager& fm,
        workspace& ws,
        std::vector<std::shared_ptr<library>> libraries,
        workspace::processor_file_compoments& pfc)
        : fm(fm)
        , ws(ws)
        , libraries(std::move(libraries))
        , pfc(pfc)
    {}

    void append_files_to_close(std::set<resource_location>& files_to_close)
    {
        std::ranges::set_difference(pfc.m_dependencies,
            next_dependencies,
            utils::transform_inserter(files_to_close, utils::first_element),
            {},
            utils::first_element,
            utils::first_element);
    }

    resource_location get_url(std::string_view library)
    {
        if (auto it = next_member_map.find(library); it != next_member_map.end())
        {
            return it->second;
        }
        else if (resource_location url; std::ranges::none_of(
                     libraries, [&url, &library](const auto& lib) { return lib->has_file(library, &url); }))
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

                    return std::make_shared<workspace::dependency_cache>(version, fm, file);
                }))
                .first->second)
            ->cache;
    }

    // Inherited via parse_lib_provider
    [[nodiscard]] utils::value_task<bool> parse_library(
        std::string library, analyzing_context ctx, processing::processing_kind kind) override
    {
        resource_location url = get_url(library);
        if (url.empty())
            co_return false;

        std::shared_ptr<file> file = co_await get_file(url);
        // TODO: if file is in error do something?

        auto& macro_pfc = co_await ws.add_processor_file_impl(file);

        auto cache_key = macro_cache_key::create_from_context(*ctx.hlasm_ctx, kind, ctx.hlasm_ctx->add_id(library));

        auto& mc = get_cache(url, file);

        if (auto files = mc.load_from_cache(cache_key, ctx); files.has_value())
        {
            for (const auto& f : files.value())
            {
                // carry-over nested copy dependencies
                current_file_map.try_emplace(f->get_location(), f);
                (void)get_cache(f->get_location(), f);
            }

            co_return true;
        }

        const bool collect_hl = file->get_lsp_editing() || macro_pfc.m_last_opencode_analyzer_with_lsp
            || macro_pfc.m_last_macro_analyzer_with_lsp || ctx.hlasm_ctx->processing_stack().parent().empty();
        analyzer a(file->get_text(),
            analyzer_options {
                std::move(url),
                this,
                std::move(ctx),
                analyzer_options::dependency(std::move(library), kind),
                collect_hl ? collect_highlighting_info::yes : collect_highlighting_info::no,
            });

        processing::hit_count_analyzer hc_analyzer(a.hlasm_ctx());
        a.register_stmt_analyzer(&hc_analyzer);

        co_await a.co_analyze();
        auto d = a.diags();

        macro_pfc.m_last_results->macro_diagnostics.assign(
            std::make_move_iterator(d.begin()), std::make_move_iterator(d.end()));

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

workspace::workspace(file_manager& file_manager, configuration_provider& configuration)
    : file_manager_(file_manager)
    , fm_vfm_(file_manager_)
    , m_configuration(configuration)
{}

workspace::~workspace() = default;

std::unordered_map<utils::resource::resource_location, std::vector<utils::resource::resource_location>>
workspace::report_used_configuration_files() const
{
    std::unordered_map<utils::resource::resource_location, std::vector<utils::resource::resource_location>> result;

    for (const auto& [processor_file_rl, component] : m_processor_files)
    {
        if (component.m_opened)
            result[component.m_alternative_config].emplace_back(processor_file_rl);
    }

    return result;
}

void workspace::produce_diagnostics(std::vector<diagnostic>& target) const
{
    for (const auto& [url, pfc] : m_processor_files)
    {
        if (is_dependency(url))
            target.insert(target.end(),
                pfc.m_last_results->macro_diagnostics.begin(),
                pfc.m_last_results->macro_diagnostics.end());
        else
            target.insert(target.end(),
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
using rl_mac_cpy_map = std::unordered_map<resource_location, mac_cpy_definitions_map>;

void generate_merged_fade_messages(const resource_location& rl,
    const processing::hit_count_entry& hc_entry,
    const rl_mac_cpy_map& active_rl_mac_cpy_map,
    std::vector<fade_message>& fms)
{
    if (!hc_entry.has_sections)
        return;

    const mac_cpy_definitions_map* active_mac_cpy_defs_map = nullptr;
    if (const auto active_rl_mac_cpy_map_it = active_rl_mac_cpy_map.find(rl);
        active_rl_mac_cpy_map_it != active_rl_mac_cpy_map.end())
        active_mac_cpy_defs_map = &active_rl_mac_cpy_map_it->second;

    const auto line_details_it_b = hc_entry.details.begin();
    const auto line_details_it_e = hc_entry.details.end();

    const auto faded_line_predicate = [active_mac_cpy_defs_map,
                                          line_details_addr = std::to_address(line_details_it_b),
                                          &hc_entry](const processing::line_detail& e) {
        if (e.macro_body)
        {
            if (!active_mac_cpy_defs_map)
                return false;

            auto lineno = static_cast<size_t>(std::ranges::distance(line_details_addr, &e));
            auto active_mac_cpy_it_e = active_mac_cpy_defs_map->end();

            auto active_mac_cpy_it = std::find_if(active_mac_cpy_defs_map->lower_bound(lineno),
                active_mac_cpy_it_e,
                [lineno](const std::pair<size_t, mac_cpybook_definition_details>& mac_cpy_def) {
                    const auto& [active_mac_cpy_start_line, active_mac_cpy_def_detail] = mac_cpy_def;
                    return lineno >= active_mac_cpy_start_line && lineno <= active_mac_cpy_def_detail.end_line;
                });

            if (active_mac_cpy_it == active_mac_cpy_it_e
                || (!active_mac_cpy_it->second.cpy_book && !hc_entry.contains_prototype(active_mac_cpy_it->first)))
                return false;
        }

        return e.contains_statement && e.count == 0;
    };

    const auto it_b =
        std::ranges::find_if(line_details_it_b, line_details_it_e, &processing::line_detail::contains_statement);
    auto faded_line_it = std::find_if(it_b, line_details_it_e, faded_line_predicate);

    while (faded_line_it != line_details_it_e)
    {
        auto active_line = std::find_if_not(std::next(faded_line_it), line_details_it_e, faded_line_predicate);
        fms.emplace_back(fade_message::inactive_statement(rl.get_uri(),
            range {
                position(std::ranges::distance(line_details_it_b, faded_line_it), 0),
                position(std::ranges::distance(line_details_it_b, std::prev(active_line)), 80),
            }));

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
    std::vector<fade_message>& fms)
{
    for (const auto& [active_rl, active_mac_cpy_defs] : active_rl_mac_cpy_map)
    {
        auto hc_map_it = hc_map.find(active_rl);
        if (hc_map_it == hc_map.end() || !hc_map_it->second.has_sections)
            continue;

        for (const auto& [mac_cpy_def_start_line, mac_cpy_def_details] : active_mac_cpy_defs)
        {
            if (!mac_cpy_def_details.cpy_book && !hc_map_it->second.contains_prototype(mac_cpy_def_start_line))
                fms.emplace_back(fade_message::unused_macro(active_rl.get_uri(),
                    range(position(mac_cpy_def_details.prototype_line, 0),
                        position(mac_cpy_def_details.prototype_line, 80))));
        }
    }
}
} // namespace

void workspace::retrieve_fade_messages(std::vector<fade_message>& fms) const
{
    processing::hit_count_map hc_map;
    rl_mac_cpy_map active_rl_mac_cpy_map;

    std::unordered_map<std::string_view, const resource_location*> opened_files_uris;

    for (const auto& [rl, component] : m_processor_files)
        if (component.m_opened)
            opened_files_uris.try_emplace(rl.get_uri(), &rl);

    for (const auto& [_, proc_file_component] : m_processor_files)
    {
        if (const auto& pfm = proc_file_component.m_last_results->fade_messages)
            std::ranges::copy_if(*pfm, std::back_inserter(fms), [&opened_files_uris](const auto& fmsg) {
                return opened_files_uris.contains(fmsg.uri);
            });

        bool take_also_opencode_hc =
            proc_file_component.m_group_id || !is_dependency(proc_file_component.m_file->get_location());

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

    std::ranges::for_each(hc_map, [&active_rl_mac_cpy_map, &fms](const auto& e) {
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

    pfc.m_last_results->opencode_diagnostics.push_back(info_SUP(std::string(pfc.m_file->get_location().get_uri())));
}

void workspace::show_message(std::string_view message)
{
    if (message_consumer_)
        message_consumer_->show_message(message, message_type::MT_INFO);
}

utils::value_task<parse_file_result> workspace::parse_file(resource_location* selected)
{
    if (m_parsing_pending.empty())
        return {};

    const auto& file_to_parse = *m_parsing_pending.begin();
    if (selected)
        *selected = file_to_parse;
    processor_file_compoments& comp = m_processor_files.at(file_to_parse);

    assert(comp.m_opened);

    if (!comp.m_last_opencode_id_storage)
        comp.m_last_opencode_id_storage = context::hlasm_context::make_default_id_storage();

    return [](processor_file_compoments& comp, workspace& self) -> utils::value_task<parse_file_result> {
        const auto& url = comp.m_file->get_location();

        auto [config, proc_grp_id] = co_await self.m_configuration.get_analyzer_configuration(url);

        comp.m_alternative_config = std::move(config.alternative_config_url);
        workspace_parse_lib_provider ws_lib(self.file_manager_, self, std::move(config.libraries), comp);

        if (auto prefetch = ws_lib.prefetch_libraries(); prefetch.valid())
            co_await std::move(prefetch);

        bool collect_perf_metrics = comp.m_collect_perf_metrics;

        auto results = co_await parse_one_file(comp.m_last_opencode_id_storage,
            comp.m_file,
            ws_lib,
            std::move(config.opts),
            std::move(config.pp_opts),
            &self.fm_vfm_);
        results.hc_macro_map = std::move(comp.m_last_results->hc_macro_map); // save macro stuff
        results.macro_diagnostics = std::move(comp.m_last_results->macro_diagnostics);
        const bool outputs_changed = comp.m_last_results->outputs != results.outputs;
        *comp.m_last_results = std::move(results);

        std::set<resource_location> files_to_close;
        ws_lib.append_files_to_close(files_to_close);

        auto parse_results = self.parse_successful(comp, std::move(ws_lib), !!proc_grp_id, config.dig_suppress_limit);

        comp.m_group_id = proc_grp_id;

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
            .outputs_changed = outputs_changed,
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


void workspace::external_configuration_invalidated(const resource_location& url)
{
    if (url.empty())
        mark_all_opened_files();
    else if (auto it = m_processor_files.find(url); it != m_processor_files.end() && it->second.m_opened)
        m_parsing_pending.emplace(url);
}

workspace_file_info workspace::parse_successful(processor_file_compoments& comp,
    workspace_parse_lib_provider libs,
    bool has_processor_group,
    std::int64_t diag_suppress_limit)
{
    workspace_file_info ws_file_info;

    comp.m_collect_perf_metrics = false; // only on open/first parsing
    m_parsing_pending.erase(comp.m_file->get_location());

    ws_file_info.processor_group_found = has_processor_group;
    if (!has_processor_group && std::cmp_greater(comp.m_last_results->opencode_diagnostics.size(), diag_suppress_limit))
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
    auto& file = co_await add_processor_file_impl(co_await file_manager_.add_file(file_location));
    file.m_opened = true;
    file.m_collect_perf_metrics = true;
    m_parsing_pending.emplace(file_location);
    if (auto t = mark_file_for_parsing(file_location, file_content_status); t.valid())
        co_await std::move(t);
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

utils::task workspace::did_change_watched_files(std::vector<resource_location> file_locations,
    std::vector<file_content_state> file_change_status,
    std::optional<std::vector<index_t<processor_group, unsigned long long>>> changed_groups)
{
    assert(file_locations.size() == file_change_status.size());

    std::vector<utils::task> pending_updates;
    for (auto cit = file_change_status.begin(); const auto& file_location : file_locations)
    {
        auto change_status = *cit++;
        if (changed_groups)
            change_status = file_content_state::changed_content;
        auto t = mark_file_for_parsing(file_location, change_status);
        if (!t.valid() || t.done())
            continue;

        pending_updates.emplace_back(std::move(t));
    }
    if (changed_groups)
    {
        for (const auto& [_, comp] : m_processor_files)
        {
            if (!comp.m_opened)
                continue;

            if (std::ranges::find(*changed_groups, comp.m_group_id) != changed_groups->end())
                m_parsing_pending.emplace(comp.m_file->get_location());
        }
    }
    return utils::task::wait_all(std::move(pending_updates));
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

std::vector<location> workspace::references(const resource_location& document_loc, position pos) const
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

std::vector<completion_item> workspace::completion(
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

std::vector<document_symbol_item> workspace::document_symbol(const resource_location& document_loc) const
{
    auto opencodes = find_related_opencodes(document_loc);
    if (opencodes.empty())
        return {};
    // for now take last opencode
    if (const auto* lsp_context = opencodes.back()->m_last_results->lsp_context.get())
        return lsp_context->document_symbol(document_loc);
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

std::vector<branch_info> workspace::branch_information(const resource_location& document_loc) const
{
    auto comp = find_processor_file_impl(document_loc);
    if (!comp)
        return {};

    if (const auto* lsp_context = comp->m_last_results->lsp_context.get())
        return lsp_context->get_opencode_branch_info();
    else
        return {};
}

std::vector<folding_range> workspace::folding(const resource_location& document_loc) const
{
    auto comp = find_processor_file_impl(document_loc);
    if (!comp)
        return {};

    auto lines = lsp::generate_indentation_map(comp->m_file->get_text());

    lsp::mark_suspicious(lines);

    auto data = lsp::compute_folding_data(lines);

    return lsp::generate_folding_ranges(data);
}

std::vector<output_line> workspace::retrieve_output(const resource_location& document_loc) const
{
    auto comp = find_processor_file_impl(document_loc);
    if (!comp)
        return {};

    return comp->m_last_results->outputs;
}

std::optional<performance_metrics> workspace::last_metrics(const resource_location& document_loc) const
{
    auto comp = find_processor_file_impl(document_loc);
    if (!comp)
        return {};

    return comp->m_last_results->metrics;
}

void workspace::set_message_consumer(message_consumer* consumer) { message_consumer_ = consumer; }

namespace {
auto generate_instruction_bk_tree(instruction_set_version version)
{
    utils::bk_tree<std::string_view, utils::levenshtein_distance_t<16>> result;

    result.reserve(instructions::get_instruction_sizes(version).total());

    for (const auto& i : instructions::all_assembler_instructions())
        result.insert(i.name());
    for (const auto& i : instructions::all_ca_instructions())
        result.insert(i.name());
    for (const auto& i : instructions::all_machine_instructions())
        if (instruction_available(i.instr_set_affiliation(), version))
            result.insert(i.name());
    for (const auto& i : instructions::all_mnemonic_codes())
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
    &get_instruction_bk_tree<instruction_set_version::Z17>,
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

    auto [opts, proc_grp] = m_configuration.get_opcode_suggestion_data(file);

    std::vector<std::pair<std::string, size_t>> result;
    if (proc_grp)
        result = proc_grp->suggest(opcode, extended);

    for (auto&& s : generate_instruction_suggestions(opcode, opts.instr_set, extended))
        result.emplace_back(std::move(s));
    std::ranges::stable_sort(result, {}, utils::second_element);

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

utils::task workspace::processor_file_compoments::update_source_if_needed(file_manager& fm)
{
    if (!m_file->up_to_date())
    {
        return fm.add_file(m_file->get_location()).then([this](std::shared_ptr<file> f) {
            m_file = std::move(f);
            // preserve output - extra change notification event exists
            *m_last_results = { .outputs = std::move(m_last_results->outputs) };
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
