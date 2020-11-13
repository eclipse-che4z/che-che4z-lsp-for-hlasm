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

#include "file_info.h"

#include <algorithm>

namespace hlasm_plugin::parser_library::lsp {

file_info::file_info(context::macro_def_ptr owner)
    : name(owner->definition_location.file)
    , type(file_type::MACRO)
    , owner(std::move(owner))
{}

file_info::file_info(context::copy_member_ptr owner)
    : name(owner->definition_location.file)
    , type(file_type::COPY)
    , owner(std::move(owner))
{}

bool file_info::is_in_range(const position& pos, const range& r)
{
    return r.start.line >= pos.line && r.end.line <= pos.line && r.start.column >= pos.column
        && r.end.column <= pos.column;
}

const symbol_occurence* file_info::find_occurence(position pos, macro_info_ptr& macro_i)
{
    const symbol_occurence* found = nullptr;

    // find in occurences
    for (const auto& occ : occurences)
        if (is_in_range(pos, occ.occurence_range))
            found = &occ;

    // if not found, return
    if (!found)
        return nullptr;

    // else, find scope
    for (const auto& scope : slices)
    {
        if (scope.begin_line < pos.line)
        {
            macro_i = nullptr;
            break;
        }
        if (scope.begin_line <= pos.line && scope.end_line >= pos.line)
        {
            macro_i = scope.macro_context;
            break;
        }
    }
    return found;
}

void file_info::update_occurences(const occurence_storage& occurences_upd)
{
    for (const auto& occ : occurences_upd)
        occurences.emplace_back(occ);
}

void file_info::update_slices(const std::vector<file_slice_t>& slices_upd)
{
    size_t curr_slice = 0;
    bool reached_end = false;

    for (size_t i = 0; i < slices_upd.size(); ++i)
    {
        if (curr_slice == slices.size() || reached_end) // if at the end of current slices, append new slices to the end
        {
            reached_end = true;
            slices.emplace_back(slices_upd[i]);
        }
        else if (slices_upd[i].end_line <= slices[curr_slice].begin_line) // if new slice inbetween present slices
            slices.insert(slices.begin() + curr_slice++, slices_upd[i]);
        else if (slices_upd[i].begin_line == slices[curr_slice].begin_line
            && slices_upd[i].end_line == slices[curr_slice].end_line) // if slices match exactly
            slices[curr_slice++] = slices_upd[i];
        else
            curr_slice++;
    }
}

file_slice_t file_slice_t::transform_slice(const macro_slice_t& slice, macro_info_ptr macro_i)
{
    file_slice_t fslice;

    fslice.begin_idx = slice.begin_statement;
    fslice.end_idx = slice.end_statement;

    fslice.begin_line = macro_i->macro_definition->copy_nests[fslice.begin_idx].back().pos.line;
    fslice.begin_line = macro_i->macro_definition->copy_nests[fslice.end_idx].back().pos.line;

    fslice.type = slice.inner_macro ? scope_type::INNER_MACRO : scope_type::MACRO;
    fslice.macro_context = macro_i;

    return fslice;
}

std::vector<file_slice_t> file_slice_t::transform_slices(
    const std::vector<macro_slice_t>& slices, macro_info_ptr macro_i)
{
    std::vector<file_slice_t> ret;
    for (const auto& s : slices)
        ret.push_back(transform_slice(s, macro_i));
    return ret;
}

} // namespace hlasm_plugin::parser_library::lsp
