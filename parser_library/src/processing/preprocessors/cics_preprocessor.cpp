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
    bool seen_end = false;

public:
    cics_preprocessor(const cics_preprocessor_options& options, library_fetcher libs, diagnostic_op_consumer* diags)
        : m_libs(std::move(libs))
        , m_diags(diags)
        , m_options(options)
    {}


    /* returns number of consumed lines */
    size_t fill_buffer(std::string_view& input, size_t lineno)
    {
        if (input.empty()) {}
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
};

} // namespace

std::unique_ptr<preprocessor> preprocessor::create(
    const cics_preprocessor_options& options, library_fetcher libs, diagnostic_op_consumer* diags)
{
    return std::make_unique<cics_preprocessor>(options, std::move(libs), diags);
}

} // namespace hlasm_plugin::parser_library::processing