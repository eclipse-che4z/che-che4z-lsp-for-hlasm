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

#ifndef HLASMPLUGIN_PARSERLIBRARY_TEST_LSP_ANALYZER_FIXTURE_H
#define HLASMPLUGIN_PARSERLIBRARY_TEST_LSP_ANALYZER_FIXTURE_H

#include "gtest/gtest.h"

#include "analyzer.h"

namespace hlasm_plugin::parser_library {

std::ostream& operator<<(std::ostream& stream, const lsp::completion_item_s& item)
{
    return stream << "{ label: " << item.label << "\n detail: " << item.detail << "\n insert text: " << item.insert_text
                  << "\n documentation: " << item.documentation << "\n kind: " << (int)item.kind << " }";
}

std::ostream& operator<<(std::ostream& stream, const position& item)
{
    return stream << "{ " << item.line << ", " << item.column << " }";
}

std::ostream& operator<<(std::ostream& stream, const location& item)
{
    return stream << "{ file: " << item.file << "\n position: " << item.pos << " }";
}

struct analyzer_fixture : public ::testing::Test
{
    const static inline std::string dummy_file_name = "source";
    analyzer a;
    analyzer_fixture(const std::string& input)
        : a(input, dummy_file_name)
    {
        a.analyze();
    }
};

} // namespace hlasm_plugin::parser_library

#endif