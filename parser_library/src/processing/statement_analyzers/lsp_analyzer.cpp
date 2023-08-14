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
#include <array>
#include <utility>

#include "context/hlasm_context.h"
#include "context/id_storage.h"
#include "context/special_instructions.h"
#include "lsp/lsp_context.h"
#include "lsp/text_data_view.h"
#include "occurrence_collector.h"
#include "processing/statement.h"
#include "processing/statement_analyzers/occurrence_collector.h"
#include "processing/statement_processors/copy_processing_info.h"
#include "processing/statement_processors/macrodef_processing_info.h"
#include "processing/statement_processors/macrodef_processor.h"
#include "range.h"
#include "semantics/operand_impls.h"
#include "semantics/statement.h"
#include "semantics/statement_fields.h"
#include "semantics/variable_symbol.h"
#include "utils/resource_location.h"

namespace hlasm_plugin::parser_library::processing {

namespace {

struct LCL_GBL_instr
{
    context::id_index name;
    context::SET_t_enum type;
    bool global;
};

constexpr std::array<LCL_GBL_instr, 6> LCL_GBL_instructions_ { {
    { context::id_storage::well_known::LCLA, context::SET_t_enum::A_TYPE, false },
    { context::id_storage::well_known::LCLB, context::SET_t_enum::B_TYPE, false },
    { context::id_storage::well_known::LCLC, context::SET_t_enum::C_TYPE, false },
    { context::id_storage::well_known::GBLA, context::SET_t_enum::A_TYPE, true },
    { context::id_storage::well_known::GBLB, context::SET_t_enum::B_TYPE, true },
    { context::id_storage::well_known::GBLC, context::SET_t_enum::C_TYPE, true },
} };

constexpr std::array<std::pair<context::id_index, context::SET_t_enum>, 3> SET_instructions_ { {
    { context::id_storage::well_known::SETA, context::SET_t_enum::A_TYPE },
    { context::id_storage::well_known::SETB, context::SET_t_enum::B_TYPE },
    { context::id_storage::well_known::SETC, context::SET_t_enum::C_TYPE },
} };

} // namespace

lsp_analyzer::lsp_analyzer(context::hlasm_context& hlasm_ctx, lsp::lsp_context& lsp_ctx, std::string_view file_text)
    : hlasm_ctx_(hlasm_ctx)
    , lsp_ctx_(lsp_ctx)
    , file_text_(file_text)
{}

bool lsp_analyzer::analyze(const context::hlasm_statement& statement,
    statement_provider_kind prov_kind,
    processing_kind proc_kind,
    bool evaluated_model)
{
    auto collection_info = get_active_collection(hlasm_ctx_.current_statement_source(), evaluated_model);

    const auto* resolved_stmt = statement.access_resolved();
    switch (proc_kind)
    {
        case processing_kind::ORDINARY:
            collect_occurrences(lsp::occurrence_kind::ORD, statement, collection_info);
            collect_occurrences(lsp::occurrence_kind::INSTR, statement, collection_info);
            if (prov_kind != statement_provider_kind::MACRO) // macros already processed during macro def processing
            {
                collect_occurrences(lsp::occurrence_kind::VAR, statement, collection_info);
                collect_occurrences(lsp::occurrence_kind::SEQ, statement, collection_info);
                if (resolved_stmt)
                {
                    collect_var_definition(*resolved_stmt);
                    collect_copy_operands(*resolved_stmt, collection_info);
                }
            }
            break;
        case processing_kind::MACRO:
            if (resolved_stmt)
                update_macro_nest(*resolved_stmt);
            if (macro_nest_ > 1)
                break; // Do not collect occurrences in nested macros to avoid collecting occurrences multiple times
            if (statement.kind == context::statement_kind::DEFERRED)
                collect_occurrences(lsp::occurrence_kind::INSTR_LIKE, statement, collection_info);
            else if (resolved_stmt)
            {
                if (const auto& instr = statement.access_resolved()->instruction_ref();
                    instr.type == semantics::instruction_si_type::ORD
                    && context::instruction_resolved_during_macro_parsing(std::get<context::id_index>(instr.value)))
                    collect_occurrences(lsp::occurrence_kind::INSTR, statement, collection_info);
            }
            collect_occurrences(lsp::occurrence_kind::VAR, statement, collection_info);
            collect_occurrences(lsp::occurrence_kind::SEQ, statement, collection_info);
            if (resolved_stmt)
                collect_copy_operands(*resolved_stmt, collection_info);
            break;
        default:
            break;
    }

    return false;
}

namespace {
const expressions::mach_expr_symbol* get_single_mach_symbol(const semantics::operand_list& operands)
{
    if (operands.size() != 1)
        return nullptr;
    auto* asm_op = operands.front()->access_asm();
    if (!asm_op)
        return nullptr;
    auto* expr = asm_op->access_expr();
    if (!expr)
        return nullptr;
    return dynamic_cast<const expressions::mach_expr_symbol*>(expr->expression.get());
}
} // namespace

void lsp_analyzer::analyze(const semantics::preprocessor_statement_si& statement)
{
    auto collection_info = get_active_collection(hlasm_ctx_.opencode_location(), false);

    auto& end_line = (*collection_info.stmt_ranges)[statement.m_details.stmt_r.start.line];
    end_line = std::max(end_line, statement.m_details.stmt_r.end.line);

    collect_occurrences(lsp::occurrence_kind::ORD, statement, collection_info);

    if (const auto& operands = statement.m_details.operands; statement.m_copylike && operands.size() == 1)
        add_copy_operand(hlasm_ctx_.ids().add(operands.front().name), operands.front().r, collection_info);
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
        // add instruction occurrence of macro name
        const auto& macro_file = macrodef->definition_location.resource_loc;
        macro_occurrences_[macro_file].first.emplace_back(macrodef->id, macrodef, result.prototype.macro_name_range);

        auto m_i = std::make_shared<lsp::macro_info>(result.external,
            location(result.prototype.macro_name_range.start, macro_file),
            std::move(macrodef),
            std::move(result.variable_symbols),
            std::move(result.file_scopes),
            std::move(macro_occurrences_));

        if (result.external)
            lsp_ctx_.add_macro(std::move(m_i), lsp::text_data_view(file_text_));
        else
            lsp_ctx_.add_macro(std::move(m_i));
    }

    in_macro_ = false;
    macro_occurrences_.clear();
}

void lsp_analyzer::copydef_finished(context::copy_member_ptr copydef, copy_processing_result&&)
{
    lsp_ctx_.add_copy(std::move(copydef), lsp::text_data_view(file_text_));
}

void lsp_analyzer::opencode_finished(workspaces::parse_lib_provider& libs)
{
    lsp_ctx_.add_opencode(
        std::make_unique<lsp::opencode_info>(std::move(opencode_var_defs_), std::move(opencode_occurrences_)),
        lsp::text_data_view(file_text_),
        libs);
}

void lsp_analyzer::collect_occurrences(
    lsp::occurrence_kind kind, const context::hlasm_statement& statement, const collection_info_t& ci)
{
    occurrence_collector collector(kind, hlasm_ctx_, *ci.stmt_occurrences, ci.evaluated_model);

    if (auto def_stmt = statement.access_deferred())
    {
        const auto& stmt_range = def_stmt->stmt_range_ref();
        auto& end_line = (*ci.stmt_ranges)[stmt_range.start.line];
        end_line = std::max(end_line, stmt_range.end.line);
        collect_occurrence(def_stmt->label_ref(), collector);
        collect_occurrence(def_stmt->instruction_ref(), collector);
        collect_occurrence(def_stmt->deferred_ref(), collector);
    }
    else if (auto res_stmt = statement.access_resolved())
    {
        const auto& stmt_range = res_stmt->stmt_range_ref();
        auto& end_line = (*ci.stmt_ranges)[stmt_range.start.line];
        end_line = std::max(end_line, stmt_range.end.line);
        collect_occurrence(res_stmt->label_ref(), collector);
        collect_occurrence(res_stmt->instruction_ref(), collector);
        collect_occurrence(res_stmt->operands_ref(), collector);
    }
}

void lsp_analyzer::collect_occurrences(
    lsp::occurrence_kind kind, const semantics::preprocessor_statement_si& statement, const collection_info_t& ci)
{
    occurrence_collector collector(kind, hlasm_ctx_, *ci.stmt_occurrences, ci.evaluated_model);
    const auto& details = statement.m_details;

    collector.occurrences.emplace_back(
        lsp::occurrence_kind::ORD, hlasm_ctx_.ids().add(details.label.name), details.label.r, ci.evaluated_model);

    collector.occurrences.emplace_back(lsp::occurrence_kind::INSTR,
        hlasm_ctx_.ids().add(details.instruction.nr.name),
        details.instruction.nr.r,
        ci.evaluated_model);

    for (const auto& ops : details.operands)
        collector.occurrences.emplace_back(
            lsp::occurrence_kind::ORD, hlasm_ctx_.ids().add(ops.name), ops.r, ci.evaluated_model);
}

void lsp_analyzer::collect_occurrence(const semantics::label_si& label, occurrence_collector& collector)
{
    switch (label.type)
    {
        case semantics::label_si_type::CONC:
            collector.get_occurrence(std::get<semantics::concat_chain>(label.value));
            break;
        case semantics::label_si_type::ORD:
            collector.get_occurrence(std::get<semantics::ord_symbol_string>(label.value).symbol, label.field_range);
            break;
        case semantics::label_si_type::SEQ:
            collector.get_occurrence(std::get<semantics::seq_sym>(label.value));
            break;
        case semantics::label_si_type::VAR:
            collector.get_occurrence(*std::get<semantics::vs_ptr>(label.value));
            break;
        default:
            break;
    }
}

void lsp_analyzer::collect_occurrence(const semantics::instruction_si& instruction, occurrence_collector& collector)
{
    if (instruction.type == semantics::instruction_si_type::CONC)
        collector.get_occurrence(std::get<semantics::concat_chain>(instruction.value));
    else if (instruction.type == semantics::instruction_si_type::ORD
        && collector.collector_kind == lsp::occurrence_kind::INSTR)
    {
        auto opcode = hlasm_ctx_.get_operation_code(
            std::get<context::id_index>(instruction.value)); // TODO: collect the instruction at the right time
        auto* macro_def = std::get_if<context::macro_def_ptr>(&opcode.opcode_detail);
        if (!opcode.opcode.empty() || macro_def)
            collector.occurrences.emplace_back(
                opcode.opcode, macro_def ? std::move(*macro_def) : context::macro_def_ptr {}, instruction.field_range);
    }
    else if (instruction.type == semantics::instruction_si_type::ORD
        && collector.collector_kind == lsp::occurrence_kind::INSTR_LIKE)
    {
        if (const auto& op = std::get<context::id_index>(instruction.value); !op.empty())
            collector.occurrences.emplace_back(lsp::occurrence_kind::INSTR_LIKE, op, instruction.field_range, false);
    }
}

void lsp_analyzer::collect_occurrence(const semantics::operands_si& operands, occurrence_collector& collector)
{
    std::for_each(operands.value.begin(), operands.value.end(), [&](const auto& op) { op->apply(collector); });
}

void lsp_analyzer::collect_occurrence(const semantics::deferred_operands_si& operands, occurrence_collector& collector)
{
    std::for_each(operands.vars.begin(), operands.vars.end(), [&](const auto& v) { collector.get_occurrence(*v); });
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

void lsp_analyzer::collect_copy_operands(
    const processing::resolved_statement& statement, const collection_info_t& collection_info)
{
    if (statement.opcode_ref().value != context::id_storage::well_known::COPY)
        return;
    if (auto sym_expr = get_single_mach_symbol(statement.operands_ref().value))
        add_copy_operand(sym_expr->value, sym_expr->get_range(), collection_info);
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
        var->access_basic()->name, type, global, hlasm_ctx_.current_statement_source(), var->symbol_range.start);
}

void lsp_analyzer::add_copy_operand(context::id_index name, const range& operand_range, const collection_info_t& ci)
{
    // find ORD occurrence of COPY_OP
    lsp::symbol_occurrence occ(lsp::occurrence_kind::ORD, name, operand_range, ci.evaluated_model);
    auto ord_sym = std::find(ci.stmt_occurrences->begin() + ci.stmt_occurrences_last, ci.stmt_occurrences->end(), occ);

    if (ord_sym != ci.stmt_occurrences->end())
        ord_sym->kind = lsp::occurrence_kind::COPY_OP;
    else
        ci.stmt_occurrences->emplace_back(lsp::occurrence_kind::COPY_OP, name, operand_range, ci.evaluated_model);
}

void lsp_analyzer::update_macro_nest(const processing::resolved_statement& statement)
{
    const auto& opcode = statement.opcode_ref().value;
    if (opcode == context::id_storage::well_known::MACRO)
        macro_nest_++;
    else if (opcode == context::id_storage::well_known::MEND)
        macro_nest_--;
}

lsp_analyzer::collection_info_t lsp_analyzer::get_active_collection(
    const utils::resource::resource_location& loc, bool evaluated_model)
{
    auto& [stmt_occurrences, stmt_ranges] = in_macro_ ? macro_occurrences_[loc] : opencode_occurrences_[loc];
    return { &stmt_occurrences, stmt_occurrences.size(), &stmt_ranges, evaluated_model };
}

} // namespace hlasm_plugin::parser_library::processing
