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

public:
    cics_preprocessor(library_fetcher libs, diagnostic_op_consumer* diags)
        : m_libs(std::move(libs))
        , m_diags(diags)
    {}

    // Inherited via preprocessor
    std::optional<std::string> generate_replacement(std::string_view& input, size_t& lineno) override
    {
        return std::optional<std::string>();
    }
};

} // namespace

std::unique_ptr<preprocessor> preprocessor::create(
    const cics_preprocessor_options&, library_fetcher libs, diagnostic_op_consumer* diags)
{
    return std::make_unique<cics_preprocessor>(std::move(libs), diags);
}

} // namespace hlasm_plugin::parser_library::processing