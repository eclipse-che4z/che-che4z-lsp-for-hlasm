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

#include "endevor_analyzer.h"

namespace hlasm_plugin::parser_library::processing {

endevor_analyzer::endevor_analyzer(analyzing_context ctx, const std::string& text)
    : m_ctx(ctx)
    , m_text(text)
{}

void endevor_analyzer::analyze(
    const context::hlasm_statement& statement, statement_provider_kind prov_kind, processing_kind proc_kind)
{
    auto preproc_stmt = statement.access_preproc();
    if (!preproc_stmt)
        return;

    auto endevor_stmt = dynamic_cast<const semantics::endevor_statement*>(preproc_stmt);
    if (!endevor_stmt)
        return;

    analyze_preproc(*endevor_stmt, prov_kind, proc_kind);

    auto& file_occs = m_opencode_occurences[m_ctx.hlasm_ctx->current_statement_location().resource_loc];
    file_occs.insert(
        file_occs.end(), std::move_iterator(m_stmt_occurences.begin()), std::move_iterator(m_stmt_occurences.end()));

    m_ctx.lsp_ctx->add_opencode(
        std::make_unique<lsp::opencode_info>(std::move(m_opencode_var_defs), std::move(m_opencode_occurences)),
        lsp::text_data_ref_t(m_text));
}

void endevor_analyzer::analyze_preproc(
    const semantics::endevor_statement& statement, statement_provider_kind prov_kind, processing_kind proc_kind)
{
    occurence_collector collector(lsp::occurence_kind::INSTR, *m_ctx.hlasm_ctx, m_stmt_occurences);
    collect_occurence(statement.instruction_ref(), collector);

    auto sym_expr = dynamic_cast<expressions::mach_expr_symbol*>(
        statement.operands_ref().value.front()->access_asm()->access_expr()->expression.get());

    if (sym_expr)
        add_copy_operand(sym_expr->value, sym_expr->get_range());
}

void endevor_analyzer::add_copy_operand(context::id_index name, const range& operand_range)
{
    // find ORD occurrence of COPY_OP
    lsp::symbol_occurence occ(lsp::occurence_kind::ORD, name, operand_range);
    auto ord_sym = std::find(m_stmt_occurences.begin(), m_stmt_occurences.end(), occ);

    if (ord_sym != m_stmt_occurences.end())
        ord_sym->kind = lsp::occurence_kind::COPY_OP;
    else
        m_stmt_occurences.emplace_back(lsp::occurence_kind::COPY_OP, name, operand_range);
}

void endevor_analyzer::collect_occurence(const semantics::instruction_si& instruction, occurence_collector& collector)
{
    if (instruction.type == semantics::instruction_si_type::CONC)
        collector.get_occurence(std::get<semantics::concat_chain>(instruction.value));
    else if (instruction.type == semantics::instruction_si_type::ORD
        && collector.collector_kind == lsp::occurence_kind::INSTR)
    {
        auto opcode = m_ctx.hlasm_ctx->get_operation_code(
            std::get<context::id_index>(instruction.value)); // TODO: collect the instruction at the right time
        auto* macro_def = std::get_if<context::macro_def_ptr>(&opcode.opcode_detail);
        if (opcode.opcode || macro_def)
            collector.occurences.emplace_back(
                opcode.opcode, macro_def ? std::move(*macro_def) : context::macro_def_ptr {}, instruction.field_range);
    }
}


} // namespace hlasm_plugin::parser_library::processing