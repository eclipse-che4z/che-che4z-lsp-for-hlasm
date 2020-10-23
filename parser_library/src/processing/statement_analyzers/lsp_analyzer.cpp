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

#include "lsp_analyzer.h"

#include <algorithm>

#include "processing/context_manager.h"


namespace hlasm_plugin::parser_library::processing {

void lsp_analyzer::analyze(
    const context::hlasm_statement& statement, statement_provider_kind prov_kind, processing_kind proc_kind)
{
    switch (proc_kind)
    {
        case processing_kind::ORDINARY:
            collect_occurences(lsp::occurence_kind::ORD, statement);
            if (prov_kind != statement_provider_kind::MACRO) // macros already processed during macro def processing
            {
                collect_occurences(lsp::occurence_kind::VAR, statement);
                collect_occurences(lsp::occurence_kind::SEQ, statement);
                collect_var_definitions(statement);
            }
            break;
        case processing_kind::MACRO:
            collect_occurences(lsp::occurence_kind::VAR, statement);
            collect_occurences(lsp::occurence_kind::SEQ, statement);
            break;
        default:
            break;
    }
}

void lsp_analyzer::collect_occurences(lsp::occurence_kind kind, const context::hlasm_statement& statement)
{
    lsp::occurence_storage s;
    occurence_collector collector(kind, s);

    if (auto def_stmt = statement.access_deferred(); def_stmt)
    {
        collect_occurence(def_stmt->label_ref(), collector);
        collect_occurence(def_stmt->instruction_ref(), collector);
    }
    else
    {
        auto res_stmt = statement.access_resolved();
        assert(res_stmt);

        collect_occurence(res_stmt->label_ref(), collector);
        collect_occurence(res_stmt->instruction_ref(), collector);
        collect_occurence(res_stmt->operands_ref(), collector);
    }
}

void lsp_analyzer::collect_occurence(const semantics::label_si& label, occurence_collector& collector)
{
    switch (label.type)
    {
        case semantics::label_si_type::CONC:
            collector.get_occurence(std::get<semantics::concat_chain>(label.value));
            break;
        case semantics::label_si_type::ORD:
            collector.get_occurence(hlasm_ctx_.ids().add(std::get<std::string>(label.value)), label.field_range);
            break;
        case semantics::label_si_type::SEQ:
            collector.get_occurence(std::get<semantics::seq_sym>(label.value));
            break;
        case semantics::label_si_type::VAR:
            collector.get_occurence(*std::get<semantics::vs_ptr>(label.value));
            break;
        default:
            break;
    }
}

void lsp_analyzer::collect_occurence(const semantics::instruction_si& instruction, occurence_collector& collector)
{
    if (instruction.type == semantics::instruction_si_type::CONC)
        collector.get_occurence(std::get<semantics::concat_chain>(instruction.value));
}

void lsp_analyzer::collect_occurence(const semantics::operands_si& operands, occurence_collector& collector)
{
    std::for_each(operands.value.begin(), operands.value.end(), [&](const auto& op) { op->apply(collector); });
}

} // namespace hlasm_plugin::parser_library::processing
