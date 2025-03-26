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
#include <ranges>

namespace {
constexpr auto occurrence_end_line(const hlasm_plugin::parser_library::lsp::symbol_occurrence& o)
{
    return o.occurrence_range.end.line;
}
} // namespace

namespace hlasm_plugin::parser_library::lsp {

file_info::file_info(text_data_view text_data)
    : type(file_type::OPENCODE)
    , data(std::move(text_data))
{}

file_info::file_info(context::macro_def_ptr owner, text_data_view text_data)
    : type(file_type::MACRO)
    , owner(std::move(owner))
    , data(std::move(text_data))
{}

file_info::file_info(context::copy_member_ptr owner, text_data_view text_data)
    : type(file_type::COPY)
    , owner(std::move(owner))
    , data(std::move(text_data))
{}

bool file_info::is_in_range(const position& pos, const range& r)
{
    auto pos_tie = std::tie(pos.line, pos.column);
    return std::tie(r.start.line, r.start.column) <= pos_tie && pos_tie <= std::tie(r.end.line, r.end.column);
}

occurrence_scope_t file_info::find_occurrence_with_scope(position pos) const
{
    std::pair<const symbol_occurrence*, size_t> found_pair(nullptr, (size_t)-1);
    auto& [found, priority] = found_pair;

    auto l = std::ranges::lower_bound(occurrences, pos.line, {}, occurrence_end_line);
    auto it_limit = occurrences_start_limit.begin() + std::ranges::distance(occurrences.begin(), l);
    // find in occurrences
    for (auto it = l; it != occurrences.end() && *it_limit <= pos.line; ++it, ++it_limit)
    {
        const auto& occ = *it;
        if (is_in_range(pos, occ.occurrence_range))
        {
            auto occ_priority = 1 * occ.evaluated_model + 2 * (occ.kind == occurrence_kind::INSTR_LIKE);
            if (!found || occ_priority < priority)
                found_pair = { &occ, occ_priority };
            if (priority == 0)
                break;
        }
    }

    // if not found, return
    if (!found)
        return std::make_pair(nullptr, nullptr);

    // else, find scope
    const auto* macro_i = find_scope(pos);
    return std::make_pair(found, macro_i);
}

const symbol_occurrence* file_info::find_closest_instruction(position pos) const noexcept
{
    auto l = std::ranges::upper_bound(occurrences, pos.line, {}, occurrence_end_line);
    auto instr = std::find_if(std::make_reverse_iterator(l), occurrences.rend(), [](const auto& occ) {
        return occ.kind == occurrence_kind::INSTR || occ.kind == occurrence_kind::INSTR_LIKE;
    });
    if (instr == occurrences.rend() || instr->kind != occurrence_kind::INSTR)
        return nullptr;

    if (instr->occurrence_range.start.line >= line_details.size())
        return nullptr;

    const auto& ld = line_details[instr->occurrence_range.start.line];
    if (ld.max_endline == 0)
        return nullptr;

    // pos.column == 15 is a workaround around incorrect range assignment for statements that were freshly continued
    if (pos.line >= ld.max_endline + (pos.column == 15))
        return nullptr;

    return std::to_address(instr);
}

std::pair<const context::section*, index_t<context::using_collection>> file_info::find_reachable_sections(
    position pos) const
{
    auto l = std::ranges::upper_bound(occurrences, pos.line, {}, occurrence_end_line);
    auto instr = std::find_if(std::make_reverse_iterator(l), occurrences.rend(), [](const auto& occ) {
        return occ.kind == occurrence_kind::INSTR || occ.kind == occurrence_kind::INSTR_LIKE;
    });
    if (instr == occurrences.rend() || instr->kind != occurrence_kind::INSTR)
        return {};

    if (instr->occurrence_range.start.line >= line_details.size())
        return {};

    const auto& ld = line_details[instr->occurrence_range.start.line];

    return { ld.active_section, ld.active_using };
}

const line_occurence_details* file_info::get_line_details(size_t l) const noexcept
{
    if (l >= line_details.size())
        return nullptr;
    return &line_details[l];
}

const macro_info* file_info::find_scope(position pos) const
{
    for (const auto& scope : slices)
        if (scope.file_lines.begin <= pos.line && scope.file_lines.end > pos.line)
            return scope.macro_context;
    return nullptr;
}

std::vector<bool> file_info::macro_map() const
{
    if (slices.empty())
        return {};
    static constexpr auto line_end = [](const auto& s) { return s.file_lines.end; };
    std::vector<bool> result(std::ranges::max(slices | std::views::transform(line_end)));
    for (const auto& scope : slices)
    {
        std::fill(result.begin() + scope.file_lines.begin, result.begin() + scope.file_lines.end, true);
    }
    return result;
}

std::vector<position> file_info::find_references(
    const symbol_occurrence& occurrence, const std::vector<symbol_occurrence>& occurrences)
{
    std::vector<position> result;
    for (const auto& occ : occurrences)
        if (occurrence.is_similar(occ))
            result.emplace_back(occ.occurrence_range.start);
    std::ranges::sort(result);
    result.erase(std::ranges::unique(result).begin(), result.end());
    return result;
}

void file_info::update_occurrences(const std::vector<symbol_occurrence>& occurrences_upd,
    const std::vector<lsp::line_occurence_details>& line_details_upd)
{
    occurrences.insert(occurrences.end(), occurrences_upd.begin(), occurrences_upd.end());

    static constexpr auto merge_lines = [](const auto& o, const auto& n) {
        return lsp::line_occurence_details {
            std::max(o.max_endline, n.max_endline),
            o.active_using ? o.active_using : n.active_using,
            o.active_section ? o.active_section : n.active_section,
            o.active_using && n.active_using && o.active_using != n.active_using,
            o.active_section && n.active_section && o.active_section != n.active_section,
            o.branches_up || n.branches_up,
            o.branches_down || n.branches_down,
            o.branches_somewhere || n.branches_somewhere,
            std::max(o.offset_to_jump_opcode, n.offset_to_jump_opcode),
        };
    };
    const auto [e, u, _] = std::ranges::transform(line_details, line_details_upd, line_details.begin(), merge_lines);

    line_details.insert(e, u, line_details_upd.end());
}

void file_info::distribute_macro_slices(
    const std::unordered_map<const context::macro_definition*, macro_info_ptr>& macros,
    std::unordered_map<utils::resource::resource_location, file_info>& files)
{
    // If the slice is not there yet, add it.
    // If the slice is already there, overwrite it with the new slice.
    // There may be some obscure cases where the same portion of code is parsed multiple times, resulting in
    // defining more macros with the same name. For now, we take the last definition (since file_slice contains
    // pointer to macro definition).
    for (const auto& [_, m] : macros)
    {
        for (const auto& [file, s_upd] : m->file_scopes)
        {
            std::ranges::transform(s_upd.first, std::back_inserter(files.at(file).slices), [&m, &file](const auto& s) {
                return file_slice_t::transform_slice(s, *m, file);
            });
        }
    }

    static constexpr auto prefer_macros = [](const file_slice_t& e) {
        return std::make_tuple(std::cref(e.file_lines), -(int)e.type);
    };
    for (auto& [_, file] : files)
    {
        auto& slices = file.slices;
        // TODO: This is a workaround for what is obviously a bug
        std::erase_if(slices, [](const file_slice_t& s) { return s.file_lines.begin > s.file_lines.end; });
        std::ranges::stable_sort(slices, {}, prefer_macros);
        auto [new_begin, __] = std::ranges::unique(std::views::reverse(slices), {}, &file_slice_t::file_lines);
        slices.erase(slices.begin(), new_begin.base());
    }
}

namespace {
auto source_line(
    const context::macro_definition& mdef, context::statement_id id, const utils::resource::resource_location& file)
{
    constexpr auto resloc = [](const context::copy_nest_item& cni) -> decltype(auto) { return cni.loc.resource_loc; };
    const auto& copy_nest = mdef.get_copy_nest(id);
    const auto it = std::ranges::find(copy_nest | std::views::reverse, file, resloc);
    // TODO: This is a workaround for what is obviously a bug
    if (it == copy_nest.rend())
        return (size_t)0;
    assert(it != copy_nest.rend());
    return it->loc.pos.line;
}
} // namespace

file_slice_t file_slice_t::transform_slice(
    const macro_slice_t& slice, const macro_info& macro_i, const utils::resource::resource_location& f)
{
    file_slice_t fslice;

    fslice.macro_lines.begin = slice.begin_statement;
    fslice.macro_lines.end = slice.end_statement;

    const auto& mdef = *macro_i.macro_definition;

    if (slice.begin_statement.value == 0)
        fslice.file_lines.begin = macro_i.definition_location.pos.line;
    else
        fslice.file_lines.begin = source_line(mdef, slice.begin_statement, f);

    fslice.file_lines.end = source_line(mdef, { slice.end_statement.value }, f) + 1;

    fslice.type = slice.inner_macro ? scope_type::INNER_MACRO : scope_type::MACRO;
    fslice.macro_context = &macro_i;

    return fslice;
}

const std::vector<symbol_occurrence>& file_info::get_occurrences() const { return occurrences; }

void file_info::process_occurrences()
{
    std::ranges::sort(occurrences, {}, [](const auto& e) {
        return std::tie(e.occurrence_range.end.line, e.occurrence_range.start.line, e.evaluated_model);
    });

    occurrences_start_limit.resize(occurrences.size());

    auto m = (size_t)-1;
    std::transform(occurrences.rbegin(), occurrences.rend(), occurrences_start_limit.rbegin(), [&m](const auto& occ) {
        return m = std::min(m, occ.occurrence_range.start.line);
    });
}

void file_info::collect_instruction_like_references(
    std::unordered_map<context::id_index, utils::resource::resource_location>& m) const
{
    for (const auto& occ : occurrences)
    {
        if (occ.kind != occurrence_kind::INSTR_LIKE)
            continue;
        m.try_emplace(occ.name);
    }
}

} // namespace hlasm_plugin::parser_library::lsp
