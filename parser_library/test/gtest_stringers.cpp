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

#include "gtest_stringers.h"

#include <ostream>

#include "location.h"
#include "lsp/completion_item.h"
#include "lsp/symbol_occurrence.h"
#include "protocol.h"

namespace hlasm_plugin::parser_library {

std::ostream& operator<<(std::ostream& stream, const position& item)
{
    return stream << "{ " << item.line << ", " << item.column << " }";
}

std::ostream& operator<<(std::ostream& stream, range item)
{
    return stream << "{ " << item.start << ", " << item.end << " }";
}

std::ostream& operator<<(std::ostream& stream, const location& item)
{
    return stream << "{ file: " << item.get_uri() << "\n position: " << item.pos << " }";
}

std::ostream& operator<<(std::ostream& stream, const performance_metrics& item)
{
    return stream << "continued statements: " << item.continued_statements
                  << "\n copy def statements: " << item.copy_def_statements
                  << "\n copy statements: " << item.copy_statements << "\n lines: " << item.lines
                  << "\n lookahead statements: " << item.lookahead_statements
                  << "\n macro def statements: " << item.macro_def_statements
                  << "\n macro statements: " << item.macro_statements
                  << "\n non continued statements: " << item.non_continued_statements
                  << "\n open code statements: " << item.open_code_statements
                  << "\n reparsed statements: " << item.reparsed_statements << "\n";
}

} // namespace hlasm_plugin::parser_library

namespace hlasm_plugin::parser_library::lsp {

std::ostream& operator<<(std::ostream& stream, const symbol_occurrence& item)
{
    return stream << "{ kind: " << (int)item.kind << "\n name: " << item.name.to_string_view()
                  << "\n range: " << item.occurrence_range << " }";
}

std::ostream& operator<<(std::ostream& stream, const lsp::completion_item_s& item)
{
    return stream << "{ label: " << item.label << "\n detail: " << item.detail << "\n insert text: " << item.insert_text
                  << "\n documentation: " << item.documentation << "\n kind: " << (int)item.kind << " }";
}

} // namespace hlasm_plugin::parser_library::lsp
