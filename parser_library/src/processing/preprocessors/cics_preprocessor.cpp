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

public:
    cics_preprocessor(const cics_preprocessor_options& options, library_fetcher libs, diagnostic_op_consumer* diags)
        : m_libs(std::move(libs))
        , m_diags(diags)
        , m_options(options)
    {}

    void inject_no_end_warning() { m_buffer.append("         DFHEIMSG 4 \n"); }

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

    /* returns number of consumed lines */
    size_t fill_buffer(std::string_view& input, size_t lineno)
    {
        if (input.empty())
        {
            if (!std::exchange(m_end_seen, true))
                inject_no_end_warning();
            return 0;
        }

        if (lineno == 0 && try_asm_xopts(input, lineno))
            return 0;

        return 0;
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
