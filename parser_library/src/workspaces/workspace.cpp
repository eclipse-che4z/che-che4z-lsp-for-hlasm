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

#include "context/instruction.h"
#include "file_manager.h"
#include "lsp/item_convertors.h"
#include "utils/bk_tree.h"
#include "utils/levenshtein_distance.h"
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
        auto found = file_manager_.find_processor_file(fname);
        if (!found)
            continue;
        details.alternative_config = m_configuration.load_alternative_config_if_needed(fname);
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

namespace {
bool trigger_reparse(const utils::resource::resource_location& file_location)
{
    return !file_location.get_uri().starts_with("hlasm:");
}
} // namespace

std::vector<processor_file_ptr> workspace::collect_dependants(
    const utils::resource::resource_location& file_location) const
{
    std::vector<processor_file_ptr> result;

    for (const auto& dep : dependants_)
    {
        auto f = file_manager_.find_processor_file(dep);
        if (!f)
            continue;
        for (auto& dep_location : f->dependencies())
        {
            if (dep_location == file_location)
            {
                result.push_back(f);
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
    std::vector<processor_file_ptr> files_to_parse;

    // TODO: apparently just opening a file without changing it triggers reparse

    if (auto this_file = file_manager_.find_processor_file(file_location);
        file_content_status == open_file_result::changed_content
        || file_content_status == open_file_result::changed_lsp && !(this_file && this_file->has_lsp_info()))
    {
        if (trigger_reparse(file_location))
            files_to_parse = collect_dependants(file_location);

        if (files_to_parse.empty() && this_file)
            files_to_parse.push_back(std::move(this_file));

        for (const auto& f : files_to_parse)
        {
            const auto& f_loc = f->get_location();

            auto alt_cfg = m_configuration.load_alternative_config_if_needed(f_loc);
            if (auto opened_it = opened_files_.find(f_loc); opened_it != opened_files_.end())
                opened_it->second.alternative_config = std::move(alt_cfg);

            if (!f->parse(*this, get_asm_options(f_loc), get_preprocessor_options(f_loc), &fm_vfm_))
                continue;

            ws_file_info = parse_successful(f);
        }

        // second check after all dependants are there to close all files that used to be dependencies
        for (const auto& f : files_to_parse)
            filter_and_close_dependencies_(f->files_to_close(), f);
    }

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

    return generate_completion(lsp_context->completion(document_loc, pos, trigger_char, trigger_kind),
        [&document_loc, this](std::string_view opcode) {
            auto suggestions = make_opcode_suggestion(document_loc, opcode, true);
            std::vector<std::string> result;
            std::transform(suggestions.begin(), suggestions.end(), std::back_inserter(result), [](auto& e) {
                return std::move(e.first);
            });
            return result;
        });
}

lsp::completion_list_s workspace::generate_completion(const lsp::completion_list_source& cls,
    std::function<std::vector<std::string>(std::string_view)> instruction_suggestions)
{
    return std::visit(
        [&instruction_suggestions](auto v) { return generate_completion(v, instruction_suggestions); }, cls);
}

lsp::completion_list_s workspace::generate_completion(
    std::monostate, const std::function<std::vector<std::string>(std::string_view)>&)
{
    return lsp::completion_list_s();
}

lsp::completion_list_s workspace::generate_completion(
    const lsp::vardef_storage* var_defs, const std::function<std::vector<std::string>(std::string_view)>&)
{
    lsp::completion_list_s items;
    for (const auto& vardef : *var_defs)
    {
        items.emplace_back(lsp::generate_completion_item(vardef));
    }

    return items;
}

lsp::completion_list_s workspace::generate_completion(
    const context::label_storage* seq_syms, const std::function<std::vector<std::string>(std::string_view)>&)
{
    lsp::completion_list_s items;
    items.reserve(seq_syms->size());
    for (const auto& [_, sym] : *seq_syms)
    {
        items.emplace_back(lsp::generate_completion_item(*sym));
    }
    return items;
}

lsp::completion_list_s workspace::generate_completion(const lsp::completion_list_instructions& cli,
    const std::function<std::vector<std::string>(std::string_view)>& instruction_suggestions)
{
    lsp::completion_list_s result;

    assert(cli.lsp_ctx);

    const auto& hlasm_ctx = cli.lsp_ctx->get_related_hlasm_context();

    auto suggestions = !cli.completed_text.empty() && instruction_suggestions
        ? instruction_suggestions(cli.completed_text)
        : std::vector<std::string>();

    // Store only instructions from the currently active instruction set
    for (const auto& instr : lsp::completion_item_s::m_instruction_completion_items)
    {
        auto id = hlasm_ctx.ids().find(instr.label);
        // TODO: we could provide more precise results here if actual generation is provided
        if (id.has_value() && hlasm_ctx.find_opcode_mnemo(id.value(), context::opcode_generation::zero))
        {
            auto& i = result.emplace_back(instr);
            if (auto space = i.insert_text.find(' '); space != std::string::npos)
            {
                if (auto col_pos = cli.completed_text_start_column + space; col_pos < 15)
                    i.insert_text.insert(i.insert_text.begin() + space, 15 - col_pos, ' ');
            }
            if (std::find(suggestions.begin(), suggestions.end(), i.label) != suggestions.end())
            {
                i.suggestion_for = cli.completed_text;
            }
        }
    }

    for (const auto& [_, macro_i] : *cli.macros)
    {
        auto& i = result.emplace_back(lsp::generate_completion_item(
            *macro_i, cli.lsp_ctx->get_file_info(macro_i->definition_location.resource_loc)));
        if (std::find(suggestions.begin(), suggestions.end(), i.label) != suggestions.end())
        {
            i.suggestion_for = cli.completed_text;
        }
    }

    return result;
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

namespace {
auto generate_instruction_bk_tree(instruction_set_version version)
{
    utils::bk_tree<std::string_view, utils::levenshtein_distance_t<16>> result;

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
        m_configuration.get_proc_grp_by_program(*pgm).apply_options_to(result);
        pgm->asm_opts.apply_options_to(result);
    }
    else
    {
        implicit_proc_grp.apply_options_to(result);
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
