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
    : m_parser(parsing::parser_holder::create(nullptr, nullptr))
    , m_hlasm_ctx(hlasm_ctx)
{}


statement_fields_parser::~statement_fields_parser() = default;

std::pair<semantics::operands_si, semantics::remarks_si> statement_fields_parser::parse_operand_field(std::string field,
    bool after_substitution,
    semantics::range_provider field_range,
    processing::processing_status status)
{
    m_hlasm_ctx->metrics.reparsed_statements++;
    const auto& h = *m_parser;

    std::optional<std::string> sub;
    if (after_substitution)
        sub = field;
    parsing::parser_error_listener_ctx listener(*m_hlasm_ctx, std::move(sub));

    h.input->reset(field);

    h.lex->reset();
    h.lex->set_file_offset(field_range.original_range.start);
    h.lex->set_unlimited_line(after_substitution);

    h.stream->reset();

    h.parser->reinitialize(m_hlasm_ctx, field_range, status);
    h.parser->setErrorHandler(std::make_shared<parsing::error_strategy>());
    h.parser->removeErrorListeners();
    h.parser->addErrorListener(&listener);
    h.parser->reset();

    h.parser->get_collector().prepare_for_next_statement();

    semantics::op_rem line;
    auto& [format, opcode] = status;
    if (format.occurence == processing::operand_occurence::ABSENT
        || format.form == processing::processing_form::UNKNOWN)
        h.parser->op_rem_body_noop();
    else
    {
        switch (format.form)
        {
            case processing::processing_form::MAC:
                line = std::move(h.parser->op_rem_body_mac_r()->line);
                // proc_status = status;
                if (line.operands.size())
                {
                    size_t string_size = line.operands.size();
                    std::vector<range> ranges;

                    for (auto& op : line.operands)
                        if (auto m_op = dynamic_cast<semantics::macro_operand_string*>(op.get()))
                            string_size += m_op->value.size();

                    std::string to_parse;
                    to_parse.reserve(string_size);

                    for (size_t i = 0; i < line.operands.size(); ++i)
                    {
                        if (auto m_op = dynamic_cast<semantics::macro_operand_string*>(line.operands[i].get()))
                            to_parse.append(m_op->value);
                        if (i != line.operands.size() - 1)
                            to_parse.push_back(',');
                        ranges.push_back(line.operands[i]->operand_range);
                    }
                    auto r = semantics::range_provider::union_range(
                        line.operands.begin()->get()->operand_range, line.operands.back()->operand_range);

                    semantics::range_provider tmp_provider(
                        r, std::move(ranges), semantics::adjusting_state::MACRO_REPARSE);

                    parsing::parser_error_listener_ctx listener(*m_hlasm_ctx, std::move(sub));

                    h.input->reset(std::move(to_parse));

                    h.lex->reset();
                    h.lex->set_file_offset(tmp_provider.original_range.start);
                    h.lex->set_unlimited_line(true);

                    h.stream->reset();

                    h.parser->reinitialize(m_hlasm_ctx, tmp_provider, status);
                    h.parser->setErrorHandler(std::make_shared<parsing::error_strategy>());
                    h.parser->removeErrorListeners();
                    h.parser->addErrorListener(&listener);
                    h.parser->reset();

                    h.parser->get_collector().prepare_for_next_statement();
                    line.operands = std::move(h.parser->macro_ops()->list);

                    collect_diags_from_child(listener);
                }
                break;
            case processing::processing_form::ASM:
                line = std::move(h.parser->op_rem_body_asm_r()->line);
                break;
            case processing::processing_form::MACH:
                line = std::move(h.parser->op_rem_body_mach_r()->line);
                break;
            case processing::processing_form::DAT:
                line = std::move(h.parser->op_rem_body_dat_r()->line);
                break;
            default:
                break;
        }
    }

    collect_diags_from_child(listener);

    for (size_t i = 0; i < line.operands.size(); i++)
    {
        if (!line.operands[i])
            line.operands[i] = std::make_unique<semantics::empty_operand>(field_range.original_range);
    }

    if (line.operands.size() == 1 && line.operands.front()->type == semantics::operand_type::EMPTY)
        line.operands.clear();

    if (after_substitution && line.operands.size() && line.operands.front()->type == semantics::operand_type::MODEL)
        line.operands.clear();

    range op_range = line.operands.empty()
        ? field_range.original_range
        : semantics::range_provider::union_range(
            line.operands.front()->operand_range, line.operands.back()->operand_range);
    range rem_range = line.remarks.empty()
        ? range(op_range.end)
        : semantics::range_provider::union_range(line.remarks.front(), line.remarks.back());

    return std::make_pair(semantics::operands_si(op_range, std::move(line.operands)),
        semantics::remarks_si(rem_range, std::move(line.remarks)));
}

void statement_fields_parser::collect_diags() const { collect_diags_from_child(*m_parser->parser); }

} // namespace hlasm_plugin::parser_library::processing
