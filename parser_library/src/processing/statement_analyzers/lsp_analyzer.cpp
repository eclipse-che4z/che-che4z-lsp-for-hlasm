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

lsp_analyzer::lsp_analyzer(context::hlasm_context& hlasm_ctx, lsp::lsp_context& lsp_ctx, const std::string& file_text)
    : hlasm_ctx_(hlasm_ctx)
    , lsp_ctx_(lsp_ctx)
    , file_text_(file_text)
    , LCL_GBL_instructions_ { { { hlasm_ctx.ids().well_known.LCLA, context::SET_t_enum::A_TYPE, false },
          { hlasm_ctx.ids().well_known.LCLB, context::SET_t_enum::B_TYPE, false },
          { hlasm_ctx.ids().well_known.LCLC, context::SET_t_enum::C_TYPE, false },
          { hlasm_ctx.ids().well_known.GBLA, context::SET_t_enum::A_TYPE, true },
          { hlasm_ctx.ids().well_known.GBLB, context::SET_t_enum::B_TYPE, true },
          { hlasm_ctx.ids().well_known.GBLC, context::SET_t_enum::C_TYPE, true } } }
    , SET_instructions_ { { { hlasm_ctx.ids().well_known.SETA, context::SET_t_enum::A_TYPE },
          { hlasm_ctx.ids().well_known.SETB, context::SET_t_enum::B_TYPE },
          { hlasm_ctx.ids().well_known.SETC, context::SET_t_enum::C_TYPE } } }
{}

void lsp_analyzer::analyze(
    const context::hlasm_statement& statement, statement_provider_kind prov_kind, processing_kind proc_kind)
{
    const auto* resolved_stmt = statement.access_resolved();
    switch (proc_kind)
    {
        case processing_kind::ORDINARY:
            collect_occurences(lsp::occurence_kind::ORD, statement);
            collect_occurences(lsp::occurence_kind::INSTR, statement);
            if (prov_kind != statement_provider_kind::MACRO) // macros already processed during macro def processing
            {
                collect_occurences(lsp::occurence_kind::VAR, statement);
                collect_occurences(lsp::occurence_kind::SEQ, statement);
                if (resolved_stmt)
                {
                    collect_var_definition(*resolved_stmt);
                    collect_copy_operands(*resolved_stmt);
                }
            }
            break;
        case processing_kind::MACRO:
            if (resolved_stmt)
                update_macro_nest(*resolved_stmt);
            if (macro_nest_ > 1)
                break; // Do not collect occurences in nested macros to avoid collecting occurences multiple times
            collect_occurences(lsp::occurence_kind::VAR, statement);
            collect_occurences(lsp::occurence_kind::SEQ, statement);
            if (resolved_stmt)
                collect_copy_operands(*resolved_stmt);
            break;
        default:
            break;
    }

    assign_statement_occurences();
}

void lsp_analyzer::macrodef_started(const macrodef_start_data& data)
{
    in_macro_ = true;
    // For external macros, the macrodef starts before encountering the MACRO statement
    if (data.is_external)
        macro_nest_ = 0;
    else
        macro_nest_ = 1;
}

void lsp_analyzer::macrodef_finished(context::macro_def_ptr macrodef, macrodef_processing_result&& result)
{
    if (!result.invalid)
    {
        // add instruction occurence of macro name
        const auto& macro_file = macrodef->definition_location.file;
        macro_occurences_[macro_file].emplace_back(macrodef->id, macrodef, result.prototype.macro_name_range);

        auto m_i = std::make_shared<lsp::macro_info>(result.external,
            location(result.prototype.macro_name_range.start, macro_file),
            std::move(macrodef),
            std::move(result.variable_symbols),
            std::move(result.file_scopes),
            std::move(macro_occurences_));

        if (result.external)
            lsp_ctx_.add_macro(std::move(m_i), lsp::text_data_ref_t(file_text_));
        else
            lsp_ctx_.add_macro(std::move(m_i));
    }

    in_macro_ = false;
    macro_occurences_.clear();
}

void lsp_analyzer::copydef_finished(context::copy_member_ptr copydef, copy_processing_result&&)
{
    lsp_ctx_.add_copy(std::move(copydef), lsp::text_data_ref_t(file_text_));
}

void lsp_analyzer::opencode_finished()
{
    lsp_ctx_.add_opencode(
        std::make_unique<lsp::opencode_info>(std::move(opencode_var_defs_), std::move(opencode_occurences_)),
        lsp::text_data_ref_t(file_text_));
}

void lsp_analyzer::assign_statement_occurences()
{
    if (in_macro_)
    {
        auto& file_occs = macro_occurences_[hlasm_ctx_.current_statement_location().file];
        file_occs.insert(
            file_occs.end(), std::move_iterator(stmt_occurences_.begin()), std::move_iterator(stmt_occurences_.end()));
    }
    else
    {
        auto& file_occs = opencode_occurences_[hlasm_ctx_.current_statement_location().file];
        file_occs.insert(
            file_occs.end(), std::move_iterator(stmt_occurences_.begin()), std::move_iterator(stmt_occurences_.end()));
    }
    stmt_occurences_.clear();
}

void lsp_analyzer::collect_occurences(lsp::occurence_kind kind, const context::hlasm_statement& statement)
{
    occurence_collector collector(kind, hlasm_ctx_, stmt_occurences_);

    if (auto def_stmt = statement.access_deferred())
    {
        collect_occurence(def_stmt->label_ref(), collector);
        collect_occurence(def_stmt->instruction_ref(), collector);
        collect_occurence(def_stmt->deferred_ref(), collector);
    }
    else if (auto res_stmt = statement.access_resolved())
    {
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
            collector.get_occurence(std::get<semantics::ord_symbol_string>(label.value).symbol, label.field_range);
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
        auto* macro_def = std::get_if<context::macro_def_ptr>(&opcode.opcode_detail);
        if (opcode.opcode || macro_def)
            collector.occurences.emplace_back(
                opcode.opcode, macro_def ? std::move(*macro_def) : context::macro_def_ptr {}, instruction.field_range);
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



bool lsp_analyzer::is_LCL_GBL(
    const processing::resolved_statement& statement, context::SET_t_enum& set_type, bool& global) const
{
    const auto& code = statement.opcode_ref();

    for (const auto& i : LCL_GBL_instructions_)
    {
        if (code.value == i.name)
        {
            set_type = i.type;
            global = i.global;
            return true;
        }
    }

    return false;
}



bool lsp_analyzer::is_SET(const processing::resolved_statement& statement, context::SET_t_enum& set_type) const
{
    const auto& code = statement.opcode_ref();

    for (const auto& [name, type] : SET_instructions_)
    {
        if (code.value == name)
        {
            set_type = type;
            return true;
        }
    }
    return false;
}

void lsp_analyzer::collect_var_definition(const processing::resolved_statement& statement)
{
    bool global;
    context::SET_t_enum type;
    if (is_SET(statement, type))
        collect_SET_defs(statement, type);
    else if (is_LCL_GBL(statement, type, global))
        collect_LCL_GBL_defs(statement, type, global);
}

void lsp_analyzer::collect_copy_operands(const processing::resolved_statement& statement)
{
    const auto& opcode = statement.opcode_ref().value;
    const auto& operands = statement.operands_ref().value;
    if (opcode == hlasm_ctx_.ids().well_known.COPY && operands.size() == 1 && operands.front()->access_asm())
    {
        auto sym_expr = dynamic_cast<expressions::mach_expr_symbol*>(
            operands.front()->access_asm()->access_expr()->expression.get());

        if (sym_expr)
            add_copy_operand(sym_expr->value, sym_expr->get_range());
    }
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
        if (op->type != semantics::operand_type::CA)
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

void lsp_analyzer::add_copy_operand(context::id_index name, const range& operand_range)
{
    // find ORD occurence of COPY_OP
    lsp::symbol_occurence occ(lsp::occurence_kind::ORD, name, operand_range);
    auto ord_sym = std::find(stmt_occurences_.begin(), stmt_occurences_.end(), occ);

    if (ord_sym != stmt_occurences_.end())
        ord_sym->kind = lsp::occurence_kind::COPY_OP;
    else
        stmt_occurences_.emplace_back(lsp::occurence_kind::COPY_OP, name, operand_range);
}

void lsp_analyzer::update_macro_nest(const processing::resolved_statement& statement)
{
    const auto& opcode = statement.opcode_ref().value;
    if (opcode == hlasm_ctx_.ids().well_known.MACRO)
        macro_nest_++;
    else if (opcode == hlasm_ctx_.ids().well_known.MEND)
        macro_nest_--;
}

} // namespace hlasm_plugin::parser_library::processing
