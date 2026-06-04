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

#include "statement_fields_parser.h"

#include "context/hlasm_context.h"
#include "lexing/string_with_newlines.h"
#include "parsing/parser_impl.h"
#include "semantics/operand_impls.h"

namespace hlasm_plugin::parser_library::processing {

statement_fields_parser::statement_fields_parser(context::hlasm_context& hlasm_ctx)
    : m_parser(std::make_unique<parsing::parser_holder>(hlasm_ctx, nullptr))
    , m_hlasm_ctx(&hlasm_ctx)
{}

statement_fields_parser::~statement_fields_parser() = default;

statement_fields_parser::parse_result statement_fields_parser::parse_operand_field(
    lexing::u8string_view_with_newlines field,
    bool after_substitution,
    semantics::range_provider field_range,
    size_t logical_column,
    processing::processing_status status,
    diagnostic_op_consumer& add_diag)
{
    m_hlasm_ctx->metrics.reparsed_statements++;

    const auto original_range = field_range.original_range;

    diagnostic_consumer_transform add_diag_subst([&field, &add_diag, after_substitution](diagnostic_op diag) {
        if (after_substitution) // field.text has not newlines
            diag.message = diagnostic_decorate_message(field.text, diag.message);
        add_diag.add_diagnostic(std::move(diag));
    });
    auto& h = *m_parser;
    h.prepare_parser(
        field, *m_hlasm_ctx, &add_diag_subst, std::move(field_range), original_range, logical_column, status);

    semantics::op_rem line;
    std::vector<semantics::literal_si> literals;

    const auto& [format, opcode] = status;
    if (format.occurrence == processing::operand_occurrence::ABSENT
        || format.form == processing::processing_form::UNKNOWN)
        h.op_rem_body_noop();
    else
    {
        switch (format.form)
        {
            case processing::processing_form::MAC: {
                line.operands = h.macro_ops(true);
                literals = h.collector.take_literals();
                break;
            }
            case processing::processing_form::ASM_GENERIC_ORD:
            case processing::processing_form::ASM_GENERIC_TEXT:
            case processing::processing_form::ASM_ALIAS:
            case processing::processing_form::ASM_END:
            case processing::processing_form::ASM_USING:
                if (auto ops = h.op_rem_body_asm(format.form, true, !after_substitution); ops)
                    line = std::move(*ops);
                literals = h.collector.take_literals();
                break;
            case processing::processing_form::MACH:
                if (auto ops = h.op_rem_body_mach(true, !after_substitution); ops)
                    line = std::move(*ops);
                transform_reloc_imm_operands(line.operands, opcode);
                literals = h.collector.take_literals();
                break;
            case processing::processing_form::DAT:
                if (auto ops = h.op_rem_body_dat(true, !after_substitution); ops)
                    line = std::move(*ops);
                literals = h.collector.take_literals();
                break;
            default:
                break;
        }
    }

    assert(std::ranges::all_of(line.operands, [](const auto& p) { return !!p; }));
    assert(line.operands.size() != 1 || line.operands.front()->type != semantics::operand_type::EMPTY);

    if (after_substitution && line.operands.size() && line.operands.front()->type == semantics::operand_type::MODEL)
        line.operands.clear();

    range op_range = line.operands.empty()
        ? original_range
        : union_range(line.operands.front()->operand_range, line.operands.back()->operand_range);

    return parse_result {
        semantics::operands_si(op_range, std::move(line.operands)),
        semantics::remarks_si(std::move(line.remarks)),
        std::move(literals),
    };
}

} // namespace hlasm_plugin::parser_library::processing
