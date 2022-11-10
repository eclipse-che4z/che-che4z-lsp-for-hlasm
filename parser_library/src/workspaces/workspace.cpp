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

#include <filesystem>
#include <memory>

#include "file_manager.h"
#include "utils/path.h"

namespace hlasm_plugin::parser_library::workspaces {

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
    , fm_vfm_(file_manager_)
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

void workspace::collect_diags() const { m_configuration.copy_diagnostics(*this); }

std::vector<processor_file_ptr> workspace::find_related_opencodes(
    const utils::resource::resource_location& document_loc) const
{
    std::vector<processor_file_ptr> opencodes;

    for (const auto& dep : dependants_)
    {
        auto f = file_manager_.find_processor_file(dep);
        if (!f)
            continue;
        if (f->dependencies().contains(document_loc))
            opencodes.push_back(std::move(f));
    }

    if (opencodes.size())
        return opencodes;

    if (auto f = file_manager_.find_processor_file(document_loc))
        return { f };
    return {};
}

void workspace::delete_diags(processor_file_ptr file)
{
    file->diags().clear();

    for (const auto& dep : file->dependencies())
    {
        auto dep_file = file_manager_.find_processor_file(dep);
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
    for (const auto& fname : opened_files_)
    {
        auto found = file_manager_.find_processor_file(fname);
        if (!found)
            continue;
        m_configuration.load_alternative_config_if_needed(fname);
        if (!found->parse(*this, get_asm_options(fname), get_preprocessor_options(fname), &fm_vfm_))
            continue;

        (void)parse_successful(found);
    }

    for (const auto& fname : dependants_)
    {
        if (auto found = file_manager_.find_processor_file(fname); found)
            filter_and_close_dependencies_(found->files_to_close(), found);
    }
}

workspace_file_info workspace::parse_file(const utils::resource::resource_location& file_location)
{
    workspace_file_info ws_file_info;

    // TODO: add support for hlasm to vscode (auto detection??) and do the decision based on languageid
    if (m_configuration.is_configuration_file(file_location))
    {
        if (m_configuration.parse_configuration_file(file_location) == parse_config_file_result::parsed)
            reparse_after_config_refresh();
        ws_file_info.config_parsing = true;
        return ws_file_info;
    }

    // TODO: what about removing files??? what if depentands_ points to not existing file?
    std::vector<processor_file_ptr> files_to_parse;

    for (const auto& dep : dependants_)
    {
        auto f = file_manager_.find_processor_file(dep);
        if (!f)
            continue;
        for (auto& dep_location : f->dependencies())
        {
            if (dep_location == file_location)
            {
                files_to_parse.push_back(f);
                break;
            }
        }
    }

    if (files_to_parse.empty())
    {
        if (auto f = file_manager_.find_processor_file(file_location); f)
            files_to_parse.push_back(f);
    }

    for (const auto& f : files_to_parse)
    {
        m_configuration.load_alternative_config_if_needed(file_location);
        if (!f->parse(*this, get_asm_options(f->get_location()), get_preprocessor_options(f->get_location()), &fm_vfm_))
            continue;

        ws_file_info = parse_successful(f);
    }

    // second check after all dependants are there to close all files that used to be dependencies
    for (const auto& f : files_to_parse)
        filter_and_close_dependencies_(f->files_to_close(), f);

    return ws_file_info;
}

workspace_file_info workspace::parse_successful(const processor_file_ptr& f)
{
    workspace_file_info ws_file_info;

    if (!f->dependencies().empty())
        dependants_.insert(f->get_location());

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

void workspace::refresh_libraries() { m_configuration.refresh_libraries(); }

workspace_file_info workspace::did_open_file(const utils::resource::resource_location& file_location)
{
    if (!m_configuration.is_configuration_file(file_location))
        opened_files_.emplace(file_location);

    return parse_file(file_location);
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

    // find if the file is a dependant
    auto fname = dependants_.find(file_location);
    if (fname != dependants_.end())
    {
        auto file = file_manager_.find_processor_file(*fname);
        // filter the dependencies that should not be closed
        filter_and_close_dependencies_(file->dependencies(), file);
        // Erase macros cached for this opencode from all its dependencies
        for (const auto& dep : file->dependencies())
        {
            auto proc_file = file_manager_.get_processor_file(dep);
            if (proc_file)
                proc_file->erase_cache_of_opencode(file_location);
        }
        // remove it from dependants
        dependants_.erase(fname);
    }

    // close the file itself
    file_manager_.did_close_file(file_location);
    file_manager_.remove_file(file_location);
}

void workspace::did_change_file(const utils::resource::resource_location& file_location, const document_change*, size_t)
{
    parse_file(file_location);
}

void workspace::did_change_watched_files(const std::vector<utils::resource::resource_location>& file_locations)
{
    refresh_libraries();
    for (const auto& file_location : file_locations)
        parse_file(file_location);
}

location workspace::definition(const utils::resource::resource_location& document_loc, const position pos) const
{
    auto opencodes = find_related_opencodes(document_loc);
    if (opencodes.empty())
        return { pos, document_loc };
    // for now take last opencode
    return opencodes.back()->get_lsp_feature_provider().definition(document_loc, pos);
}

location_list workspace::references(const utils::resource::resource_location& document_loc, const position pos) const
{
    auto opencodes = find_related_opencodes(document_loc);
    if (opencodes.empty())
        return {};
    // for now take last opencode
    return opencodes.back()->get_lsp_feature_provider().references(document_loc, pos);
}

lsp::hover_result workspace::hover(const utils::resource::resource_location& document_loc, const position pos) const
{
    auto opencodes = find_related_opencodes(document_loc);
    if (opencodes.empty())
        return {};
    // for now take last opencode
    return opencodes.back()->get_lsp_feature_provider().hover(document_loc, pos);
}

lsp::completion_list_s workspace::completion(const utils::resource::resource_location& document_loc,
    const position pos,
    const char trigger_char,
    completion_trigger_kind trigger_kind) const
{
    auto opencodes = find_related_opencodes(document_loc);
    if (opencodes.empty())
        return {};
    // for now take last opencode
    return opencodes.back()->get_lsp_feature_provider().completion(document_loc, pos, trigger_char, trigger_kind);
}

lsp::document_symbol_list_s workspace::document_symbol(
    const utils::resource::resource_location& document_loc, long long limit) const
{
    auto opencodes = find_related_opencodes(document_loc);
    if (opencodes.empty())
        return {};
    // for now take last opencode
    return opencodes.back()->get_lsp_feature_provider().document_symbol(document_loc, limit);
}

void workspace::open()
{
    opened_ = true;

    m_configuration.parse_configuration_file();
}

void workspace::close() { opened_ = false; }

void workspace::set_message_consumer(message_consumer* consumer) { message_consumer_ = consumer; }

file_manager& workspace::get_file_manager() { return file_manager_; }

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

void workspace::filter_and_close_dependencies_(
    const std::set<utils::resource::resource_location>& dependencies, processor_file_ptr file)
{
    std::set<utils::resource::resource_location> filtered;
    // filters out externally open files
    for (const auto& dependency : dependencies)
    {
        auto dependency_file = file_manager_.find_processor_file(dependency);
        if (dependency_file && !dependency_file->get_lsp_editing())
            filtered.insert(dependency);
    }

    // filters the files that are dependencies of other dependants and externally open files
    for (const auto& dependant : dependants_)
    {
        auto fdependant = file_manager_.find_processor_file(dependant);
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
        file_manager_.did_close_file(dep);
        file_manager_.remove_file(dep);
    }
}

bool workspace::is_dependency_(const utils::resource::resource_location& file_location)
{
    for (const auto& dependant : dependants_)
    {
        auto fdependant = file_manager_.find_processor_file(dependant);
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

parse_result workspace::parse_library(const std::string& library, analyzing_context ctx, library_data data)
{
    auto& proc_grp = get_proc_grp_by_program(ctx.hlasm_ctx->opencode_location());
    for (auto&& lib : proc_grp.libraries())
    {
        std::shared_ptr<processor> found = lib->find_file(library);
        if (found)
            return found->parse_macro(*this, std::move(ctx), data);
    }

    return false;
}

bool workspace::has_library(const std::string& library, const utils::resource::resource_location& program) const
{
    auto& proc_grp = get_proc_grp_by_program(program);
    for (auto&& lib : proc_grp.libraries())
    {
        std::shared_ptr<processor> found = lib->find_file(library);
        if (found)
            return true;
    }

    return false;
}

std::optional<std::string> workspace::get_library(const std::string& library,
    const utils::resource::resource_location& program,
    std::optional<utils::resource::resource_location>& location) const
{
    auto& proc_grp = get_proc_grp_by_program(program);
    for (auto&& lib : proc_grp.libraries())
    {
        std::shared_ptr<processor> found = lib->find_file(library);
        if (!found)
            continue;

        auto f = dynamic_cast<file*>(found.get());
        if (!f) // for now
            return std::nullopt;

        location = f->get_location();
        return f->get_text();
    }
    return std::nullopt;
}

asm_option workspace::get_asm_options(const utils::resource::resource_location& file_location) const
{
    asm_option result;

    const auto* pgm = m_configuration.get_program(file_location);
    if (pgm)
    {
        m_configuration.get_proc_grp_by_program(*pgm).update_asm_options(result);
        pgm->asm_opts.apply(result);
    }
    else
    {
        implicit_proc_grp.update_asm_options(result);
    }

    utils::resource::resource_location relative_to_location(
        file_location.lexically_relative(location_).lexically_normal());

    // TODO - convert sysin_path from std::filesystem::path to utils::resource::resource_location
    std::filesystem::path sysin_path = !pgm
            && (relative_to_location == utils::resource::resource_location()
                || relative_to_location.lexically_out_of_scope())
        ? file_location.get_path()
        : relative_to_location.get_path();
    result.sysin_member = utils::path::filename(sysin_path).string();
    result.sysin_dsn = utils::path::parent_path(sysin_path).string();

    return result;
}

std::vector<preprocessor_options> workspace::get_preprocessor_options(
    const utils::resource::resource_location& file_location) const
{
    return get_proc_grp_by_program(file_location).preprocessors();
}

processor_file_ptr workspace::get_processor_file(const utils::resource::resource_location& file_location)
{
    return get_file_manager().get_processor_file(file_location);
}

} // namespace hlasm_plugin::parser_library::workspaces
