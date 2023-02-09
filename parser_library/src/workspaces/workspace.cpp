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
#include <memory>

#include "context/instruction.h"
#include "file_impl.h"
#include "file_manager.h"
#include "lsp/document_symbol_item.h"
#include "lsp/item_convertors.h"
#include "lsp/lsp_context.h"
#include "processor_file_impl.h"
#include "utils/bk_tree.h"
#include "utils/levenshtein_distance.h"
#include "utils/path.h"

namespace hlasm_plugin::parser_library::workspaces {

struct workspace_parse_lib_provider final : public parse_lib_provider
{
    workspace& ws;
    std::vector<std::shared_ptr<library>> libraries;
    std::unordered_map<utils::resource::resource_location,
        std::shared_ptr<file>,
        utils::resource::resource_location_hasher>
        used_files;

    workspace_parse_lib_provider(workspace& ws, const utils::resource::resource_location& loc)
        : ws(ws)
        , libraries(ws.get_proc_grp_by_program(loc).libraries())
    {}

    // Inherited via parse_lib_provider
    parse_result parse_library(std::string_view library, analyzing_context ctx, library_data data) override
    {
        utils::resource::resource_location url;
        for (const auto& lib : libraries)
        {
            if (!lib->has_file(library, &url))
                continue;

            auto found = ws.add_processor_file_impl(url).m_processor_file;
            assert(found);

            used_files.try_emplace(url, found->current_source());

            return found->parse_macro(*this, std::move(ctx), std::move(data));
        }

        return false;
    }
    bool has_library(std::string_view library, utils::resource::resource_location* loc) const override
    {
        return std::any_of(libraries.begin(), libraries.end(), [&library, loc](const auto& lib) {
            return lib->has_file(library, loc);
        });
    }
    std::optional<std::pair<std::string, utils::resource::resource_location>> get_library(
        std::string_view library) const override
    {
        utils::resource::resource_location url;
        for (const auto& lib : libraries)
        {
            if (!lib->has_file(library, &url))
                continue;

            auto content = ws.file_manager_.get_file_content(url);
            if (!content.has_value())
                return std::nullopt;

            return std::make_pair(std::move(content).value(), std::move(url));
        }
        return std::nullopt;
    }
};

workspace::workspace(const utils::resource::resource_location& location,
    const std::string& name,
    file_manager& file_manager,
    const lib_config& global_config,
    const shared_json& global_settings,
    std::atomic<bool>* cancel)
    : cancel_(cancel)
    , name_(name)
    , location_(location.lexically_normal())
    , file_manager_(file_manager)
    , fm_vfm_(file_manager_, location)
    , implicit_proc_grp("pg_implicit", {}, {})
    , global_config_(global_config)
    , m_configuration(file_manager, location_, global_settings)
{}

workspace::workspace(const utils::resource::resource_location& location,
    file_manager& file_manager,
    const lib_config& global_config,
    const shared_json& global_settings,
    std::atomic<bool>* cancel)
    : workspace(location, location.get_uri(), file_manager, global_config, global_settings, cancel)
{}

workspace::workspace(file_manager& file_manager,
    const lib_config& global_config,
    const shared_json& global_settings,
    std::atomic<bool>* cancel)
    : workspace(utils::resource::resource_location(""), file_manager, global_config, global_settings, cancel)
{
    opened_ = true;
}

void workspace::collect_diags() const
{
    std::unordered_set<utils::resource::resource_location, utils::resource::resource_location_hasher> used_b4g_configs;

    for (const auto& [_, details] : opened_files_)
        used_b4g_configs.emplace(details.alternative_config);

    m_configuration.copy_diagnostics(*this, used_b4g_configs);

    for (const auto& [_, pfc] : m_processor_files)
        collect_diags_from_child(*pfc.m_processor_file);
}

void workspace::retrieve_fade_messages(std::vector<fade_message_s>& fms) const
{
    for (const auto& [key, value] : m_processor_files)
        value.m_processor_file->retrieve_fade_messages(fms);
}

std::vector<std::shared_ptr<processor_file>> workspace::find_related_opencodes(
    const utils::resource::resource_location& document_loc) const
{
    std::vector<std::shared_ptr<processor_file>> opencodes;

    if (auto f = find_processor_file(document_loc))
        opencodes.push_back(f);

    for (const auto& dep : dependants_)
    {
        auto f = find_processor_file(dep);
        if (!f)
            continue;
        if (f->dependencies().contains(document_loc))
            opencodes.push_back(std::move(f));
    }

    return opencodes;
}

void workspace::delete_diags(std::shared_ptr<processor_file> file)
{
    file->diags().clear();

    for (const auto& dep : file->dependencies())
    {
        auto dep_file = find_processor_file(dep);
        if (dep_file)
            dep_file->diags().clear();
    }

    file->diags().push_back(diagnostic_s::info_SUP(file->get_location()));
}

void workspace::show_message(const std::string& message)
{
    if (message_consumer_)
        message_consumer_->show_message(message, message_type::MT_INFO);
}

lib_config workspace::get_config() const { return m_configuration.get_config().fill_missing_settings(global_config_); }

const ws_uri& workspace::uri() const { return location_.get_uri(); }

void workspace::reparse_after_config_refresh()
{
    // Reparse every opened file when configuration is changed
    for (auto& [fname, details] : opened_files_)
    {
        auto comp = find_processor_file_impl(fname);
        if (!comp)
            continue;

        details.alternative_config = m_configuration.load_alternative_config_if_needed(fname);
        workspace_parse_lib_provider ws_lib(*this, fname);
        if (!comp->m_processor_file->parse(ws_lib, get_asm_options(fname), get_preprocessor_options(fname), &fm_vfm_))
            continue;

        (void)parse_successful(*comp, std::move(ws_lib));
    }

    for (const auto& fname : dependants_)
    {
        if (auto found = find_processor_file(fname); found)
            filter_and_close_dependencies_(found->files_to_close(), found);
    }
}

namespace {
bool trigger_reparse(const utils::resource::resource_location& file_location)
{
    return !file_location.get_uri().starts_with("hlasm:");
}
} // namespace

std::vector<workspace::processor_file_compoments*> workspace::collect_dependants(
    const utils::resource::resource_location& file_location)
{
    std::vector<processor_file_compoments*> result;

    for (const auto& dep : dependants_)
    {
        auto component = find_processor_file_impl(dep);
        if (!component)
            continue;
        for (auto& dep_location : component->m_processor_file->dependencies())
        {
            if (dep_location == file_location)
            {
                result.push_back(component);
                break;
            }
        }
    }

    return result;
}

workspace_file_info workspace::parse_file(
    const utils::resource::resource_location& file_location, open_file_result file_content_status)
{
    workspace_file_info ws_file_info;

    // TODO: add support for hlasm to vscode (auto detection??) and do the decision based on languageid
    if (m_configuration.is_configuration_file(file_location))
    {
        if (file_content_status == open_file_result::identical)
            return {};
        if (m_configuration.parse_configuration_file(file_location) == parse_config_file_result::parsed)
            reparse_after_config_refresh();
        ws_file_info.config_parsing = true;
        return ws_file_info;
    }

    // TODO: what about removing files??? what if depentands_ points to not existing file?
    std::vector<processor_file_compoments*> files_to_parse;

    // TODO: apparently just opening a file without changing it triggers reparse

    if (processor_file_compoments* this_file = nullptr; file_content_status == open_file_result::changed_content
        || file_content_status == open_file_result::changed_lsp
            && !((this_file = find_processor_file_impl(file_location)) != nullptr
                && this_file->m_processor_file->has_lsp_info()))
    {
        if (trigger_reparse(file_location))
            files_to_parse = collect_dependants(file_location);

        if (files_to_parse.empty())
        {
            if (!this_file)
                this_file = &add_processor_file_impl(file_location);
            files_to_parse.push_back(this_file);
        }

        for (auto* component : files_to_parse)
        {
            const auto& f = component->m_processor_file;
            const auto& f_loc = component->m_processor_file->get_location();

            auto alt_cfg = m_configuration.load_alternative_config_if_needed(f_loc);
            if (auto opened_it = opened_files_.find(f_loc); opened_it != opened_files_.end())
                opened_it->second.alternative_config = std::move(alt_cfg);

            workspace_parse_lib_provider ws_lib(*this, f_loc);
            if (!f->parse(ws_lib, get_asm_options(f_loc), get_preprocessor_options(f_loc), &fm_vfm_))
                continue;

            ws_file_info = parse_successful(*component, std::move(ws_lib));
        }

        // second check after all dependants are there to close all files that used to be dependencies
        for (const auto* component : files_to_parse)
            filter_and_close_dependencies_(component->m_processor_file->files_to_close(), component->m_processor_file);
    }

    return ws_file_info;
}

workspace_file_info workspace::parse_successful(processor_file_compoments& comp, workspace_parse_lib_provider libs)
{
    workspace_file_info ws_file_info;

    const auto& f = comp.m_processor_file;

    if (!f->dependencies().empty())
        dependants_.insert(f->get_location());
    f->store_used_files(std::move(libs.used_files));

    const processor_group& grp = get_proc_grp_by_program(f->get_location());
    f->collect_diags();
    ws_file_info.processor_group_found = &grp != &implicit_proc_grp;
    if (&grp == &implicit_proc_grp && (int64_t)f->diags().size() > get_config().diag_supress_limit)
    {
        ws_file_info.diagnostics_suppressed = true;
        delete_diags(f);
    }

    return ws_file_info;
}

bool workspace::refresh_libraries(const std::vector<utils::resource::resource_location>& file_locations)
{
    return m_configuration.refresh_libraries(file_locations);
}

workspace_file_info workspace::did_open_file(
    const utils::resource::resource_location& file_location, open_file_result file_content_status)
{
    if (!m_configuration.is_configuration_file(file_location))
        opened_files_.try_emplace(file_location);

    return parse_file(file_location, file_content_status);
}

void workspace::did_close_file(const utils::resource::resource_location& file_location)
{
    opened_files_.erase(file_location);

    // first check whether the file is a dependency
    // if so, simply close it, no other action is needed
    if (is_dependency_(file_location))
    {
        file_manager_.did_close_file(file_location);
        return;
    }

    std::vector<utils::resource::resource_location> deps_to_cleanup;

    // find if the file is a dependant
    auto fname = dependants_.find(file_location);
    if (fname != dependants_.end())
    {
        auto file = find_processor_file(*fname);

        const auto& deps = file->dependencies();

        // filter the dependencies that should not be closed
        filter_and_close_dependencies_(deps, file);
        deps_to_cleanup.reserve(deps.size());
        deps_to_cleanup.assign(deps.begin(), deps.end());
        // remove it from dependants
        dependants_.erase(fname);
    }

    // close the file itself
    m_processor_files.erase(file_location);
    file_manager_.did_close_file(file_location);
    file_manager_.remove_file(file_location);

    // Erase macros cached for this opencode from all its dependencies
    for (const auto& dep : deps_to_cleanup)
    {
        auto proc_file = find_processor_file(dep);
        if (proc_file)
            proc_file->erase_unused_cache_entries();
    }
}

void workspace::did_change_file(
    const utils::resource::resource_location& file_location, const document_change*, size_t cnt)
{
    parse_file(file_location, cnt ? open_file_result::changed_content : open_file_result::identical);
}

void workspace::did_change_watched_files(const std::vector<utils::resource::resource_location>& file_locations)
{
    bool refreshed = refresh_libraries(file_locations);
    for (const auto& file_location : file_locations)
    {
        parse_file(
            file_location, refreshed ? open_file_result::changed_content : file_manager_.update_file(file_location));
    }
}

location workspace::definition(const utils::resource::resource_location& document_loc, position pos) const
{
    auto opencodes = find_related_opencodes(document_loc);
    if (opencodes.empty())
        return { pos, document_loc };
    // for now take last opencode
    if (const auto* lsp_context = opencodes.back()->get_lsp_context())
        return lsp_context->definition(document_loc, pos);
    else
        return { pos, document_loc };
}

location_list workspace::references(const utils::resource::resource_location& document_loc, position pos) const
{
    auto opencodes = find_related_opencodes(document_loc);
    if (opencodes.empty())
        return {};
    // for now take last opencode
    if (const auto* lsp_context = opencodes.back()->get_lsp_context())
        return lsp_context->references(document_loc, pos);
    else
        return {};
}

std::string workspace::hover(const utils::resource::resource_location& document_loc, position pos) const
{
    auto opencodes = find_related_opencodes(document_loc);
    if (opencodes.empty())
        return {};
    // for now take last opencode
    if (const auto* lsp_context = opencodes.back()->get_lsp_context())
        return lsp_context->hover(document_loc, pos);
    else
        return {};
}

lsp::completion_list_s workspace::completion(const utils::resource::resource_location& document_loc,
    position pos,
    const char trigger_char,
    completion_trigger_kind trigger_kind)
{
    auto opencodes = find_related_opencodes(document_loc);
    if (opencodes.empty())
        return {};
    // for now take last opencode
    const auto* lsp_context = opencodes.back()->get_lsp_context();
    if (!lsp_context)
        return {};

    return lsp::generate_completion(lsp_context->completion(document_loc, pos, trigger_char, trigger_kind),
        [&document_loc, this](std::string_view opcode) {
            auto suggestions = make_opcode_suggestion(document_loc, opcode, true);
            std::vector<std::string> result;
            std::transform(suggestions.begin(), suggestions.end(), std::back_inserter(result), [](auto& e) {
                return std::move(e.first);
            });
            return result;
        });
}

lsp::document_symbol_list_s workspace::document_symbol(
    const utils::resource::resource_location& document_loc, long long limit) const
{
    auto opencodes = find_related_opencodes(document_loc);
    if (opencodes.empty())
        return {};
    // for now take last opencode
    if (const auto* lsp_context = opencodes.back()->get_lsp_context())
        return lsp_context->document_symbol(document_loc, limit);
    else
        return {};
}

std::vector<token_info> workspace::semantic_tokens(const utils::resource::resource_location& document_loc) const
{
    auto comp = find_processor_file_impl(document_loc);
    if (!comp)
        return {};

    const auto& f = comp->m_processor_file;

    if (!f || !f->current_source())
        return {};

    return f->get_hl_info();
}

void workspace::open()
{
    opened_ = true;

    m_configuration.parse_configuration_file();
}

void workspace::close() { opened_ = false; }

void workspace::set_message_consumer(message_consumer* consumer) { message_consumer_ = consumer; }

file_manager& workspace::get_file_manager() const { return file_manager_; }

bool workspace::settings_updated()
{
    bool updated = m_configuration.settings_updated();
    if (updated && m_configuration.parse_configuration_file() == parse_config_file_result::parsed)
    {
        reparse_after_config_refresh();
    }
    return updated;
}

const processor_group& workspace::get_proc_grp_by_program(const utils::resource::resource_location& file) const
{
    if (const auto* pgm = m_configuration.get_program(file); pgm)
        return m_configuration.get_proc_grp_by_program(*pgm);
    else
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
    const utils::resource::resource_location& file, std::string_view opcode_, bool extended)
{
    std::string opcode(opcode_);
    for (auto& c : opcode)
        c = static_cast<char>(std::toupper((unsigned char)c));

    std::vector<std::pair<std::string, size_t>> result;

    asm_option opts;
    if (const auto* pgm = m_configuration.get_program(file); pgm)
    {
        auto& proc_grp = m_configuration.get_proc_grp_by_program(*pgm);
        proc_grp.apply_options_to(opts);
        pgm->asm_opts.apply_options_to(opts);

        result = proc_grp.suggest(opcode, extended);
    }
    else
    {
        implicit_proc_grp.apply_options_to(opts);
    }

    for (auto&& s : generate_instruction_suggestions(opcode, opts.instr_set, extended))
        result.emplace_back(std::move(s));
    std::stable_sort(result.begin(), result.end(), [](const auto& l, const auto& r) { return l.second < r.second; });

    return result;
}

void workspace::filter_and_close_dependencies_(
    const std::set<utils::resource::resource_location>& dependencies, std::shared_ptr<processor_file> file)
{
    std::set<utils::resource::resource_location> filtered;
    // filters out externally open files
    for (const auto& dependency : dependencies)
        if (auto dep_file = file_manager_.find(dependency); dep_file && !dep_file->get_lsp_editing())
            filtered.insert(dependency);

    // filters the files that are dependencies of other dependants and externally open files
    for (const auto& dependant : dependants_)
    {
        auto fdependant = find_processor_file(dependant);
        if (!fdependant)
            continue;
        for (auto& dependency : fdependant->dependencies())
        {
            if (fdependant->get_location() != file->get_location() && filtered.contains(dependency))
                filtered.erase(dependency);
        }
    }

    // close all exclusive dependencies of file
    for (auto& dep : filtered)
    {
        m_processor_files.erase(dep);
        file_manager_.did_close_file(dep);
        file_manager_.remove_file(dep);
    }
}

bool workspace::is_dependency_(const utils::resource::resource_location& file_location) const
{
    for (const auto& dependant : dependants_)
    {
        auto fdependant = find_processor_file(dependant);
        if (!fdependant)
            continue;
        for (auto& dependency : fdependant->dependencies())
        {
            if (dependency == file_location)
                return true;
        }
    }
    return false;
}

std::vector<std::shared_ptr<library>> workspace::get_libraries(
    const utils::resource::resource_location& file_location) const
{
    return get_proc_grp_by_program(file_location).libraries();
}

asm_option workspace::get_asm_options(const utils::resource::resource_location& file_location) const
{
    asm_option result;

    const auto* pgm = m_configuration.get_program(file_location);
    if (pgm)
    {
        m_configuration.get_proc_grp_by_program(*pgm).apply_options_to(result);
        pgm->asm_opts.apply_options_to(result);
    }
    else
    {
        implicit_proc_grp.apply_options_to(result);
    }

    utils::resource::resource_location relative_to_location(
        file_location.lexically_relative(location_).lexically_normal());

    const auto& sysin_path = !pgm && (relative_to_location.empty() || relative_to_location.lexically_out_of_scope())
        ? file_location
        : relative_to_location;
    result.sysin_member = sysin_path.filename();
    result.sysin_dsn = sysin_path.parent().get_local_path_or_uri();

    return result;
}

std::vector<preprocessor_options> workspace::get_preprocessor_options(
    const utils::resource::resource_location& file_location) const
{
    return get_proc_grp_by_program(file_location).preprocessors();
}

workspace::processor_file_compoments& workspace::add_processor_file_impl(
    const utils::resource::resource_location& file_location)
{
    if (auto p = find_processor_file_impl(file_location))
        return *p;

    processor_file_compoments pfc {
        std::make_shared<processor_file_impl>(file_manager_.add_file(file_location), file_manager_, cancel_),
    };

    return m_processor_files.insert_or_assign(file_location, std::move(pfc)).first->second;
}

std::shared_ptr<processor_file> workspace::find_processor_file(
    const utils::resource::resource_location& file_location) const
{
    auto p = find_processor_file_impl(file_location);
    if (!p)
        return {};
    return p->m_processor_file;
}

void workspace::processor_file_compoments::update_source_if_needed() const
{
    if (!m_processor_file->current_version())
    {
        m_processor_file->update_source();
    }
}

workspace::processor_file_compoments* workspace::find_processor_file_impl(
    const utils::resource::resource_location& file_location)
{
    if (auto it = m_processor_files.find(file_location); it != m_processor_files.end())
    {
        it->second.update_source_if_needed();
        return &it->second;
    }
    return nullptr;
}

const workspace::processor_file_compoments* workspace::find_processor_file_impl(
    const utils::resource::resource_location& file_location) const
{
    if (auto it = m_processor_files.find(file_location); it != m_processor_files.end())
    {
        it->second.update_source_if_needed();
        return &it->second;
    }
    return nullptr;
}

} // namespace hlasm_plugin::parser_library::workspaces
