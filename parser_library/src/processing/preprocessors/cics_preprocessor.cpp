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

#include <cassert>
#include <charconv>
#include <regex>
#include <utility>

#include "lexing/logical_line.h"
#include "preprocessor_options.h"
#include "processing/preprocessor.h"
#include "workspaces/parse_lib_provider.h"

namespace hlasm_plugin::parser_library::processing {

namespace {

class cics_preprocessor : public preprocessor
{
    const char* m_last_position = nullptr;
    lexing::logical_line m_logical_line;
    std::string m_operands;
    library_fetcher m_libs;
    diagnostic_op_consumer* m_diags = nullptr;
    std::string m_buffer;
    cics_preprocessor_options m_options;
    bool m_end_seen = false;
    bool m_global_macro_called = false;
    bool m_pendig_prolog = false;

public:
    cics_preprocessor(const cics_preprocessor_options& options, library_fetcher libs, diagnostic_op_consumer* diags)
        : m_libs(std::move(libs))
        , m_diags(diags)
        , m_options(options)
    {}

    void inject_no_end_warning()
    {
        m_buffer.append("*DFH7041I W  NO END CARD FOUND - COPYBOOK ASSUMED.\n"
                        "         DFHEIMSG 4\n");
    }

    void inject_DFHEIGBL(bool rsect)
    {
        if (rsect)
        {
            if (m_options.leasm)
                m_buffer.append("         DFHEIGBL ,,RS,LE          INSERTED BY TRANSLATOR\n");
            else
                m_buffer.append("         DFHEIGBL ,,RS,NOLE        INSERTED BY TRANSLATOR\n");
        }
        else
        {
            if (m_options.leasm)
                m_buffer.append("         DFHEIGBL ,,,LE            INSERTED BY TRANSLATOR\n");
            else
                m_buffer.append("         DFHEIGBL ,,,NOLE          INSERTED BY TRANSLATOR\n");
        }
    }

    void inject_prolog() { m_buffer.append("         DFHEIENT                  INSERTED BY TRANSLATOR\n"); }
    void inject_end_code()
    {
        if (m_options.epilog)
            m_buffer.append("         DFHEIRET                  INSERTED BY TRANSLATOR\n");
        if (m_options.prolog)
        {
            m_buffer.append("         DFHEISTG                  INSERTED BY TRANSLATOR\n");
            m_buffer.append("         DFHEIEND                  INSERTED BY TRANSLATOR\n");
        }
    }

    bool try_asm_xopts(std::string_view input, size_t lineno)
    {
        if (input.substr(0, 5) != "*ASM ")
            return false;

        auto [line, _] = lexing::extract_line(input);
        if (m_diags && line.size() > lexing::default_ictl.end && line[lexing::default_ictl.end] != ' ')
            m_diags->add_diagnostic(diagnostic_op::warn_CIC001(range(position(lineno, 0))));

        line = line.substr(0, lexing::default_ictl.end);

        static const std::regex asm_statement(R"(\*ASM[ ]+[Xx][Oo][Pp][Tt][Ss][(']([A-Z, ]*)[)'][ ]*)");
        static const std::regex op_sep("[ ,]+");
        static const std::unordered_map<std::string_view, std::pair<bool cics_preprocessor_options::*, bool>> opts {
            { "PROLOG", { &cics_preprocessor_options::prolog, true } },
            { "NOPROLOG", { &cics_preprocessor_options::prolog, false } },
            { "EPILOG", { &cics_preprocessor_options::epilog, true } },
            { "NOEPILOG", { &cics_preprocessor_options::epilog, false } },
            { "LEASM", { &cics_preprocessor_options::leasm, true } },
            { "NOLEASM", { &cics_preprocessor_options::leasm, false } },
        };

        std::match_results<std::string_view::const_iterator> m_regex_match;
        if (!std::regex_match(line.begin(), line.end(), m_regex_match, asm_statement) || m_regex_match[1].length() == 0)
            return false;

        std::string_view operands = std::string_view(&*m_regex_match[1].first, (size_t)m_regex_match[1].length());
        auto opts_begin =
            std::regex_token_iterator<std::string_view::iterator>(operands.begin(), operands.end(), op_sep, -1);
        auto opts_end = std::regex_token_iterator<std::string_view::iterator>();

        for (; opts_begin != opts_end; ++opts_begin)
        {
            if (opts_begin->length() == 0)
                continue;

            std::string_view name(&*opts_begin->first, opts_begin->second - opts_begin->first);
            if (auto o = opts.find(name); o != opts.end())
                (m_options.*o->second.first) = o->second.second;
        }

        return true;
    }

    static constexpr size_t valid_cols = 1 + lexing::default_ictl.end - (lexing::default_ictl.begin - 1);
    static auto create_line_preview(std::string_view input)
    {
        return lexing::utf8_substr(lexing::extract_line(input).first, lexing::default_ictl.begin - 1, valid_cols);
    }

    static bool ignore_line(std::string_view s) { return s.empty() || s.front() == '*' || s.substr(0, 2) == ".*"; }

    /* returns number of consumed lines */
    size_t fill_buffer(std::string_view& input, size_t lineno)
    {
        if (std::exchange(m_pendig_prolog, false))
            inject_prolog();
        if (input.empty())
        {
            if (!std::exchange(m_end_seen, true))
                inject_no_end_warning();
            return 0;
        }

        if (lineno == 0 && try_asm_xopts(input, lineno))
            return 0;

        auto [line, line_len_chars, _] = create_line_preview(input);

        if (ignore_line(line))
            return 0;

        // apparently lines full of characters are ignored
        if (line_len_chars == valid_cols && line.find(' ') == std::string_view::npos)
            return 0;

        static const std::regex line_of_interest(
            "([^ ]*)[ ]+(START|CSECT|RSECT|END|[eE][xX][eE][cC][ ]+[cC][iI][cC][sS])(?: .+)?");

        std::match_results<std::string_view::iterator> matches;
        if (!std::regex_match(line.begin(), line.end(), matches, line_of_interest))
            return 0;

        auto label = matches[1].length() ? std::string_view(&*matches[1].first, matches[1].second - matches[1].first)
                                         : std::string_view();
        switch (*matches[2].first)
        {
            case 'S':
            case 'C':
                if (!std::exchange(m_global_macro_called, true))
                    inject_DFHEIGBL(false);
                m_pendig_prolog = m_options.prolog;
                return 0;

            case 'R':
                m_global_macro_called = true;
                inject_DFHEIGBL(true);
                m_pendig_prolog = m_options.prolog;
                return 0;

            case 'e':
            case 'E':
                if (matches[2].first[1] == 'N')
                {
                    inject_end_code();
                    return 0;
                }
                break;

            default:
                assert(false);
                break;
        }

        static constexpr const lexing::logical_line_extractor_args cics_extract { 1, 71, 2, false, false };
        m_logical_line.clear();
        bool extracted = lexing::extract_logical_line(m_logical_line, input, cics_extract);
        assert(extracted);

        // print the first line, remove continuation character
        auto buf_len = m_buffer.size();
        m_buffer.append(lexing::utf8_substr(m_logical_line.segments.front().line, 0, cics_extract.end).str);
        if (auto after_cont = lexing::utf8_substr(m_logical_line.segments.front().line, cics_extract.end + 1).str;
            !after_cont.empty())
            m_buffer.append(" ").append(after_cont);
        m_buffer.replace(buf_len, label.size(), lexing::utf8_substr(label).char_count, ' ');
        m_buffer[buf_len] = '*';
        m_buffer.append("\n");

        if (auto char_count = lexing::utf8_substr(label).char_count; char_count <= 8)
            m_buffer.append(label).append(9 - char_count, ' ');
        else
            m_buffer.append(label).append(" DS 0H\n").append(9, ' ');

        m_buffer.append("DFHECALL =X'0E'\n"); // TODO: generate correct calls

        if (m_logical_line.continuation_error)
            m_buffer
                .append("*DFH7080I W  CONTINUATION OF EXEC COMMAND IGNORED.\n"
                        "         DFHEIMSG 4\n")
                .append("*DFH7080I W  CONTINUATION OF EXEC COMMAND IGNORED.\n"
                        "         DFHEIMSG 4\n"); // seems to be done twice

        for (auto it = m_logical_line.segments.begin() + 1; it != m_logical_line.segments.end(); ++it)
        {
            const auto& l = *it;
            buf_len = m_buffer.size();
            m_buffer.append(lexing::utf8_substr(l.line, 0, cics_extract.end).str);
            if (auto after_cont = lexing::utf8_substr(l.line, cics_extract.end + 1).str; !after_cont.empty())
                m_buffer.append(" ").append(after_cont);
            if (!m_logical_line.continuation_error)
                m_buffer[buf_len] = '*';
            m_buffer.append("\n");
        }

        return m_logical_line.segments.size();
    }

    // Inherited via preprocessor
    std::optional<std::string> generate_replacement(std::string_view& input, size_t& lineno) override
    {
        if (input.data() == m_last_position)
            return std::nullopt;

        m_buffer.clear();
        if (std::exchange(m_last_position, input.data()) == nullptr)
        {
            // nothing so far
        }

        lineno += fill_buffer(input, lineno);
        if (m_buffer.size())
            return m_buffer;
        else
            return std::nullopt;
    }

    bool finished() const override { return !m_pendig_prolog; }

    cics_preprocessor_options current_options() const { return m_options; }
};

} // namespace

std::unique_ptr<preprocessor> preprocessor::create(
    const cics_preprocessor_options& options, library_fetcher libs, diagnostic_op_consumer* diags)
{
    return std::make_unique<cics_preprocessor>(options, std::move(libs), diags);
}

cics_preprocessor_options test_cics_current_options(const preprocessor& p)
{
    return static_cast<const cics_preprocessor&>(p).current_options();
}

} // namespace hlasm_plugin::parser_library::processing
