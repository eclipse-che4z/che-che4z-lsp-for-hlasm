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

std::vector<cached_opsyn_mnemo> get_opsyn_state(context::hlasm_context& ctx)
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
        if (std::find(cached_instr.begin(), cached_instr.end(), opcode.get_instr_id()) != cached_instr.end())
            result.push_back({ from, opcode.get_instr_id(), opcode.macro_opcode != nullptr });
    }

    // Also macros with the same name as CA instructions may alias the instructions
    for (const auto& [id, macro] : ctx.macros())
    {
        if (!macro)
            continue;
        // Macros for which there is an opsyn in effect are already captured
        if (ctx.get_operation_code(id).macro_opcode != macro)
            continue;

        // If there is an opsyn, that aliases an instruction to be CA instruction, add it to result
        if (std::find(cached_instr.begin(), cached_instr.end(), macro->id) != cached_instr.end())
            result.push_back({ id, id, true });
    }

    std::sort(result.begin(), result.end(), [](const cached_opsyn_mnemo& lhs, const cached_opsyn_mnemo& rhs) {
        return std::tie(lhs.from_instr, lhs.to_instr, lhs.is_macro)
            < std::tie(rhs.from_instr, rhs.to_instr, rhs.is_macro);
    });

    return result;
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
    return &cached_data.cached_analyzer;
}

bool macro_cache::load_from_cache(const macro_cache_key& key, analyzing_context ctx)
{
    if (auto cached_analyzer = find_cached_analyzer(key))
    {
        if (key.data.proc_kind == processing::processing_kind::MACRO)
        {
            lsp::macro_info_ptr info = cached_analyzer->context().lsp_ctx->get_macro_info(key.data.library_member);
            ctx.hlasm_ctx->add_macro(info->macro_definition);
            ctx.lsp_ctx->add_macro(info, lsp::text_data_ref_t(macro_file_->get_text()));
        }
        else if (key.data.proc_kind == processing::processing_kind::COPY)
        {
            auto copy_member = cached_analyzer->context().hlasm_ctx->get_copy_member(key.data.library_member);
            ctx.hlasm_ctx->add_copy_member(copy_member);
            ctx.lsp_ctx->add_copy(copy_member, lsp::text_data_ref_t(macro_file_->get_text()));
        }
        // Add all copy members dependant on this macro/copy?
        return true;
    }

    /*std::variant<lsp::macro_info_ptr, context::copy_member_ptr> external_dep;
    assert(data.proc_kind == processing::processing_kind::MACRO || data.proc_kind == processing::processing_kind::COPY);
    if (data.proc_kind == processing::processing_kind::MACRO)
        external_dep = ctx.lsp_ctx->get_macro_info(data.library_member);
    else if (data.proc_kind == processing::processing_kind::COPY)
        external_dep = ctx.hlasm_ctx->get_copy_member(data.library_member);
        */
    return false;
}

version_stamp macro_cache::get_copy_member_versions(context::macro_def_ptr ctx) const
{
    version_stamp result;
    auto copy_files = ctx->get_copy_files();
    
    for (auto it = copy_files.begin(); it != copy_files.end();)
    {
        auto file = file_mngr_->find(*it);
        if (!file)
            throw std::runtime_error("Dependencies of a macro must be open right after parsing the macro.");
        result.emplace(std::move(copy_files.extract(it++).value()), file->get_version());
    }
    return result;
}

void macro_cache::save_analyzer(const macro_cache_key& key, std::unique_ptr<analyzer> analyzer) {}


macro_cache_key macro_cache_key::create_from_context(analyzing_context ctx, library_data data)
{
    return { data, get_opsyn_state(*ctx.hlasm_ctx) };
}

} // namespace hlasm_plugin::parser_library::workspaces
