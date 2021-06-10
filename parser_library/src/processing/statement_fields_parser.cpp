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

#include "hlasmparser.h"
#include "lexing/token_stream.h"
#include "parsing/error_strategy.h"
#include "parsing/parser_error_listener_ctx.h"

namespace hlasm_plugin::parser_library::processing {

statement_fields_parser::statement_fields_parser(context::hlasm_context* hlasm_ctx)
    : m_parser(parsing::parser_holder::create(nullptr))
    , m_hlasm_ctx(hlasm_ctx)
{}


statement_fields_parser::~statement_fields_parser() = default;


const parsing::parser_holder& statement_fields_parser::prepare_parser(const std::string& text,
    bool unlimited_line,
    semantics::range_provider field_range,
    processing::processing_status status,
    parsing::parser_error_listener_ctx& err_listener)
{
    m_parser->input->reset(text);

    m_parser->lex->reset();
    m_parser->lex->set_file_offset(field_range.original_range.start);
    m_parser->lex->set_unlimited_line(unlimited_line);

    m_parser->stream->reset();

    m_parser->parser->reinitialize(m_hlasm_ctx, std::move(field_range), status, &err_listener);
    m_parser->parser->removeErrorListeners();
    m_parser->parser->addErrorListener(&err_listener);

    m_parser->parser->reset();

    m_parser->parser->get_collector().prepare_for_next_statement();

    return *m_parser;
}

std::pair<semantics::operands_si, semantics::remarks_si> statement_fields_parser::parse_operand_field(std::string field,
    bool after_substitution,
    semantics::range_provider field_range,
    processing::processing_status status,
    const std::function<void(diagnostic_op)>& add_diag)
{
    m_hlasm_ctx->metrics.reparsed_statements++;

    parsing::parser_error_listener_ctx listener(
        *m_hlasm_ctx, after_substitution ? &field : nullptr, add_diag, field_range);

    const auto original_range = field_range.original_range;

    const auto& h = prepare_parser(field, after_substitution, std::move(field_range), status, listener);

    semantics::op_rem line;
    const auto& [format, opcode] = status;
    if (format.occurence == processing::operand_occurence::ABSENT
        || format.form == processing::processing_form::UNKNOWN)
        h.parser->op_rem_body_noop();
    else
    {
        switch (format.form)
        {
            case processing::processing_form::MAC:
                line = std::move(h.parser->op_rem_body_mac_r()->line);

                if (line.operands.size())
                {
                    auto [to_parse, ranges, r] = join_operands(line.operands);

                    semantics::range_provider tmp_provider(
                        r, std::move(ranges), semantics::adjusting_state::MACRO_REPARSE);

                    const auto& h_second = prepare_parser(to_parse, true, tmp_provider, status, listener);

                    line.operands = std::move(h_second.parser->macro_ops()->list);
                }
                break;
            case processing::processing_form::ASM:
                line = std::move(h.parser->op_rem_body_asm_r()->line);
                break;
            case processing::processing_form::MACH:
                line = std::move(h.parser->op_rem_body_mach_r()->line);
                transform_reloc_imm_operands(line.operands, opcode.value);
                break;
            case processing::processing_form::DAT:
                line = std::move(h.parser->op_rem_body_dat_r()->line);
                break;
            default:
                break;
        }
    }

    collect_diags_from_child(listener);

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

    return std::make_pair(semantics::operands_si(op_range, std::move(line.operands)),
        semantics::remarks_si(rem_range, std::move(line.remarks)));
}

void statement_fields_parser::collect_diags() const {}

} // namespace hlasm_plugin::parser_library::processing
