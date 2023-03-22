/*
 * Copyright (c) 2023 Broadcom.
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

#include "file.h"

#include "utils/unicode_text.h"

namespace hlasm_plugin::parser_library::workspaces {

size_t index_from_position(std::string_view text, const std::vector<size_t>& line_indices, position loc)
{
    size_t end = loc.column;
    if (loc.line >= line_indices.size())
        return text.size();
    size_t i = line_indices[loc.line];
    size_t utf16_counter = 0;

    while (utf16_counter < end && i < text.size())
    {
        if (unsigned char c = text[i]; utils::utf8_one_byte_begin(c)) [[likely]]
        {
            ++i;
            ++utf16_counter;
        }
        else
        {
            const auto cs = utils::utf8_prefix_sizes[c];

            if (!cs.utf8)
                throw std::runtime_error("The text of the file is not in utf-8."); // WRONG UTF-8 input

            i += cs.utf8;
            utf16_counter += cs.utf16;
        }
    }
    return i;
}

void find_newlines(std::vector<size_t>& output, std::string_view text)
{
    static constexpr std::string_view nl("\r\n");
    for (auto i = text.find_first_of(nl); i != std::string_view::npos; i = text.find_first_of(nl, i))
    {
        i += 1 + text.substr(i).starts_with(nl);
        output.push_back(i);
    }
}

std::vector<size_t> create_line_indices(std::string_view text)
{
    std::vector<size_t> ret;
    create_line_indices(ret, text);
    return ret;
}

void create_line_indices(std::vector<size_t>& output, std::string_view text)
{
    output.clear();
    output.push_back(0);
    find_newlines(output, text);
}

void apply_text_diff(std::string& text, std::vector<size_t>& lines, range r, std::string_view replacement)
{
    if (r.start > r.end || r.end.line > lines.size())
        return;

    size_t range_end_line = r.end.line;
    size_t range_start_line = r.start.line;

    size_t begin = index_from_position(text, lines, r.start);
    size_t end = index_from_position(text, lines, r.end);

    text.replace(begin, end - begin, replacement);

    std::vector<size_t> new_lines;
    find_newlines(new_lines, replacement);

    size_t old_lines_count = range_end_line - range_start_line;
    size_t new_lines_count = new_lines.size();

    size_t char_diff = replacement.size() - (end - begin);

    // add or remove lines depending on the difference
    if (new_lines_count > old_lines_count)
    {
        size_t diff = new_lines_count - old_lines_count;
        lines.insert(lines.end(), diff, 0);

        for (size_t i = lines.size() - 1; i > range_end_line + diff; --i)
        {
            lines[i] = lines[i - diff] + char_diff;
        }
    }
    else
    {
        size_t diff = old_lines_count - new_lines_count;

        for (size_t i = range_start_line + 1 + new_lines_count; i < lines.size() - diff; ++i)
        {
            lines[i] = lines[i + diff] + char_diff;
        }

        lines.erase(lines.end() - diff, lines.end());
    }


    for (size_t i = range_start_line + 1; i <= range_start_line + new_lines_count; ++i)
    {
        lines[i] = new_lines[i - range_start_line - 1] + begin;
    }
}

} // namespace hlasm_plugin::parser_library::workspaces
