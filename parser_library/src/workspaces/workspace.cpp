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
#include <iostream>
#include <regex>
#include <string>

#include "lib_config.h"
#include "processor.h"
#include "utils/path.h"
#include "utils/platform.h"
#include "wildcard.h"

using json = nlohmann::json;

namespace hlasm_plugin::parser_library::workspaces {

workspace::workspace(const ws_uri& uri,
    const std::string& name,
    file_manager& file_manager,
    const lib_config& global_config,
    std::atomic<bool>* cancel)
    : cancel_(cancel)
    , name_(name)
    , uri_(uri)
    , file_manager_(file_manager)
    , implicit_proc_grp("pg_implicit")
    , ws_path_(uri)
    , global_config_(global_config)
{
    auto hlasm_folder = utils::path::join(ws_path_, HLASM_PLUGIN_FOLDER);
    proc_grps_path_ = utils::path::join(hlasm_folder, FILENAME_PROC_GRPS);
    pgm_conf_path_ = utils::path::join(hlasm_folder, FILENAME_PGM_CONF);
}

workspace::workspace(
    const ws_uri& uri, file_manager& file_manager, const lib_config& global_config, std::atomic<bool>* cancel)
    : workspace(uri, uri, file_manager, global_config, cancel)
{}

workspace::workspace(file_manager& file_manager, const lib_config& global_config, std::atomic<bool>* cancel)
    : workspace("", file_manager, global_config, cancel)
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

std::vector<processor_file_ptr> workspace::find_related_opencodes(const std::string& document_uri) const
{
    std::vector<processor_file_ptr> opencodes;

    for (const auto& dep : dependants_)
    {
        auto f = file_manager_.find_processor_file(dep);
        if (!f)
            continue;
        if (f->dependencies().find(document_uri) != f->dependencies().end())
            opencodes.push_back(std::move(f));
    }

    if (opencodes.size())
        return opencodes;

    if (auto f = file_manager_.find_processor_file(document_uri))
        return { f };
    return {};
}

void workspace::delete_diags(processor_file_ptr file)
{
    file->diags().clear();

    for (const std::string& fname : file->dependencies())
    {
        auto dep_file = file_manager_.find_processor_file(fname);
        if (dep_file)
            dep_file->diags().clear();
    }

    auto notified_found = diag_suppress_notified_.emplace(file->get_file_name(), false);
    if (!notified_found.first->second)
        show_message("Diagnostics suppressed from " + file->get_file_name() + ", because there is no configuration.");
    notified_found.first->second = true;
}

void workspace::show_message(const std::string& message)
{
    if (message_consumer_)
        message_consumer_->show_message(message, message_type::MT_INFO);
}

lib_config workspace::get_config() { return local_config_.fill_missing_settings(global_config_); }

const processor_group& workspace::get_proc_grp_by_program(const std::string& filename) const
{
    assert(opened_);

    std::string file = utils::path::lexically_normal(utils::path::lexically_relative(filename, uri_)).string();

    // direct match
    auto program = exact_pgm_conf_.find(file);
    if (program != exact_pgm_conf_.cend())
        return proc_grps_.at(program->second.pgroup);

    for (const auto& pgm : regex_pgm_conf_)
    {
        if (std::regex_match(file, pgm.second))
            return proc_grps_.at(pgm.first.pgroup);
    }
    return implicit_proc_grp;
}

const ws_uri& workspace::uri() { return uri_; }

void workspace::parse_file(const std::string& file_uri)
{
    std::filesystem::path file_path(file_uri);
    // add support for hlasm to vscode (auto detection??) and do the decision based on languageid
    if (utils::path::equal(file_path, proc_grps_path_) || utils::path::equal(file_path, pgm_conf_path_))
    {
        if (load_and_process_config())
        {
            for (auto fname : dependants_)
            {
                auto found = file_manager_.find_processor_file(fname);
                if (found)
                    found->parse(*this);
            }

            for (auto fname : dependants_)
            {
                auto found = file_manager_.find_processor_file(fname);
                if (found)
                    filter_and_close_dependencies_(found->files_to_close(), found);
            }
        }
        return;
    }
    // what about removing files??? what if depentands_ points to not existing file?
    std::vector<processor_file_ptr> files_to_parse;

    for (auto fname : dependants_)
    {
        auto f = file_manager_.find_processor_file(fname);
        if (!f)
            continue;
        for (auto& name : f->dependencies())
        {
            if (name == file_uri)
            {
                files_to_parse.push_back(f);
                break;
            }
        }
    }

    if (files_to_parse.empty())
    {
        auto f = file_manager_.find_processor_file(file_uri);
        if (f)
            files_to_parse.push_back(f);
    }

    for (auto f : files_to_parse)
    {
        f->parse(*this);
        if (!f->dependencies().empty())
            dependants_.insert(f->get_file_name());


        // if there is no processor group assigned to the program, delete diagnostics that may have been created
        if (cancel_ && cancel_->load()) // skip, if parsing was cancelled using the cancellation token
            continue;

        const processor_group& grp = get_proc_grp_by_program(f->get_file_name());
        f->collect_diags();
        if (&grp == &implicit_proc_grp && (int64_t)f->diags().size() > get_config().diag_supress_limit)
            delete_diags(f);
        else
            diag_suppress_notified_[f->get_file_name()] = false;
    }

    // second check after all dependants are there to close all files that used to be dependencies
    for (auto f : files_to_parse)
        filter_and_close_dependencies_(f->files_to_close(), f);
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

void workspace::did_open_file(const std::string& file_uri) { parse_file(file_uri); }

void workspace::did_close_file(const std::string& file_uri)
{
    diag_suppress_notified_[file_uri] = false;
    // first check whether the file is a dependency
    // if so, simply close it, no other action is needed
    if (is_dependency_(file_uri))
    {
        file_manager_.did_close_file(file_uri);
        return;
    }

    // find if the file is a dependant
    auto fname = dependants_.find(file_uri);
    if (fname != dependants_.end())
    {
        auto file = file_manager_.find_processor_file(*fname);
        // filter the dependencies that should not be closed
        filter_and_close_dependencies_(file->dependencies(), file);
        // remove it from dependants
        dependants_.erase(fname);
    }

    // close the file itself
    file_manager_.did_close_file(file_uri);
    file_manager_.remove_file(file_uri);
}

void workspace::did_change_file(const std::string file_uri, const document_change*, size_t) { parse_file(file_uri); }

void workspace::did_change_watched_files(const std::string& file_uri)
{
    refresh_libraries();
    parse_file(file_uri);
}

location workspace::definition(const std::string& document_uri, const position pos) const
{
    auto opencodes = find_related_opencodes(document_uri);
    if (opencodes.empty())
        return { pos, document_uri };
    // for now take last opencode
    return opencodes.back()->get_lsp_feature_provider().definition(document_uri, pos);
}

location_list workspace::references(const std::string& document_uri, const position pos) const
{
    auto opencodes = find_related_opencodes(document_uri);
    if (opencodes.empty())
        return {};
    // for now take last opencode
    return opencodes.back()->get_lsp_feature_provider().references(document_uri, pos);
}

lsp::hover_result workspace::hover(const std::string& document_uri, const position pos) const
{
    auto opencodes = find_related_opencodes(document_uri);
    if (opencodes.empty())
        return {};
    // for now take last opencode
    return opencodes.back()->get_lsp_feature_provider().hover(document_uri, pos);
}

lsp::completion_list_s workspace::completion(const std::string& document_uri,
    const position pos,
    const char trigger_char,
    completion_trigger_kind trigger_kind) const
{
    auto opencodes = find_related_opencodes(document_uri);
    if (opencodes.empty())
        return {};
    // for now take last opencode
    return opencodes.back()->get_lsp_feature_provider().completion(document_uri, pos, trigger_char, trigger_kind);
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

// open config files and parse them
bool workspace::load_and_process_config()
{
    std::filesystem::path ws_path(uri_);

    config_diags_.clear();

    opened_ = true;

    json pgm_conf_json;
    json proc_grps_json;
    file_ptr pgm_conf_file;

    bool load_ok = load_config(proc_grps_json, pgm_conf_json, pgm_conf_file);
    if (!load_ok)
        return false;

    // get extensions from pgm conf
    extension_regex_map extensions;
    std::regex extension_regex("^(.*\\*)(\\.\\w+)$");
    json wildcards = pgm_conf_json["alwaysRecognize"];
    for (const auto& wildcard : wildcards)
    {
        std::string wildcard_str = wildcard.get<std::string>();
        // extension wildcard
        if (std::regex_match(wildcard_str, extension_regex))
            extensions.insert({ std::regex_replace(wildcard_str, extension_regex, "$2"),
                wildcard2regex(utils::path::join(ws_path, wildcard_str).string()) });
    }

    /*auto suppress_diags_limit_json = pgm_conf_json.find("diagnosticsSuppressLimit");
    if (suppress_diags_limit_json != pgm_conf_json.end())
    {
        if (suppress_diags_limit_json->is_number_integer())
            new_config.diag_supress_limit = suppress_diags_limit_json->get<int64_t>();
    }*/
    local_config_ = lib_config::load_from_json(pgm_conf_json);

    auto extensions_ptr = std::make_shared<const extension_regex_map>(std::move(extensions));
    // process processor groups
    json pgs = proc_grps_json["pgroups"];
    for (auto& pg : pgs)
    {
        const std::string& name = pg["name"].get<std::string>();
        const json& libs = pg["libs"];
        std::map<std::string, std::string> asm_options;
        try
        {
            if (pg.count("asm_options"))
            {
                asm_options = pg.at("asm_options").get<std::map<std::string, std::string>>();
            }
        }
        catch (const nlohmann::basic_json<>::exception&)
        {
            file_ptr proc_grps_file =
                file_manager_.add_file((ws_path / HLASM_PLUGIN_FOLDER / FILENAME_PROC_GRPS).string());
            config_diags_.push_back(diagnostic_s::error_W002(proc_grps_file->get_file_name(), name_));
        }
        processor_group prc_grp(name);

        for (auto& lib_path_json : libs)
        {
            bool optional = false;
            std::string path;
            bool valid = false;

            if (lib_path_json.is_string())
            {
                // the added '/' will ensure the path always ends with directory separator
                path = lib_path_json.get<std::string>();
                valid = true;
            }
            else if (lib_path_json.is_object() && lib_path_json.count("path"))
            {
                if (auto pp = lib_path_json.find("path"); pp != lib_path_json.cend() && pp->is_string())
                {
                    path = pp->get<std::string>();
                    if (auto op = lib_path_json.find("optional"); op != lib_path_json.cend())
                    {
                        if (op->is_boolean())
                        {
                            optional = op->get<bool>();
                            valid = true;
                        }
                    }
                    else
                        valid = true;
                }
            }

            if (valid)
            {
                std::filesystem::path lib_path = [&path]() {
                    if (!path.empty())
                        return utils::path::join(std::move(path), "");
                    return std::filesystem::path {};
                }();

                if (utils::path::is_absolute(lib_path))
                    prc_grp.add_library(std::make_unique<library_local>(
                        file_manager_, utils::path::lexically_normal(lib_path).string(), extensions_ptr, optional));
                else
                    prc_grp.add_library(std::make_unique<library_local>(file_manager_,
                        utils::path::lexically_normal(utils::path::join(ws_path, lib_path)).string(),
                        extensions_ptr,
                        optional));
                // else ignore, publish warning
            }
        }
        if (!asm_options.empty())
        {
            prc_grp.add_asm_options(asm_options);
        }

        add_proc_grp(std::move(prc_grp));
    }

    // process programs
    json pgms = pgm_conf_json["pgms"];
    for (auto& pgm : pgms)
    {
        std::string pgm_name = pgm["program"].get<std::string>();
        const std::string& pgroup = pgm["pgroup"].get<std::string>();

        if (proc_grps_.find(pgroup) != proc_grps_.end())
        {
            if (utils::platform::is_windows())
            {
                // change of forward slash to double backslash on windows
                static const std::regex slash("\\/");
                pgm_name = std::regex_replace(pgm_name, slash, "\\");
            }
            if (!is_wildcard(pgm_name))
                exact_pgm_conf_.emplace(pgm_name, program { pgm_name, pgroup });
            else
                regex_pgm_conf_.push_back({ program { pgm_name, pgroup }, wildcard2regex(pgm_name) });
        }
        else
        {
            config_diags_.push_back(diagnostic_s::error_W004(pgm_conf_file->get_file_name(), name_));
        }
    }

    return true;
}
bool workspace::load_config(nlohmann::json& proc_grps_json, nlohmann::json& pgm_conf_json, file_ptr& pgm_conf_file)
{
    std::filesystem::path hlasm_base = utils::path::join(uri_, HLASM_PLUGIN_FOLDER);

    // proc_grps.json parse
    file_ptr proc_grps_file = file_manager_.add_file(utils::path::join(hlasm_base, FILENAME_PROC_GRPS).string());

    if (proc_grps_file->update_and_get_bad())
        return false;

    try
    {
        proc_grps_json = nlohmann::json::parse(proc_grps_file->get_text());
        proc_grps_.clear();
    }
    catch (const nlohmann::json::exception&)
    {
        // could not load proc_grps
        config_diags_.push_back(diagnostic_s::error_W002(proc_grps_file->get_file_name(), name_));
        return false;
    }

    // pgm_conf.json parse
    pgm_conf_file = file_manager_.add_file(utils::path::join(hlasm_base, FILENAME_PGM_CONF).string());

    if (pgm_conf_file->update_and_get_bad())
        return false;

    try
    {
        pgm_conf_json = nlohmann::json::parse(pgm_conf_file->get_text());
        exact_pgm_conf_.clear();
        regex_pgm_conf_.clear();
    }
    catch (const nlohmann::json::exception&)
    {
        config_diags_.push_back(diagnostic_s::error_W003(pgm_conf_file->get_file_name(), name_));
        return false;
    }

    return true;
}
bool workspace::is_wildcard(const std::string& str)
{
    return str.find('*') != std::string::npos || str.find('?') != std::string::npos;
}

void workspace::filter_and_close_dependencies_(const std::set<std::string>& dependencies, processor_file_ptr file)
{
    std::set<std::string> filtered;
    // filters out externally open files
    for (auto& dependency : dependencies)
    {
        auto dependency_file = file_manager_.find_processor_file(dependency);
        if (dependency_file && !dependency_file->get_lsp_editing())
            filtered.insert(dependency);
    }

    // filters the files that are dependencies of other dependants and externally open files
    for (auto dependant : dependants_)
    {
        auto fdependant = file_manager_.find_processor_file(dependant);
        if (!fdependant)
            continue;
        for (auto& dependency : fdependant->dependencies())
        {
            if (fdependant->get_file_name() != file->get_file_name() && filtered.find(dependency) != filtered.end())
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

bool workspace::is_dependency_(const std::string& file_uri)
{
    for (auto dependant : dependants_)
    {
        auto fdependant = file_manager_.find_processor_file(dependant);
        if (!fdependant)
            continue;
        for (auto& dependency : fdependant->dependencies())
        {
            if (dependency == file_uri)
                return true;
        }
    }
    return false;
}

parse_result workspace::parse_library(const std::string& library, analyzing_context ctx, const library_data data)
{
    auto& proc_grp = get_proc_grp_by_program(ctx.hlasm_ctx->opencode_file_name());
    for (auto&& lib : proc_grp.libraries())
    {
        std::shared_ptr<processor> found = lib->find_file(library);
        if (found)
            return found->parse_macro(*this, std::move(ctx), data);
    }

    return false;
}

bool workspace::has_library(const std::string& library, const std::string& program) const
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

const asm_option& workspace::get_asm_options(const std::string& file_name)
{
    auto& proc_grp = get_proc_grp_by_program(file_name);

    return proc_grp.asm_options();
}

processor_file_ptr workspace::get_processor_file(const std::string& filename)
{
    return get_file_manager().get_processor_file(filename);
}

} // namespace hlasm_plugin::parser_library::workspaces
