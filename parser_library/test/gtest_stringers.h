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

#ifndef HLASMPLUGIN_PARSERLIBRARY_TEST_GTEST_STRINGERS_H
#define HLASMPLUGIN_PARSERLIBRARY_TEST_GTEST_STRINGERS_H

#include "gtest/gtest.h"

#include "analyzer.h"

// This file contains ostream operator<< overloads to improve GTest error reporting.


namespace hlasm_plugin::parser_library {

inline std::ostream& operator<<(std::ostream& stream, const position& item)
{
    return stream << "{ " << item.line << ", " << item.column << " }";
}

inline std::ostream& operator<<(std::ostream& stream, range item)
{
    return stream << "{ " << item.start << ", " << item.end << " }";
}

inline std::ostream& operator<<(std::ostream& stream, const location& item)
{
    return stream << "{ file: " << item.file << "\n position: " << item.pos << " }";
}

} // namespace hlasm_plugin::parser_library

namespace hlasm_plugin::parser_library::lsp {

inline std::ostream& operator<<(std::ostream& stream, const symbol_occurence& item)
{
    return stream << "{ kind: " << (int)item.kind << "\n name: " << *item.name << "\n range: " << item.occurence_range
                  << " }";
}

inline std::ostream& operator<<(std::ostream& stream, const lsp::completion_item_s& item)
{
    return stream << "{ label: " << item.label << "\n detail: " << item.detail << "\n insert text: " << item.insert_text
                  << "\n documentation: " << item.documentation << "\n kind: " << (int)item.kind << " }";
}

} // namespace hlasm_plugin::parser_library::lsp


#endif
