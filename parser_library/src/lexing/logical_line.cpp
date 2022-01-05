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

#include "logical_line.h"

namespace hlasm_plugin::parser_library::lexing {
std::pair<std::string_view, logical_line_segment_eol> extract_line(std::string_view& input)
{
    auto eol = input.find_first_of("\r\n");
    if (eol == std::string_view::npos)
    {
        std::string_view ret = input;
        input = std::string_view();
        return std::make_pair(ret, logical_line_segment_eol::none);
    }
    else
    {
        auto ret = std::make_pair(input.substr(0, eol), logical_line_segment_eol::lf);
        size_t remove = eol + 1;
        if (input.at(eol) == '\r')
        {
            if (input.size() > eol + 1 && input.at(eol + 1) == '\n')
            {
                ++remove;
                ret.second = logical_line_segment_eol::crlf;
            }
            else
                ret.second = logical_line_segment_eol::cr;
        }
        input.remove_prefix(remove);

        return ret;
    }
}

template<bool validate>
std::pair<size_t, size_t> substr_step(std::string_view& s, size_t& chars)
{
    std::pair<size_t, size_t> result = { 0, 0 };

    while (chars)
    {
        if (s.empty())
            break;
        --chars;
        ++result.first;

        unsigned char c = s.front();
        if (c < 0x80)
        {
            ++result.second;
            s.remove_prefix(1);
            continue;
        }

        const auto cs = utf8_prefix_sizes[c];
        if constexpr (validate)
        {
            if (!cs.utf8 || s.size() < cs.utf8)
                throw hlasm_plugin::parser_library::lexing::utf8_error();
            for (const auto* p = s.data() + 1; p != s.data() + cs.utf8; ++p)
                if ((*p & 0xc0) != 0x80)
                    throw hlasm_plugin::parser_library::lexing::utf8_error();
        }

        result.second += cs.utf16;
        s.remove_prefix(cs.utf8);
    }

    return result;
}

std::pair<std::string_view, size_t> skip_chars(std::string_view s, size_t count)
{
    const auto s_ = s;
    auto [_, utf16_skipped] = substr_step<true>(s, count);
    return std::pair(s_.substr(s_.size() - s.size()), utf16_skipped);
}

template<bool validate>
utf8_substr_result utf8_substr(std::string_view s, size_t offset, size_t length)
{
    substr_step<validate>(s, offset);

    if (offset) // not long enought
        return {};

    utf8_substr_result result = { s, 0, 0 };

    std::tie(result.char_count, result.utf16_len) = substr_step<validate>(s, length);

    result.str = result.str.substr(0, result.str.size() - s.size());

    return result;
}

template utf8_substr_result utf8_substr<false>(std::string_view s, size_t offset, size_t length);
template utf8_substr_result utf8_substr<true>(std::string_view s, size_t offset, size_t length);

size_t length_utf16(std::string_view s)
{
    auto len = (size_t)-1;

    auto [_, utf16] = substr_step<true>(s, len);

    return utf16;
}

// returns "need more", input must be non-empty
bool append_to_logical_line(logical_line& out, std::string_view& input, const logical_line_extractor_args& opts)
{
    auto [line, eol] = extract_line(input);
    auto [code_start, utf16_begin_offset] = skip_chars(line, opts.begin - 1);
    auto after_code = skip_chars(code_start, opts.end + 1 - opts.begin).first;

    logical_line_segment& segment = out.segments.emplace_back();
    segment.eol = eol;
    segment.line = line;
    segment.code = code_start.substr(0, after_code.data() - code_start.data());
    segment.code_offset = code_start.data() - line.data();
    segment.code_offset_utf16 = utf16_begin_offset;
    if (after_code.empty())
        return false;

    if (after_code.at(0) == ' ' || opts.end == 80 || opts.continuation == 0)
    {
        segment.ignore = after_code;
        return false;
    }

    // line is continued
    segment.ignore = skip_chars(after_code, 1).first;
    segment.continuation = after_code.substr(0, segment.ignore.data() - after_code.data());

    if (opts.dbcs)
    {
        size_t continuation_chars = 1;

        const auto cont_size = segment.continuation.size();
        for (const auto* s = segment.continuation.data() - cont_size; s >= code_start.data(); s -= cont_size)
        {
            if (std::string_view(s, cont_size) != segment.continuation)
            {
                segment.continuation = std::string_view(s + cont_size, continuation_chars * cont_size);
                break;
            }
            ++continuation_chars;
        }

        if (continuation_chars > 1)
        {
            segment.code.remove_suffix((continuation_chars - 1) * cont_size);
            const auto c = segment.continuation.front();
            if (c == '<' || c == '>')
                out.so_si_continuation |= segment.so_si_continuation = true;
        }
    }

    return true;
}
void finish_logical_line(logical_line& out, const logical_line_extractor_args& opts)
{
    if (out.segments.empty())
        return;

    const size_t cont_size = opts.continuation - opts.begin;
    for (size_t i = 1; i < out.segments.size(); ++i)
    {
        auto& s = out.segments[i];
        const auto should_be_space = s.code.substr(0, cont_size);
        const bool contains_non_space = should_be_space.find_first_not_of(' ') != std::string_view::npos;
        out.continuation_error |= s.continuation_error = contains_non_space;
        auto [code_without_cont, utf16_skipped] = skip_chars(s.code, cont_size);
        s.code_offset += code_without_cont.data() - s.code.data();
        s.code_offset_utf16 += utf16_skipped;
        s.code = code_without_cont;
    }
    if (!opts.eof_copy_rules)
    {
        out.missing_next_line = !out.segments.back().continuation.empty();
    }
    else
    {
        auto& ll = out.segments.back();
        if (!ll.continuation.empty())
        {
            ll.ignore = std::string_view(ll.continuation.data(), ll.continuation.size() + ll.ignore.size());
            ll.continuation = std::string_view();
        }
    }
}

bool extract_logical_line(logical_line& out, std::string_view& input, const logical_line_extractor_args& opts)
{
    out.clear();

    if (input.empty())
        return false;

    do
    {
        if (!append_to_logical_line(out, input, opts))
            break;
    } while (!input.empty());

    finish_logical_line(out, opts);

    return true;
}
} // namespace hlasm_plugin::parser_library::lexing
