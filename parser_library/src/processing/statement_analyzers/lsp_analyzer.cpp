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

#include "processing/statement_processors/macrodef_processor.h"


namespace hlasm_plugin::parser_library::processing {

lsp_analyzer::lsp_analyzer(context::hlasm_context& hlasm_ctx, lsp::lsp_context& lsp_ctx)
    : hlasm_ctx_(hlasm_ctx)
    , lsp_ctx_(lsp_ctx)
    , in_macro_(false)
{}

void lsp_analyzer::analyze(
    const context::hlasm_statement& statement, statement_provider_kind prov_kind, processing_kind proc_kind)
{
    switch (proc_kind)
    {
        case processing_kind::ORDINARY:
            collect_occurences(lsp::occurence_kind::ORD, statement);
            collect_occurences(lsp::occurence_kind::INSTR, statement);
            if (prov_kind != statement_provider_kind::MACRO) // macros already processed during macro def processing
            {
                collect_occurences(lsp::occurence_kind::VAR, statement);
                collect_occurences(lsp::occurence_kind::SEQ, statement);
                collect_var_definition(statement);
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

void lsp_analyzer::macrodef_started(const macrodef_start_data&) { in_macro_ = true; }

void lsp_analyzer::macrodef_finished(context::macro_def_ptr macrodef, macrodef_processing_result&& result)
{
    if (!result.invalid)
        lsp_ctx_.add_macro(std::make_shared<lsp::macro_info>(result.external,
            std::move(macrodef),
            std::move(result.variable_symbols),
            std::move(result.file_scopes),
            std::move(macro_occurences_)));

    in_macro_ = false;
    macro_occurences_.clear();
}

void lsp_analyzer::copydef_started(const copy_start_data&) {}

void lsp_analyzer::copydef_finished(context::copy_member_ptr copydef, copy_processing_result&&)
{
    lsp_ctx_.add_copy(std::move(copydef));
}

void lsp_analyzer::opencode_finished()
{
    lsp_ctx_.add_opencode(std::make_unique<lsp::opencode_info>(
        hlasm_ctx_, std::move(opencode_var_defs_), std::move(opencode_occurences_)));
}

void lsp_analyzer::collect_occurences(lsp::occurence_kind kind, const context::hlasm_statement& statement)
{
    lsp::occurence_storage occs;
    occurence_collector collector(kind, hlasm_ctx_, occs);

    if (auto def_stmt = statement.access_deferred(); def_stmt)
    {
        collect_occurence(def_stmt->label_ref(), collector);
        collect_occurence(def_stmt->instruction_ref(), collector);
        collect_occurence(def_stmt->deferred_ref(), collector);
    }
    else
    {
        auto res_stmt = statement.access_resolved();
        assert(res_stmt);

        collect_occurence(res_stmt->label_ref(), collector);
        collect_occurence(res_stmt->instruction_ref(), collector);
        collect_occurence(res_stmt->operands_ref(), collector);
    }

    if (in_macro_)
    {
        auto& file_occs = macro_occurences_[hlasm_ctx_.current_statement_location().file];
        file_occs.insert(file_occs.end(), std::move_iterator(occs.begin()), std::move_iterator(occs.end()));
    }
    else
    {
        auto& file_occs = opencode_occurences_[hlasm_ctx_.current_statement_location().file];
        file_occs.insert(file_occs.end(), std::move_iterator(occs.begin()), std::move_iterator(occs.end()));
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
    else if (instruction.type == semantics::instruction_si_type::ORD
        && collector.collector_kind == lsp::occurence_kind::INSTR)
    {
        auto opcode = hlasm_ctx_.get_operation_code(std::get<context::id_index>(instruction.value));
        collector.occurences.emplace_back(opcode.machine_opcode, opcode.macro_opcode, instruction.field_range);
    }
}

void lsp_analyzer::collect_occurence(const semantics::operands_si& operands, occurence_collector& collector)
{
    std::for_each(operands.value.begin(), operands.value.end(), [&](const auto& op) { op->apply(collector); });
}

void lsp_analyzer::collect_occurence(const semantics::deferred_operands_si& operands, occurence_collector& collector)
{
    std::for_each(operands.vars.begin(), operands.vars.end(), [&](const auto& v) { collector.get_occurence(*v); });
}

#define IF_LCL_GBL(X, Y, Z)                                                                                            \
    if (code.value == ctx.ids().add(#X))                                                                               \
    {                                                                                                                  \
        set_type = context::SET_t_enum::Y;                                                                             \
        global = Z;                                                                                                    \
        return true;                                                                                                   \
    }
#define IF_SET(X, Y)                                                                                                   \
    if (code.value == ctx.ids().add(#X))                                                                               \
    {                                                                                                                  \
        set_type = context::SET_t_enum::Y;                                                                             \
        return true;                                                                                                   \
    }

bool is_LCL_GBL(const processing::resolved_statement& statement,
    context::hlasm_context& ctx,
    context::SET_t_enum& set_type,
    bool& global)
{
    const auto& code = statement.opcode_ref();

    IF_LCL_GBL(LCLA, A_TYPE, false);
    IF_LCL_GBL(LCLB, B_TYPE, false);
    IF_LCL_GBL(LCLC, C_TYPE, false);
    IF_LCL_GBL(GBLA, A_TYPE, true);
    IF_LCL_GBL(GBLB, B_TYPE, true);
    IF_LCL_GBL(GBLC, C_TYPE, true);
    return false;
}

bool is_SET(const processing::resolved_statement& statement, context::hlasm_context& ctx, context::SET_t_enum& set_type)
{
    const auto& code = statement.opcode_ref();

    IF_SET(SETA, A_TYPE);
    IF_SET(SETB, B_TYPE);
    IF_SET(SETC, C_TYPE);
    return false;
}

void lsp_analyzer::collect_var_definition(const context::hlasm_statement& statement)
{
    auto res_stmt = statement.access_resolved();
    if (!res_stmt)
        return;

    bool global;
    context::SET_t_enum type;
    if (is_SET(*res_stmt, hlasm_ctx_, type))
        collect_SET_defs(*res_stmt, type);
    else if (is_LCL_GBL(*res_stmt, hlasm_ctx_, type, global))
        collect_LCL_GBL_defs(*res_stmt, type, global);
}

void lsp_analyzer::collect_SET_defs(const processing::resolved_statement& statement, context::SET_t_enum type)
{
    if (statement.label_ref().type != semantics::label_si_type::VAR)
        return;

    auto var = std::get<semantics::vs_ptr>(statement.label_ref().value).get();

    add_var_def(var, type, false);
}

void lsp_analyzer::collect_LCL_GBL_defs(
    const processing::resolved_statement& statement, context::SET_t_enum type, bool global)
{
    for (auto& op : statement.operands_ref().value)
    {
        if (op->type != semantics::operand_type::EMPTY)
            continue;

        auto ca_op = op->access_ca();
        assert(ca_op);

        if (ca_op->kind == semantics::ca_kind::VAR)
        {
            auto var = ca_op->access_var()->variable_symbol.get();

            add_var_def(var, type, global);
        }
    }
}

void lsp_analyzer::add_var_def(const semantics::variable_symbol* var, context::SET_t_enum type, bool global)
{
    if (var->created)
        return;

    if (std::find_if(opencode_var_defs_.begin(),
            opencode_var_defs_.end(),
            [&](const auto& def) { return def.name == var->access_basic()->name; })
        != opencode_var_defs_.end())
        return;

    opencode_var_defs_.emplace_back(
        var->access_basic()->name, type, global, hlasm_ctx_.current_statement_location().file, var->symbol_range.start);
}

} // namespace hlasm_plugin::parser_library::processing
