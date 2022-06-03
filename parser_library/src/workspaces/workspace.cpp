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
#include <filesystem>
#include <iostream>
#include <memory>
#include <regex>
#include <string>

#include "file_manager_vfm.h"
#include "lib_config.h"
#include "library_local.h"
#include "nlohmann/json.hpp"
#include "processor.h"
#include "utils/path.h"
#include "utils/platform.h"
#include "wildcard.h"

using json = nlohmann::json;

namespace hlasm_plugin::parser_library::workspaces {

workspace::workspace(const utils::resource::resource_location& location,
    const std::string& name,
    file_manager& file_manager,
    const lib_config& global_config,
    std::atomic<bool>* cancel)
    : cancel_(cancel)
    , name_(name)
    , location_(location)
    , file_manager_(file_manager)
    , fm_vfm_(file_manager_)
    , implicit_proc_grp("pg_implicit", {}, {})
    , global_config_(global_config)
{
    auto hlasm_folder = utils::resource::resource_location::join(location_, HLASM_PLUGIN_FOLDER);
    proc_grps_loc_ = utils::resource::resource_location::join(hlasm_folder, FILENAME_PROC_GRPS);
    pgm_conf_loc_ = utils::resource::resource_location::join(hlasm_folder, FILENAME_PGM_CONF);
}

workspace::workspace(const utils::resource::resource_location& location,
    file_manager& file_manager,
    const lib_config& global_config,
    std::atomic<bool>* cancel)
    : workspace(location, location.get_uri(), file_manager, global_config, cancel)
{}

workspace::workspace(file_manager& file_manager, const lib_config& global_config, std::atomic<bool>* cancel)
    : workspace(utils::resource::resource_location(""), file_manager, global_config, cancel)
{
    opened_ = true;
}

void workspace::collect_diags() const
{
    for (auto& pg : proc_grps_)
    {
        collect_diags_from_child(pg.second);
    }

    for (auto& diag : config_diags_)
        add_diagnostic(diag);
}

void workspace::add_proc_grp(processor_group pg) { proc_grps_.emplace(pg.name(), std::move(pg)); }

bool workspace::program_id_match(const std::string& filename, const program_id& program) const
{
    std::regex prg_regex = wildcard2regex(program);
    return std::regex_match(filename, prg_regex);
}

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

    auto [notified_found, _] = diag_suppress_notified_.try_emplace(file->get_location(), false);
    if (!notified_found->second)
        show_message("Diagnostics suppressed from " + file->get_location().to_presentable()
            + ", because there is no configuration.");
    notified_found->second = true;
}

void workspace::show_message(const std::string& message)
{
    if (message_consumer_)
        message_consumer_->show_message(message, message_type::MT_INFO);
}

lib_config workspace::get_config() { return local_config_.fill_missing_settings(global_config_); }

const processor_group& workspace::get_proc_grp_by_program(const utils::resource::resource_location& file_location) const
{
    const auto* pgm = get_program(file_location);
    if (pgm)
        return proc_grps_.at(pgm->pgroup);
    return implicit_proc_grp;
}
const processor_group& workspace::get_proc_grp_by_program(const program& pgm) const
{
    return proc_grps_.at(pgm.pgroup);
}

const program* workspace::get_program(const utils::resource::resource_location& file_location) const
{
    assert(opened_);

    std::string file =
        utils::path::lexically_normal(utils::path::lexically_relative(file_location.get_path(), location_.get_path()))
            .string();

    // direct match
    auto program = exact_pgm_conf_.find(file);
    if (program != exact_pgm_conf_.cend())
        return &program->second;

    for (const auto& pgm : regex_pgm_conf_)
    {
        if (std::regex_match(file, pgm.second))
            return &pgm.first;
    }
    return nullptr;
}

const ws_uri& workspace::uri() const { return location_.get_uri(); }

bool workspace::is_config_file(const utils::resource::resource_location& file_location) const
{
    return file_location == proc_grps_loc_ || file_location == pgm_conf_loc_;
}

workspace_file_info workspace::parse_config_file()
{
    workspace_file_info ws_file_info;

    if (load_and_process_config())
    {
        // Reparse every opened file when configuration is changed
        for (const auto& fname : opened_files_)
        {
            auto found = file_manager_.find_processor_file(fname);
            if (found)
                found->parse(*this, get_asm_options(fname), get_preprocessor_options(fname), &fm_vfm_);
        }

        for (const auto& fname : dependants_)
        {
            auto found = file_manager_.find_processor_file(fname);
            if (found)
                filter_and_close_dependencies_(found->files_to_close(), found);
        }
    }
    ws_file_info.config_parsing = true;
    return ws_file_info;
}

workspace_file_info workspace::parse_file(const utils::resource::resource_location& file_location)
{
    workspace_file_info ws_file_info;

    // TODO: add support for hlasm to vscode (auto detection??) and do the decision based on languageid
    if (is_config_file(file_location))
    {
        return parse_config_file();
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
        f->parse(*this, get_asm_options(f->get_location()), get_preprocessor_options(f->get_location()), &fm_vfm_);
        if (!f->dependencies().empty())
            dependants_.insert(f->get_location());

        // if there is no processor group assigned to the program, delete diagnostics that may have been created
        if (cancel_ && cancel_->load()) // skip, if parsing was cancelled using the cancellation token
            continue;

        const processor_group& grp = get_proc_grp_by_program(f->get_location());
        f->collect_diags();
        ws_file_info.processor_group_found = &grp != &implicit_proc_grp;
        if (&grp == &implicit_proc_grp && (int64_t)f->diags().size() > get_config().diag_supress_limit)
        {
            ws_file_info.diagnostics_suppressed = true;
            delete_diags(f);
        }
        else
            diag_suppress_notified_[f->get_location()] = false;
    }

    // second check after all dependants are there to close all files that used to be dependencies
    for (const auto& f : files_to_parse)
        filter_and_close_dependencies_(f->files_to_close(), f);

    return ws_file_info;
}

void workspace::refresh_libraries()
{
    for (auto& proc_grp : proc_grps_)
    {
        for (auto& lib : proc_grp.second.libraries())
        {
            lib->refresh();
        }
    }
}

workspace_file_info workspace::did_open_file(const utils::resource::resource_location& file_location)
{
    if (!is_config_file(file_location))
        opened_files_.emplace(file_location);

    return parse_file(file_location);
}

void workspace::did_close_file(const utils::resource::resource_location& file_location)
{
    diag_suppress_notified_[file_location] = false;

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

void workspace::did_change_watched_files(const utils::resource::resource_location& file_location)
{
    refresh_libraries();
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

void workspace::open() { load_and_process_config(); }

void workspace::close() { opened_ = false; }

void workspace::set_message_consumer(message_consumer* consumer) { message_consumer_ = consumer; }

file_manager& workspace::get_file_manager() { return file_manager_; }

const processor_group& workspace::get_proc_grp(const proc_grp_id& proc_grp) const
{
    assert(opened_);
    return proc_grps_.at(proc_grp);
}

void replace_all(std::string& str, std::string_view what, std::string_view with)
{
    for (std::string::size_type p = 0; (p = str.find(what, p)) != std::string::npos; p += with.size())
        str.replace(p, what.size(), with);
}

bool starts_with(std::string_view s, std::string_view b) { return s.substr(0, b.size()) == b; }

std::regex pathmask_to_regex(std::string input)
{
    std::string r;
    r.reserve(input.size());

    replace_all(input, "\\", "/");
    if (input.back() != '/')
        input.push_back('/');

    std::string_view s = input;

    bool path_started = false;
    while (!s.empty())
    {
        switch (auto c = s.front())
        {
            case '*':
                if (starts_with(s, "**/"))
                {
                    if (path_started)
                    {
                        path_started = false;
                        r.append("[^/\\\\]*[/\\\\]");
                    }
                    r.append("(?:[^/\\\\]+[/\\\\])*");
                    s.remove_prefix(3);
                }
                else if (starts_with(s, "**"))
                {
                    r.append(".*");
                    s.remove_prefix(2);
                }
                else if (starts_with(s, "*/"))
                {
                    path_started = false;
                    r.append("[^/\\\\]*[/\\\\]");
                    s.remove_prefix(2);
                }
                else
                {
                    r.append("[^/\\\\]*");
                    s.remove_prefix(1);
                }
                break;

            case '/':
                path_started = false;
                r.append("[/\\\\]");
                s.remove_prefix(1);
                break;

            case '?':
            case '^':
            case '$':
            case '+':
            case '.':
            case '(':
            case ')':
            case '|':
            case '{':
            case '}':
            case '[':
            case ']':
                r.push_back('\\');
                [[fallthrough]];
            default:
                path_started = true;
                r.push_back(c);
                s.remove_prefix(1);
                break;
        }
    }

    return std::regex(r);
}

namespace {
std::string fix_path(std::string path)
{
    if (!path.empty() && path.back() != '/' && path.back() != '\\')
        path.push_back('/');
    return path;
};
} // namespace

void workspace::find_and_add_libs(
    std::string root, const std::string& path_pattern, processor_group& prc_grp, const library_local_options& opts)
{
    std::regex path_validator = pathmask_to_regex(path_pattern);

    std::set<std::string> processed_canonical_paths;
    std::deque<std::pair<std::string, std::string>> dirs_to_search;

    if (std::error_code ec; dirs_to_search.emplace_back(fix_path(root), utils::path::canonical(root, ec).string()), ec)
    {
        if (!opts.optional_library)
            add_diagnostic(diagnostic_s::error_L0001(root));
        return;
    }

    constexpr size_t limit = 1000;
    while (!dirs_to_search.empty())
    {
        if (processed_canonical_paths.size() > limit)
        {
            add_diagnostic(diagnostic_s::warning_L0005(path_pattern, limit));
            break;
        }
        auto [path, canonical_path] = std::move(dirs_to_search.front());
        dirs_to_search.pop_front();

        if (!processed_canonical_paths.insert(std::move(canonical_path)).second)
            continue;

        if (std::regex_match(path, path_validator))
            prc_grp.add_library(
                std::make_unique<library_local>(file_manager_, utils::resource::resource_location(path), opts));

        auto rc = utils::path::list_directory_subdirs_and_symlinks(
            path, [&processed_canonical_paths, &dirs_to_search](const auto& path) {
                std::error_code ec;
                auto cp = utils::path::canonical(path, ec).string();
                if (ec || processed_canonical_paths.count(cp))
                    return;
                if (utils::path::is_directory(cp))
                    dirs_to_search.emplace_back(fix_path(path.string()), std::move(cp));
            });
        if (rc != utils::path::list_directory_rc::done)
        {
            add_diagnostic(diagnostic_s::error_L0001(path));
            break;
        }
    }
}

namespace {
std::vector<std::string> get_macro_extensions_compatibility_list(const config::pgm_conf& pgm_config)
{
    // Extract extension list for compatibility reasons
    std::vector<std::string> macro_extensions_compatibility_list;
    for (const auto& wildcard : pgm_config.always_recognize)
    {
        std::string_view wc(wildcard);
        if (const auto ext_pattern = wc.rfind("*."); ext_pattern != std::string_view::npos)
        {
            wc.remove_prefix(ext_pattern + 1);
            macro_extensions_compatibility_list.push_back(std::string(wc));
        }
    }

    std::sort(macro_extensions_compatibility_list.begin(), macro_extensions_compatibility_list.end());

    macro_extensions_compatibility_list.erase(
        std::unique(macro_extensions_compatibility_list.begin(), macro_extensions_compatibility_list.end()),
        macro_extensions_compatibility_list.end());

    return macro_extensions_compatibility_list;
}
} // namespace

void workspace::process_processor_group(
    const config::processor_group& pg, const config::proc_grps& proc_groups, const config::pgm_conf& pgm_config)
{
    std::filesystem::path ws_path(location_.get_path());

    processor_group prc_grp(pg.name, pg.asm_options, pg.preprocessor);

    for (const auto& lib : pg.libs)
    {
        std::filesystem::path lib_path = [&path = lib.path]() {
            if (!path.empty())
                return utils::path::join(path, "");
            return std::filesystem::path {};
        }();
        if (!utils::path::is_absolute(lib_path))
            lib_path = utils::path::join(ws_path, lib_path);
        lib_path = utils::path::lexically_normal(lib_path);

        library_local_options opts;
        opts.optional_library = lib.optional;
        if (!lib.macro_extensions.empty())
            opts.extensions = lib.macro_extensions;
        else if (!proc_groups.macro_extensions.empty())
            opts.extensions = proc_groups.macro_extensions;
        else if (auto macro_extensions_compatibility_list = get_macro_extensions_compatibility_list(pgm_config);
                 !macro_extensions_compatibility_list.empty())
        {
            opts.extensions = macro_extensions_compatibility_list;
            opts.extensions_from_deprecated_source = true;
        }

        auto path_string = lib_path.string();
        if (auto asterisk = path_string.find('*'); asterisk == std::string::npos)
            prc_grp.add_library(std::make_unique<library_local>(
                file_manager_, utils::resource::resource_location(path_string), std::move(opts)));
        else
            find_and_add_libs(
                path_string.substr(0, path_string.find_last_of("/\\", asterisk)), path_string, prc_grp, opts);
    }

    add_proc_grp(std::move(prc_grp));
}

void workspace::process_program(const config::program_mapping& pgm, const file_ptr& pgm_conf_file)
{
    if (proc_grps_.find(pgm.pgroup) != proc_grps_.end())
    {
        std::string pgm_name = pgm.program;
        if (utils::platform::is_windows())
            std::replace(pgm_name.begin(), pgm_name.end(), '/', '\\');

        if (!is_wildcard(pgm_name))
            exact_pgm_conf_.emplace(pgm_name, program { pgm_name, pgm.pgroup, pgm.opts });
        else
            regex_pgm_conf_.push_back({ program { pgm_name, pgm.pgroup, pgm.opts }, wildcard2regex(pgm_name) });
    }
    else
    {
        config_diags_.push_back(diagnostic_s::error_W0004(pgm_conf_file->get_location().to_presentable(), name_));
    }
}

// open config files and parse them
bool workspace::load_and_process_config()
{
    config_diags_.clear();

    opened_ = true;

    config::pgm_conf pgm_config;
    config::proc_grps proc_groups;
    file_ptr proc_grps_file;
    file_ptr pgm_conf_file;

    bool load_ok = load_config(proc_groups, pgm_config, proc_grps_file, pgm_conf_file);
    if (!load_ok)
        return false;

    local_config_ = lib_config::load_from_pgm_config(pgm_config);

    // process processor groups
    for (auto& pg : proc_groups.pgroups)
    {
        process_processor_group(pg, proc_groups, pgm_config);
    }

    // process programs
    for (auto& pgm : pgm_config.pgms)
    {
        process_program(pgm, pgm_conf_file);
    }

    return true;
}
bool workspace::load_config(
    config::proc_grps& proc_groups, config::pgm_conf& pgm_config, file_ptr& proc_grps_file, file_ptr& pgm_conf_file)
{
    auto hlasm_base = utils::resource::resource_location::join(location_, HLASM_PLUGIN_FOLDER);

    // proc_grps.json parse
    proc_grps_file = file_manager_.add_file(utils::resource::resource_location::join(hlasm_base, FILENAME_PROC_GRPS));

    if (proc_grps_file->update_and_get_bad())
        return false;

    try
    {
        nlohmann::json::parse(proc_grps_file->get_text()).get_to(proc_groups);
        proc_grps_.clear();
        for (const auto& pg : proc_groups.pgroups)
        {
            if (!pg.asm_options.valid())
                config_diags_.push_back(diagnostic_s::error_W0005(
                    proc_grps_file->get_location().to_presentable(), pg.name, "processor group"));
            if (!pg.preprocessor.valid())
                config_diags_.push_back(
                    diagnostic_s::error_W0006(proc_grps_file->get_location().to_presentable(), pg.name));
        }
    }
    catch (const nlohmann::json::exception&)
    {
        // could not load proc_grps
        config_diags_.push_back(diagnostic_s::error_W0002(proc_grps_file->get_location().to_presentable(), name_));
        return false;
    }

    // pgm_conf.json parse
    pgm_conf_file = file_manager_.add_file(utils::resource::resource_location::join(hlasm_base, FILENAME_PGM_CONF));

    if (pgm_conf_file->update_and_get_bad())
        return false;

    try
    {
        nlohmann::json::parse(pgm_conf_file->get_text()).get_to(pgm_config);
        for (const auto& pgm : pgm_config.pgms)
        {
            if (!pgm.opts.valid())
                config_diags_.push_back(
                    diagnostic_s::error_W0005(pgm_conf_file->get_location().to_presentable(), pgm.program, "program"));
        }
        exact_pgm_conf_.clear();
        regex_pgm_conf_.clear();
    }
    catch (const nlohmann::json::exception&)
    {
        config_diags_.push_back(diagnostic_s::error_W0003(pgm_conf_file->get_location().to_presentable(), name_));
        return false;
    }

    return true;
}
bool workspace::is_wildcard(const std::string& str)
{
    return str.find('*') != std::string::npos || str.find('?') != std::string::npos;
}

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

    auto pgm = get_program(file_location);
    if (pgm)
    {
        get_proc_grp_by_program(*pgm).update_asm_options(result);
        pgm->asm_opts.apply(result);
    }
    else
    {
        implicit_proc_grp.update_asm_options(result);
    }

    return result;
}

preprocessor_options workspace::get_preprocessor_options(const utils::resource::resource_location& file_location) const
{
    auto& proc_grp = get_proc_grp_by_program(file_location);

    return proc_grp.preprocessor();
}

processor_file_ptr workspace::get_processor_file(const utils::resource::resource_location& file_location)
{
    return get_file_manager().get_processor_file(file_location);
}

} // namespace hlasm_plugin::parser_library::workspaces
