/*
 * Copyright (c) 2021 Broadcom.
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

#include "macro_cache.h"

#include <array>

#include "analyzer.h"
#include "context/hlasm_context.h"
#include "context/special_instructions.h"
#include "file_manager.h"
#include "lsp/lsp_context.h"

namespace hlasm_plugin::parser_library::workspaces {

macro_cache::macro_cache(const file_manager& file_mngr, std::shared_ptr<file> macro_file)
    : file_mngr_(&file_mngr)
    , macro_file_(std::move(macro_file))
{}

std::vector<cached_opsyn_mnemo> macro_cache_key::get_opsyn_state(context::hlasm_context& ctx)
{
    std::vector<cached_opsyn_mnemo> result;

    context::id_index last_from;
    auto& opcodes = ctx.opcode_mnemo_storage();
    for (auto it = opcodes.rbegin(); it != opcodes.rend(); ++it)
    {
        const auto& [from_pair, opcode] = *it;
        const auto& [from, gen] = from_pair;

        // ignore historical OPSYNs and original instructions
        if (from == last_from || gen == context::opcode_generation::zero)
            continue;

        // If there is an opsyn, that aliases an instruction to be CA instruction, add it to result
        if (context::instruction_resolved_during_macro_parsing(opcode.opcode)
            || context::instruction_resolved_during_macro_parsing(from))
            result.push_back(
                { from, opcode.opcode, std::holds_alternative<context::macro_def_ptr>(opcode.opcode_detail) });

        last_from = from;
    }

    sort_opsyn_state(result);

    return result;
}

macro_cache_key macro_cache_key::create_from_context(context::hlasm_context& hlasm_ctx, library_data data)
{
    return { comparable_weak_ptr(hlasm_ctx.ids_ptr()), data, get_opsyn_state(hlasm_ctx) };
}

void macro_cache_key::sort_opsyn_state(std::vector<cached_opsyn_mnemo>& opsyn_state)
{
    std::sort(opsyn_state.begin(), opsyn_state.end(), [](const cached_opsyn_mnemo& lhs, const cached_opsyn_mnemo& rhs) {
        return std::tie(lhs.from_instr, lhs.to_instr, lhs.is_macro)
            < std::tie(rhs.from_instr, rhs.to_instr, rhs.is_macro);
    });
}

const macro_cache_data* macro_cache::find_cached_data(const macro_cache_key& key) const
{
    auto it = cache_.find(key);
    if (it == cache_.end())
        return nullptr;

    const auto& cached_data = it->second;

    for (const auto& [fname, cached_version] : cached_data.stamps)
    {
        auto file = file_mngr_->find(fname);
        if (!file)
            return nullptr; // Reparse needed
        if (file->get_version() != cached_version)
            return nullptr;
    }

    // Version of all dependent files are the same.
    return &cached_data;
}

bool macro_cache::load_from_cache(const macro_cache_key& key, const analyzing_context& ctx) const
{
    if (auto cached_data = find_cached_data(key))
    {
        if (key.data.proc_kind == processing::processing_kind::MACRO)
        {
            lsp::macro_info_ptr info = std::get<lsp::macro_info_ptr>(cached_data->cached_member);
            if (!info)
                return true; // The file for which the analyzer is cached does not contain definition of macro
            ctx.hlasm_ctx->add_macro(info->macro_definition);
            ctx.lsp_ctx->add_macro(info, lsp::text_data_view(macro_file_->get_text()));

            // Add all copy members on which this macro is dependant
            for (const auto& copy_ptr : info->macro_definition->used_copy_members)
            {
                auto file = file_mngr_->find(copy_ptr->definition_location.resource_loc);
                ctx.hlasm_ctx->add_copy_member(copy_ptr);
                ctx.lsp_ctx->add_copy(copy_ptr, lsp::text_data_view(file->get_text()));
            }
        }
        else if (key.data.proc_kind == processing::processing_kind::COPY)
        {
            auto copy_member = std::get<context::copy_member_ptr>(cached_data->cached_member);
            ctx.hlasm_ctx->add_copy_member(copy_member);
            ctx.lsp_ctx->add_copy(copy_member, lsp::text_data_view(macro_file_->get_text()));
        }

        return true;
    }
    return false;
}

version_stamp macro_cache::get_copy_member_versions(context::macro_def_ptr macro) const
{
    version_stamp result;

    for (const auto& copy_ptr : macro->used_copy_members)
    {
        auto file = file_mngr_->find(copy_ptr->definition_location.resource_loc);
        if (!file)
            throw std::runtime_error("Dependencies of a macro must be open right after parsing the macro.");
        result.try_emplace(file->get_location(), file->get_version());
    }
    return result;
}

void macro_cache::save_macro(const macro_cache_key& key, const analyzer& analyzer)
{
    auto& cache_data = cache_[key];
    if (key.data.proc_kind == processing::processing_kind::MACRO)
    {
        // Add stamps for all macro dependencies
        auto parsed_macro = analyzer.context().hlasm_ctx->get_macro_definition(key.data.library_member);
        if (parsed_macro)
            cache_data.stamps = get_copy_member_versions(std::move(parsed_macro));
        else
            cache_data.stamps.clear();
    }
    else // Copy members do not have additional dependencies
        cache_data.stamps.clear();

    cache_data.stamps.try_emplace(macro_file_->get_location(), macro_file_->get_version());
    if (key.data.proc_kind == processing::processing_kind::MACRO)
        cache_data.cached_member =
            analyzer.context().lsp_ctx->get_macro_info(key.data.library_member, context::opcode_generation::current);
    else if (key.data.proc_kind == processing::processing_kind::COPY)
        cache_data.cached_member = analyzer.context().hlasm_ctx->get_copy_member(key.data.library_member);
}

void macro_cache::erase_unused()
{
    std::erase_if(cache_, [](const auto& e) { return e.first.related_open_code.expired(); });
}

} // namespace hlasm_plugin::parser_library::workspaces
