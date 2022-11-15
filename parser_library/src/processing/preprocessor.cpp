/*
 * Copyright (c) 2022 Broadcom.
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

#include "preprocessor.h"

#include "lexing/logical_line.h"
#include "semantics/source_info_processor.h"
#include "semantics/statement.h"
#include "utils/unicode_text.h"

namespace hlasm_plugin::parser_library::processing {

preprocessor::line_iterator preprocessor::extract_nonempty_logical_line(
    lexing::logical_line& out, line_iterator it, line_iterator end, const lexing::logical_line_extractor_args& opts)
{
    out.clear();

    while (it != end)
    {
        auto text = it++->text();
        if (!append_to_logical_line(out, text, opts))
            break;
    }

    finish_logical_line(out, opts);

    return it;
}

bool preprocessor::is_continued(std::string_view s)
{
    const auto cont = utils::utf8_substr(s, lexing::default_ictl_copy.end, 1).str;
    return !cont.empty() && cont != " ";
}

void preprocessor::do_highlighting(
    const semantics::preprocessor_statement_si& stmt, semantics::source_info_processor& src_proc)
{
    if (stmt.label_ref().type != semantics::label_si_type::EMPTY)
        src_proc.add_hl_symbol(token_info(stmt.label_ref().field_range, semantics::hl_scopes::label));

    src_proc.add_hl_symbol(token_info(stmt.instruction_ref().field_range, semantics::hl_scopes::instruction));

    for (const auto& op : stmt.operands_ref().value)
    {
        if (op)
            src_proc.add_hl_symbol(token_info(op->operand_range, semantics::hl_scopes::operand));
    }

    if (stmt.remarks_ref().value.size())
        src_proc.add_hl_symbol(token_info(stmt.remarks_ref().field_range, semantics::hl_scopes::remark));
}

} // namespace hlasm_plugin::parser_library::processing
