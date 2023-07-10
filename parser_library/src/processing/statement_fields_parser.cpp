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
#include "lexing/token_stream.h"
#include "parsing/error_strategy.h"
#include "parsing/parser_impl.h"
#include "semantics/operand_impls.h"

namespace hlasm_plugin::parser_library::processing {

statement_fields_parser::statement_fields_parser(context::hlasm_context* hlasm_ctx)
    : m_parser_singleline(parsing::parser_holder::create(nullptr, hlasm_ctx, nullptr, false))
    , m_parser_multiline(parsing::parser_holder::create(nullptr, hlasm_ctx, nullptr, true))
    , m_hlasm_ctx(hlasm_ctx)
{}

statement_fields_parser::~statement_fields_parser() = default;

constexpr bool is_multiline(std::string_view v)
{
    auto nl = v.find_first_of("\r\n");
    if (nl == std::string_view::npos)
        return false;
    v.remove_prefix(nl);
    v.remove_prefix(1 + v.starts_with("\r\n"));

    return !v.empty();
}

statement_fields_parser::parse_result statement_fields_parser::parse_operand_field(std::string field,
    bool after_substitution,
    semantics::range_provider field_range,
    size_t logical_column,
    processing::processing_status status,
    diagnostic_op_consumer& add_diag)
{
    m_hlasm_ctx->metrics.reparsed_statements++;

    const auto original_range = field_range.original_range;

    diagnostic_consumer_transform add_diag_subst([&field, &add_diag, after_substitution](diagnostic_op diag) {
        if (after_substitution)
            diag.message = diagnostic_decorate_message(field, diag.message);
        add_diag.add_diagnostic(std::move(diag));
    });
    const auto& h = is_multiline(field) ? *m_parser_multiline : *m_parser_singleline;
    h.prepare_parser(field,
        m_hlasm_ctx,
        &add_diag_subst,
        std::move(field_range),
        original_range,
        logical_column,
        status,
        after_substitution);

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
            case processing::processing_form::MAC:
                line = h.op_rem_body_mac_r();
                literals = h.parser->get_collector().take_literals();

                if (h.error_handler->error_reported())
                {
                    line.operands.clear();
                }
                else if (line.operands.size())
                {
                    auto [to_parse, ranges, r] = join_operands(line.operands);

                    const auto& h_second = *m_parser_singleline;
                    h_second.prepare_parser(to_parse,
                        m_hlasm_ctx,
                        &add_diag_subst,
                        semantics::range_provider(
                            r, std::move(ranges), semantics::adjusting_state::MACRO_REPARSE, h.lex->get_line_limits()),
                        original_range,
                        logical_column,
                        status,
                        true);

                    line.operands = h_second.macro_ops();
                    literals = h.parser->get_collector().take_literals();
                }
                break;
            case processing::processing_form::ASM:
                line = h.op_rem_body_asm_r();
                literals = h.parser->get_collector().take_literals();
                break;
            case processing::processing_form::MACH:
                line = h.op_rem_body_mach_r();
                transform_reloc_imm_operands(line.operands, opcode.value);
                literals = h.parser->get_collector().take_literals();
                break;
            case processing::processing_form::DAT:
                line = h.op_rem_body_dat_r();
                literals = h.parser->get_collector().take_literals();
                break;
            default:
                break;
        }
    }

    for (auto& op : line.operands)
    {
        if (!op)
            op = std::make_unique<semantics::empty_operand>(original_range);
    }

    if (line.operands.size() == 1 && line.operands.front()->type == semantics::operand_type::EMPTY)
        line.operands.clear();

    if (after_substitution && line.operands.size() && line.operands.front()->type == semantics::operand_type::MODEL)
        line.operands.clear();

    range op_range = line.operands.empty()
        ? original_range
        : union_range(line.operands.front()->operand_range, line.operands.back()->operand_range);
    range rem_range =
        line.remarks.empty() ? range(op_range.end) : union_range(line.remarks.front(), line.remarks.back());

    return parse_result {
        semantics::operands_si(op_range, std::move(line.operands)),
        semantics::remarks_si(rem_range, std::move(line.remarks)),
        std::move(literals),
    };
}

void statement_fields_parser::collect_diags() const {}

} // namespace hlasm_plugin::parser_library::processing
