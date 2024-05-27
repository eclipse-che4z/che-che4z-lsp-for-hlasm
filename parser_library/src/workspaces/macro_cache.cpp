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
#include "file.h"
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

    for (const auto& [from, versions] : ctx.opcode_mnemo_storage())
    {
        const auto& [opcode, gen] = versions.back();
        if (gen == context::opcode_generation::zero)
            continue;

        // If there is an opsyn, that aliases an instruction to be CA instruction, add it to result
        if (context::instruction_resolved_during_macro_parsing(opcode.opcode)
            || context::instruction_resolved_during_macro_parsing(from))
            result.emplace_back(from, opcode.opcode, opcode.is_macro());
    }

    sort_opsyn_state(result);

    return result;
}

macro_cache_key macro_cache_key::create_from_context(
    context::hlasm_context& hlasm_ctx, processing::processing_kind kind, context::id_index name)
{
    return { kind, name, get_opsyn_state(hlasm_ctx) };
}

void macro_cache_key::sort_opsyn_state(std::vector<cached_opsyn_mnemo>& opsyn_state)
{
    std::ranges::sort(opsyn_state, {}, [](const auto& e) { return std::tie(e.from_instr, e.to_instr, e.is_macro); });
}

const macro_cache_data* macro_cache::find_cached_data(const macro_cache_key& key) const
{
    auto it = cache_.find(key);
    if (it == cache_.end())
        return nullptr;

    const auto& cached_data = it->second;

    for (const auto& [fname, cached_version] : cached_data.stamps)
    {
        if (auto file = file_mngr_->find(fname); !file || file->get_version() != cached_version)
        {
            return nullptr; // Reparse needed
        }
    }

    // Version of all dependent files are the same.
    return &cached_data;
}

std::optional<std::vector<std::shared_ptr<file>>> macro_cache::load_from_cache(
    const macro_cache_key& key, const analyzing_context& ctx) const
{
    std::optional<std::vector<std::shared_ptr<file>>> result;
    if (auto cached_data = find_cached_data(key))
    {
        auto& locs = result.emplace();
        if (key.kind == processing::processing_kind::MACRO)
        {
            lsp::macro_info_ptr info = std::get<lsp::macro_info_ptr>(cached_data->cached_member);
            if (!info)
                return result; // The file for which the analyzer is cached does not contain definition of macro
            ctx.hlasm_ctx->add_macro(info->macro_definition, info->external);
            ctx.lsp_ctx->add_macro(info, lsp::text_data_view(macro_file_->get_text()));

            // Add all copy members on which this macro is dependant
            for (const auto& copy_ptr : info->macro_definition->used_copy_members)
            {
                const auto& file = locs.emplace_back(file_mngr_->find(copy_ptr->definition_location.resource_loc));
                ctx.hlasm_ctx->add_copy_member(copy_ptr);
                ctx.lsp_ctx->add_copy(copy_ptr, lsp::text_data_view(file->get_text()));
            }
        }
        else if (key.kind == processing::processing_kind::COPY)
        {
            const auto& copy_member = std::get<context::copy_member_ptr>(cached_data->cached_member);
            ctx.hlasm_ctx->add_copy_member(copy_member);
            ctx.lsp_ctx->add_copy(copy_member, lsp::text_data_view(macro_file_->get_text()));
        }
    }
    return result;
}

version_stamp macro_cache::get_copy_member_versions(context::macro_definition& macro) const
{
    version_stamp result;

    for (const auto& copy_ptr : macro.used_copy_members)
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
    if (key.kind == processing::processing_kind::MACRO)
    {
        // Add stamps for all macro dependencies
        auto parsed_macro = analyzer.context().hlasm_ctx->get_macro_definition(key.name);
        if (parsed_macro)
            cache_data.stamps = get_copy_member_versions(*parsed_macro);
        else
            cache_data.stamps.clear();
    }
    else // Copy members do not have additional dependencies
        cache_data.stamps.clear();

    cache_data.stamps.try_emplace(macro_file_->get_location(), macro_file_->get_version());
    if (key.kind == processing::processing_kind::MACRO)
        cache_data.cached_member =
            analyzer.context().lsp_ctx->get_macro_info(key.name, context::opcode_generation::current);
    else if (key.kind == processing::processing_kind::COPY)
        cache_data.cached_member = analyzer.context().hlasm_ctx->get_copy_member(key.name);
}

} // namespace hlasm_plugin::parser_library::workspaces
