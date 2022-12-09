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

#include "workspaces/file_impl.h"

namespace hlasm_plugin::parser_library::lsp {

bool operator==(const line_range& lhs, const line_range& rhs) { return lhs.begin == rhs.begin && lhs.end == rhs.end; }
bool operator<(const line_range& lhs, const line_range& rhs)
{
    return std::tie(lhs.begin, lhs.end) < std::tie(rhs.begin, rhs.end);
}

file_info::file_info(utils::resource::resource_location location, text_data_view text_data)
    : location(std::move(location))
    , type(file_type::OPENCODE)
    , data(std::move(text_data))
{}

file_info::file_info(context::macro_def_ptr owner, text_data_view text_data)
    : location(owner->definition_location.resource_loc)
    , type(file_type::MACRO)
    , owner(std::move(owner))
    , data(std::move(text_data))
{}

file_info::file_info(context::copy_member_ptr owner, text_data_view text_data)
    : location(owner->definition_location.resource_loc)
    , type(file_type::COPY)
    , owner(std::move(owner))
    , data(std::move(text_data))
{}

bool file_info::is_in_range(const position& pos, const range& r)
{
    auto pos_tie = std::tie(pos.line, pos.column);
    return std::tie(r.start.line, r.start.column) <= pos_tie && pos_tie <= std::tie(r.end.line, r.end.column);
}

occurence_scope_t file_info::find_occurence_with_scope(position pos) const
{
    std::pair<const symbol_occurence*, bool> found_pair;
    auto& [found, found_in_evaluated_model] = found_pair;

    auto l = std::lower_bound(occurences.begin(), occurences.end(), pos, [](const auto& occ, const auto& p) {
        return occ.occurence_range.end.line < p.line;
    });
    auto it_limit = occurences_start_limit.begin() + std::distance(occurences.begin(), l);
    // find in occurrences
    for (auto it = l; it != occurences.end() && *it_limit <= pos.line; ++it, ++it_limit)
    {
        const auto& occ = *it;
        if (is_in_range(pos, occ.occurence_range))
        {
            if (!found || !occ.evaluated_model && found_in_evaluated_model)
                found_pair = { &occ, occ.evaluated_model };
            if (!found_in_evaluated_model)
                break;
        }
    }

    // if not found, return
    if (!found)
        return std::make_pair(nullptr, nullptr);

    // else, find scope
    macro_info_ptr macro_i = find_scope(pos);
    return std::make_pair(found, std::move(macro_i));
}

macro_info_ptr file_info::find_scope(position pos) const
{
    for (const auto& [_, scope] : slices)
        if (scope.file_lines.begin <= pos.line && scope.file_lines.end > pos.line)
            return scope.macro_context;
    return nullptr;
}

std::vector<position> file_info::find_references(
    const symbol_occurence& occurence, const std::vector<symbol_occurence>& occurences)
{
    std::vector<position> result;
    for (const auto& occ : occurences)
        if (occurence.is_similar(occ))
            result.emplace_back(occ.occurence_range.start);
    return result;
}

void file_info::update_occurences(const occurence_storage& occurences_upd)
{
    occurences.insert(occurences.end(), occurences_upd.begin(), occurences_upd.end());
}

void file_info::update_slices(const std::vector<file_slice_t>& slices_upd)
{
    for (const auto& slice : slices_upd)
    {
        // If the slice is not there yet, add it.
        // If the slice is already there, overwrite it with the new slice.
        // There may be some obscure cases where the same portion of code is parsed multiple times, resulting in
        // defining more macros with the same name. For now, we take the last definition (since file_slice contains
        // pointer to macro definition).
        slices[slice.file_lines] = slice;
    }
}

file_slice_t file_slice_t::transform_slice(const macro_slice_t& slice, macro_info_ptr macro_i)
{
    file_slice_t fslice;

    fslice.macro_lines.begin = slice.begin_statement;
    fslice.macro_lines.end = slice.end_statement;

    if (slice.begin_statement == 0)
        fslice.file_lines.begin = macro_i->definition_location.pos.line;
    else
        fslice.file_lines.begin = macro_i->macro_definition->copy_nests[fslice.macro_lines.begin].back().loc.pos.line;

    if (slice.end_statement == macro_i->macro_definition->copy_nests.size())
        fslice.file_lines.end = macro_i->macro_definition->copy_nests.back().back().loc.pos.line + 1;
    else
        fslice.file_lines.end = macro_i->macro_definition->copy_nests[fslice.macro_lines.end].back().loc.pos.line;

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


const std::vector<symbol_occurence>& file_info::get_occurences() const { return occurences; }

void file_info::process_occurrences()
{
    std::sort(occurences.begin(), occurences.end(), [](const auto& l, const auto& r) {
        return std::tie(l.occurence_range.end.line, l.occurence_range.start.line, l.evaluated_model)
            < std::tie(r.occurence_range.end.line, r.occurence_range.start.line, r.evaluated_model);
    });

    occurences_start_limit.resize(occurences.size());

    std::transform(occurences.rbegin(),
        occurences.rend(),
        occurences_start_limit.rbegin(),
        [min = (size_t)-1](const auto& occ) mutable { return min = std::min(min, occ.occurence_range.start.line); });
}

} // namespace hlasm_plugin::parser_library::lsp
