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


#include "file_impl.h"

#include <cerrno>
#include <codecvt>
#include <exception>
#include <fstream>
#include <locale>
#include <string>

#include "utils/utf8text.h"


namespace hlasm_plugin::parser_library::workspaces {

file_impl::file_impl(file_uri uri)
    : file_name_(std::move(uri))
    , text_()
{}

void file_impl::collect_diags() const {}

const file_uri& file_impl::get_file_name() { return file_name_; }

const std::string& file_impl::get_text()
{
    if (!up_to_date_)
        load_text();
    return text_;
}

void file_impl::load_text()
{
    std::ifstream fin(file_name_, std::ios::in | std::ios::binary);

    if (fin)
    {
        fin.seekg(0, std::ios::end);
        text_.resize((size_t)fin.tellg());
        fin.seekg(0, std::ios::beg);
        fin.read(&text_[0], text_.size());
        fin.close();

        text_ = replace_non_utf8_chars(text_);

        up_to_date_ = true;
        bad_ = false;
        return;
    }
    else
    {
        text_ = "";
        up_to_date_ = false;
        bad_ = true;
        // add_diagnostic(diagnostic_s{file_name_, {}, diagnostic_severity::error,
        //	"W0001", "HLASM plugin", "Could not open file" + file_name_, {} });
    }
}

// adds positions of newlines into vector 'lines'
size_t find_newlines(const std::string& text, std::vector<size_t>& lines)
{
    size_t before = lines.size();
    bool was_r = false;
    for (size_t i = 0; i < text.size(); ++i)
    {
        char ch = text[i];
        if (was_r)
        {
            if (ch == '\n')
            {
                lines.push_back(i + 1);
                was_r = false;
            }
            else if (ch == '\r')
                lines.push_back(i);
            else
            {
                lines.push_back(i);
                was_r = false;
            }
        }
        else
        {
            if (ch == '\n')
                lines.push_back(i + 1);
            else if (ch == '\r')
                was_r = true;
        }
    }

    if (was_r)
        lines.push_back(text.size());

    return lines.size() - before;
}

void file_impl::did_open(std::string new_text, version_t version)
{
    text_ = std::move(new_text);
    version_ = version;

    lines_ind_.clear();
    lines_ind_.push_back(0);

    find_newlines(text_, lines_ind_);
    up_to_date_ = true;
    bad_ = false;
    editing_ = true;
}

bool file_impl::get_lsp_editing() { return editing_; }


// applies a change to the text and updates line begginings
void file_impl::did_change(range range, std::string new_text)
{
    size_t range_end_line = (size_t)range.end.line;
    size_t range_start_line = (size_t)range.start.line;

    size_t begin = index_from_position(text_, lines_ind_, range.start);
    size_t end = index_from_position(text_, lines_ind_, range.end);

    text_.replace(begin, end - begin, new_text);

    std::vector<size_t> new_lines;

    find_newlines(new_text, new_lines);


    size_t old_lines_count = range_end_line - range_start_line;
    size_t new_lines_count = new_lines.size();

    size_t char_diff = new_text.size() - (end - begin);

    // add or remove lines depending on the difference
    if (new_lines_count > old_lines_count)
    {
        size_t diff = new_lines_count - old_lines_count;
        lines_ind_.insert(lines_ind_.end(), diff, 0);

        for (size_t i = lines_ind_.size() - 1; i > range_end_line + diff; --i)
        {
            lines_ind_[i] = lines_ind_[i - diff] + char_diff;
        }
    }
    else
    {
        size_t diff = old_lines_count - new_lines_count;

        for (size_t i = range_start_line + 1 + new_lines_count; i < lines_ind_.size() - diff; ++i)
        {
            lines_ind_[i] = lines_ind_[i + diff] + char_diff;
        }

        for (size_t i = 0; i < diff; ++i)
            lines_ind_.pop_back();
    }


    for (size_t i = range_start_line + 1; i <= range_start_line + new_lines_count; ++i)
    {
        lines_ind_[i] = new_lines[i - (size_t)range_start_line - 1] + begin;
    }


    ++version_;
}

void file_impl::did_change(std::string new_text)
{
    text_ = std::move(new_text);
    lines_ind_.clear();
    lines_ind_.push_back(0);
    find_newlines(text_, lines_ind_);
    ++version_;
}

void file_impl::did_close() { editing_ = false; }

const std::string& file_impl::get_text_ref() { return text_; }

version_t file_impl::get_version() { return version_; }

bool file_impl::update_and_get_bad()
{
    // If user is editing file through LSP, do not load from disk.
    if (editing_)
        return false;

    load_text();
    return bad_;
}

bool utf8_one_byte_begin(char ch)
{
    return (ch & 0x80) == 0; // 0xxxxxxx
}

bool utf8_continue_byte(char ch)
{
    return (ch & 0xC0) == 0x80; // 10xxxxxx
}


bool utf8_two_byte_begin(char ch)
{
    return (ch & 0xE0) == 0xC0; // 110xxxxx
}

bool utf8_three_byte_begin(char ch)
{
    return (ch & 0xF0) == 0xE0; // 1110xxxx
}

bool utf8_four_byte_begin(char ch)
{
    return (ch & 0xF8) == 0xF0; // 11110xxx
}

size_t file_impl::index_from_position(const std::string& text, const std::vector<size_t>& line_indices, position loc)
{
    size_t end = (size_t)loc.column;
    if (loc.line >= line_indices.size())
        return text.size();
    size_t i = line_indices[loc.line];
    size_t utf16_counter = 0;

    while (utf16_counter < end && i < text.size())
    {
        if (!utf8_one_byte_begin(text[i]))
        {
            const auto cs = utils::utf8_prefix_sizes[(unsigned char)text[i]];

            if (!cs.utf8)
                throw std::runtime_error("The text of the file is not in utf-8."); // WRONG UTF-8 input

            i += cs.utf8;
            utf16_counter += cs.utf16;
        }
        else
        {
            ++i;
            ++utf16_counter;
        }
    }
    return i;
}

std::string file_impl::replace_non_utf8_chars(std::string_view text)
{
    std::string ret;
    ret.reserve(text.size());
    while (!text.empty())
    {
        if (utf8_one_byte_begin(text.front()))
        {
            ret.push_back(text.front());
            text.remove_prefix(1);
            continue;
        }

        const auto cs = utils::utf8_prefix_sizes[(unsigned char)text.front()];
        if (cs.utf8 != 0 && cs.utf8 <= text.size() && utils::utf8_valid_multibyte_prefix(text[0], text[1])
            && std::all_of(text.begin() + 2, text.begin() + cs.utf8, utf8_continue_byte))
        {
            // copy the character to output
            ret.append(text.substr(0, cs.utf8));
            text.remove_prefix(cs.utf8);
        }
        else
        {
            // UTF8 replacement for unknown character
            ret.push_back((uint8_t)0xEF);
            ret.push_back((uint8_t)0xBF);
            ret.push_back((uint8_t)0xBD);
            text.remove_prefix(1);
        }
    }
    return ret;
}

std::vector<size_t> file_impl::create_line_indices(const std::string& text)
{
    std::vector<size_t> ret;
    ret.push_back(0);
    find_newlines(text, ret);
    return ret;
}


} // namespace hlasm_plugin::parser_library::workspaces
