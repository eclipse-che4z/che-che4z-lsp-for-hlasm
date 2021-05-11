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

#include "file_manager.h"

namespace hlasm_plugin::parser_library::workspaces {

void macro_cache::collect_diags() const {}

macro_cache::macro_cache(const file_manager& file_mngr, file& macro_file)
    : file_mngr_(&file_mngr)
    , macro_file_(&macro_file)
{}

std::vector<cached_opsyn_mnemo> macro_cache_key::get_opsyn_state(context::hlasm_context& ctx)
{
    auto& wn = ctx.ids().well_known;
    // List of instructions that are resolved during macro definition - therefore are affected by OPSYN
    std::array<context::id_index, 19> cached_instr { wn.COPY,
        wn.ASPACE,
        wn.GBLA,
        wn.GBLB,
        wn.GBLC,
        wn.LCLA,
        wn.LCLB,
        wn.LCLC,
        wn.SETA,
        wn.SETB,
        wn.SETC,
        ctx.ids().add("AIF"),
        ctx.ids().add("MEND"),
        ctx.ids().add("MACRO"),
        ctx.ids().add("MEXIT"),
        ctx.ids().add("AIF"),
        ctx.ids().add("AREAD"),
        ctx.ids().add("ACTR"),
        ctx.ids().add("AGO") };

    std::vector<cached_opsyn_mnemo> result;

    for (const auto& [from, opcode] : ctx.opcode_mnemo_storage())
    {
        // If there is an opsyn, that aliases an instruction to be CA instruction, add it to result
        if (std::find(cached_instr.begin(), cached_instr.end(), opcode.opcode) != cached_instr.end())
            result.push_back(
                { from, opcode.opcode, std::holds_alternative<context::macro_def_ptr>(opcode.opcode_detail) });

        if (std::find(cached_instr.begin(), cached_instr.end(), from) != cached_instr.end())
            result.push_back(
                { from, opcode.opcode, std::holds_alternative<context::macro_def_ptr>(opcode.opcode_detail) });
    }

    // Also macros with the same name as CA instructions may alias the instructions
    for (const auto& [id, macro] : ctx.macros())
    {
        if (!macro)
            continue;
        auto opcode = ctx.get_operation_code(id);
        // Macros for which there is an opsyn in effect are already captured
        if (auto pval = std::get_if<context::macro_def_ptr>(&opcode.opcode_detail); !pval || *pval != macro)
            continue;

        // If there is an opsyn, that aliases an instruction to be CA instruction, add it to result
        if (std::find(cached_instr.begin(), cached_instr.end(), macro->id) != cached_instr.end())
            result.push_back({ id, id, true });
    }

    sort_opsyn_state(result);

    return result;
}

macro_cache_key macro_cache_key::create_from_context(context::hlasm_context& hlasm_ctx, library_data data)
{
    return { data, get_opsyn_state(hlasm_ctx) };
}

void macro_cache_key::sort_opsyn_state(std::vector<cached_opsyn_mnemo>& opsyn_state)
{
    std::sort(opsyn_state.begin(), opsyn_state.end(), [](const cached_opsyn_mnemo& lhs, const cached_opsyn_mnemo& rhs) {
        return std::tie(lhs.from_instr, lhs.to_instr, lhs.is_macro)
            < std::tie(rhs.from_instr, rhs.to_instr, rhs.is_macro);
    });
}

const analyzer* macro_cache::find_cached_analyzer(const macro_cache_key& key) const
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
    return cached_data.cached_analyzer.get();
}

bool macro_cache::load_from_cache(const macro_cache_key& key, const analyzing_context & ctx)
{
    if (auto cached_analyzer = find_cached_analyzer(key))
    {
        if (key.data.proc_kind == processing::processing_kind::MACRO)
        {
            lsp::macro_info_ptr info = cached_analyzer->context().lsp_ctx->get_macro_info(key.data.library_member);
            if (!info)
                return true; // The file for which the analyzer is cached does not contain definition of macro
            ctx.hlasm_ctx->add_macro(info->macro_definition);
            ctx.lsp_ctx->add_macro(info, lsp::text_data_ref_t(macro_file_->get_text()));

            // Add all copy members on which this macro is dependant
            for (const auto& copy_ptr : info->macro_definition->used_copy_members)
            {
                auto copy_file = file_mngr_->find(copy_ptr->definition_location.file);
                ctx.hlasm_ctx->add_copy_member(copy_ptr);
                ctx.lsp_ctx->add_copy(copy_ptr, lsp::text_data_ref_t(copy_file->get_text()));
            }
        }
        else if (key.data.proc_kind == processing::processing_kind::COPY)
        {
            auto copy_member = cached_analyzer->context().hlasm_ctx->get_copy_member(key.data.library_member);
            ctx.hlasm_ctx->add_copy_member(copy_member);
            ctx.lsp_ctx->add_copy(copy_member, lsp::text_data_ref_t(macro_file_->get_text()));
        }
        
        return true;
    }
    return false;
}

version_stamp macro_cache::get_copy_member_versions(context::macro_def_ptr macro) const
{
    version_stamp result;

    for (const auto & copy_ptr : macro->used_copy_members)
    {
        auto file = file_mngr_->find(copy_ptr->definition_location.file);
        if (!file)
            throw std::runtime_error("Dependencies of a macro must be open right after parsing the macro.");
        result.try_emplace(file->get_file_name(), file->get_version());
    }
    return result;
}

void macro_cache::save_analyzer(const macro_cache_key& key, std::unique_ptr<analyzer> analyzer)
{
    auto& cache_data = cache_[key];
    if (key.data.proc_kind == processing::processing_kind::MACRO)
    {
        // Add stamps for all macro dependencies
        auto parsed_macro = analyzer->context().hlasm_ctx->get_macro_definition(key.data.library_member);
        if (parsed_macro)
            cache_data.stamps = get_copy_member_versions(std::move(parsed_macro));
        else
            cache_data.stamps.clear();
    }
    else // Copy members do not have additional dependencies
        cache_data.stamps.clear();

    cache_data.stamps.emplace(macro_file_->get_file_name(), macro_file_->get_version());
    cache_data.cached_analyzer = std::move(analyzer);
}

} // namespace hlasm_plugin::parser_library::workspaces
