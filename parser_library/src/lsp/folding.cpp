/*
 * Copyright (c) 2024 Broadcom.
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

#include "folding.h"

#include <algorithm>
#include <cctype>
#include <functional>
#include <stack>
#include <utility>

#include "../lexing/logical_line.h"

namespace {
using text_iterator =
    hlasm_plugin::utils::utf8_iterator<std::string_view::const_iterator, hlasm_plugin::utils::utf8_utf32_counter>;
using logical_line = hlasm_plugin::parser_library::lexing::logical_line<text_iterator>;

signed char get_comment_offset(const logical_line& r)
{
    auto b = r.segments.front().code_begin();
    const auto e = r.segments.front().code_end();
    if (b == e)
        return 0;
    if (*b == '*')
        return 1;
    if (*b++ != '.')
        return 0;
    return b != e && *b == '*' ? 2 : 0;
}

bool is_blank_comment(const logical_line& ll, unsigned char first_line_offset)
{
    return ll.segments.size() == 1
        && std::all_of(std::next(ll.begin(), first_line_offset), ll.end(), [](unsigned char e) { return e == ' '; });
}

bool is_separator(const logical_line& ll)
{
    static constexpr const auto threshold = 36;
    return ll.segments.size() == 1
        && std::ranges::count_if(ll, [](unsigned char e) { return e != ' ' && !std::isalnum(e); }) > threshold;
}

bool blank_line(const logical_line& r)
{
    return std::ranges::all_of(r, [](const auto& e) { return e == ' '; });
}

std::pair<bool, signed char> label_and_indent(const logical_line& r)
{
    const auto start = r.segments.front().code_begin();
    const auto e = r.segments.front().code_end();

    const auto label_end = std::find(start, e, ' ');
    const auto instr = std::find_if(label_end, e, [](char c) { return c != ' '; });

    if (instr == e)
        return { false, (signed char)-1 };

    return { label_end != start, (signed char)(instr.counter() - start.counter()) };
}

} // namespace

namespace hlasm_plugin::parser_library::lsp {

std::vector<line_entry> generate_indentation_map(std::string_view text)
{
    logical_line ll;
    text_iterator it(text.cbegin());
    const text_iterator end(text.cend());

    std::vector<line_entry> lines;

    size_t lineno = 0;
    while (lexing::extract_logical_line(ll, it, end, lexing::default_ictl))
    {
        const auto comment_offset = get_comment_offset(ll);
        const bool comment = comment_offset;
        const auto blank = blank_line(ll);
        const auto blank_comment = comment && is_blank_comment(ll, comment_offset);
        const auto separator = comment && is_separator(ll);
        const auto [has_label, indent] = comment ? std::pair(false, (signed char)-1) : label_and_indent(ll);
        const auto end_lineno = lineno + ll.segments.size();

        lines.push_back({
            .lineno = lineno,
            .end_lineno = end_lineno,
            .comment_offset = comment_offset,
            .indent = indent,
            .comment = comment,
            .blank = blank,
            .blank_comment = blank_comment,
            .separator = separator,
            .has_label = has_label,
            .suspicious = false,
        });

        lineno = end_lineno;
    }

    return lines;
}

void mark_suspicious(std::vector<lsp::line_entry>& lines)
{
    static constexpr size_t min_lines = 50;
    static constexpr size_t percentile_factor = 20; // 5%

    std::vector<signed char> indents;
    indents.reserve(lines.size());

    for (const auto& e : lines)
        if (e.indent > 0)
            indents.push_back(e.indent);

    if (indents.size() < min_lines)
        return;

    const auto percentile_pos = indents.begin() + indents.size() / percentile_factor;
    std::ranges::nth_element(indents, percentile_pos);
    const auto limit = *percentile_pos;

    for (auto& x : lines)
        if (x.indent > 0 && x.indent < limit)
            x.suspicious = true;
}

void folding_by_indentation(std::vector<fold_data>& data, std::span<const line_entry> lines)
{
    struct prev_region
    {
        size_t end_above;
        size_t lineno;
        signed char indent;
    };

    std::stack<prev_region> pr;
    pr.emplace(lines.back().end_lineno, lines.back().end_lineno, (signed char)-1);

    for (auto it = lines.rbegin(); it != lines.rend(); ++it)
    {
        const auto& line = *it;

        if (line.indent < 0 || line.comment || line.suspicious)
            continue;

        if (pr.top().indent > line.indent)
        {
            while (pr.top().indent > line.indent)
                pr.pop();
            if (pr.top().end_above >= 2 + line.end_lineno)
                data[line.lineno].indentation = pr.top().end_above - 1;
            else
            {
                data[line.lineno].small_structure = true;
                data[pr.top().end_above - 1].small_structure = true;
            }
        }

        if (pr.top().indent == line.indent)
            pr.top().end_above = line.lineno;
        else
            pr.emplace(line.lineno, line.lineno, line.indent);
    }
}

void folding_by_comments(std::vector<fold_data>& data, std::span<const line_entry> lines)
{
    static constexpr const auto iscomment = [](const line_entry& le) { return le.comment; };
    for (auto it = lines.begin(); (it = std::find_if(it, lines.end(), iscomment)) != lines.end();)
    {
        auto end = std::find_if(it, lines.end(), std::not_fn(iscomment));

        if (auto last = std::prev(end); it != last)
            data[it->lineno].comment = last->end_lineno - 1;

        it = end;
    }
}

void folding_between_comments(std::vector<fold_data>& data, std::span<const line_entry> lines)
{
    static constexpr const auto isnotcomment = [](const line_entry& le) { return !le.comment; };
    for (auto it = lines.begin(); (it = std::find_if(it, lines.end(), isnotcomment)) != lines.end();)
    {
        auto end = std::find_if(it, lines.end(), std::not_fn(isnotcomment));

        if (auto last = std::prev(end); it != last)
            data[it->lineno].notcomment = last->end_lineno - 1;

        it = end;
    }
}

void adjust_folding_data(std::span<fold_data> data)
{
    std::vector<bool> structured(data.size());
    for (size_t l = 0; l < data.size(); ++l)
    {
        if (auto el = data[l].indentation)
        {
            std::fill(structured.begin() + l, structured.begin() + el + 1, true);
            l = el;
        }
        else if (data[l].small_structure)
            structured[l] = true;
    }

    for (size_t l = 0; l < data.size(); ++l)
    {
        auto& d = data[l];
        if (!d.notcomment)
            continue;

        const auto reach = structured.begin() + d.notcomment + 1;
        auto s = std::find(structured.begin() + l, reach, true);
        if (s == reach)
            continue;

        auto se = std::find(s, reach, false);

        auto newlimit = s - structured.begin();
        auto oldend = d.notcomment;

        d.notcomment = newlimit - 1;

        if (se == reach)
            continue;

        auto& nextd = data[se - structured.begin()];
        nextd.notcomment = std::max(nextd.notcomment, oldend);
    }
}

std::vector<fold_data> compute_folding_data(std::span<const line_entry> lines)
{
    if (lines.empty())
        return {};

    std::vector<fold_data> data(lines.back().end_lineno);

    folding_by_indentation(data, lines);
    folding_by_comments(data, lines);
    folding_between_comments(data, lines);

    adjust_folding_data(data);

    return data;
}

std::vector<folding_range> generate_folding_ranges(std::span<const fold_data> data)
{
    std::vector<folding_range> result;

    for (size_t l = 0; l < data.size(); ++l)
    {
        if (auto end = data[l].indentation)
            result.emplace_back(l, end, fold_type::none);
        else if ((end = data[l].comment) != 0)
            result.emplace_back(l, end, fold_type::comment);
        else if ((end = data[l].notcomment) != 0)
            result.emplace_back(l, end, fold_type::none);
    }

    return result;
}

} // namespace hlasm_plugin::parser_library::lsp
