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
#include "context/instruction.h"
#include "context/ordinary_assembly/ordinary_assembly_dependency_solver.h"
#include "context/ordinary_assembly/postponed_statement.h"
#include "context/ordinary_assembly/symbol_dependency_tables.h"
#include "context/source_context.h"
#include "context/special_instructions.h"
#include "library_info_transitional.h"
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
    using enum lsp::occurrence_kind;
    auto collection_info = get_active_collection(hlasm_ctx_.current_statement_source(), evaluated_model);

    const auto* resolved_stmt = statement.access_resolved();
    switch (proc_kind)
    {
        case processing_kind::ORDINARY: {
            lsp::occurrence_kind kind = ORD;
            if (prov_kind == statement_provider_kind::MACRO)
            {
                if (!resolved_stmt)
                    kind = kind | INSTR;
                else if (const auto& instr = resolved_stmt->instruction_ref();
                         instr.type == semantics::instruction_si_type::ORD
                         && !context::instruction_resolved_during_macro_parsing(resolved_stmt->opcode_ref().value))
                    kind = kind | INSTR;
            }
            else
                kind = kind | INSTR | VAR | SEQ;

            collect_occurrences(kind, statement, collection_info);
            if (prov_kind != statement_provider_kind::MACRO && resolved_stmt)
            { // macros already processed during macro def processing
                collect_var_definition(*resolved_stmt);
                collect_copy_operands(*resolved_stmt, collection_info);
            }

            if (resolved_stmt && resolved_stmt->opcode_ref().value == context::id_index("TITLE"))
                collect_title(*resolved_stmt);
            break;
        }
        case processing_kind::MACRO: {
            if (resolved_stmt)
                update_macro_nest(*resolved_stmt);
            if (macro_nest_ > 1)
                break; // Do not collect occurrences in nested macros to avoid collecting occurrences multiple times
            lsp::occurrence_kind kind {};
            if (statement.kind == context::statement_kind::DEFERRED)
                kind = INSTR_LIKE;
            else if (resolved_stmt)
            {
                if (const auto& instr = resolved_stmt->instruction_ref();
                    instr.type == semantics::instruction_si_type::ORD
                    && context::instruction_resolved_during_macro_parsing(resolved_stmt->opcode_ref().value))
                    kind = INSTR;
            }
            collect_occurrences(kind | VAR | SEQ, statement, collection_info);
            if (resolved_stmt)
                collect_copy_operands(*resolved_stmt, collection_info);
            break;
        }
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
    auto ci = get_active_collection(hlasm_ctx_.opencode_location(), false);

    collect_endline(statement.m_details.stmt_r, ci);

    collect_occurrences(lsp::occurrence_kind::ORD, statement, ci);

    if (const auto& operands = statement.m_details.operands; statement.m_copylike && operands.size() == 1)
        add_copy_operand(hlasm_ctx_.ids().add(operands.front().name), operands.front().r, ci);
}


lsp::line_occurence_details& lsp_analyzer::line_details(const range& r, const collection_info_t& ci)
{
    if (r.start.line >= ci.line_details->size())
        ci.line_details->resize(r.start.line + 1);
    return (*ci.line_details)[r.start.line];
}

void lsp_analyzer::collect_title(const processing::resolved_statement& stmt)
{
    const auto& ops = stmt.operands_ref().value;
    if (ops.size() < 1)
        return;
    const auto* op = ops.front()->access_asm();
    if (!op)
        return;
    const auto* str = op->access_string();
    if (!str)
        return;

    lsp_ctx_.add_title(str->value, hlasm_ctx_.processing_stack());
}

void lsp_analyzer::collect_endline(const range& r, const collection_info_t& ci)
{
    auto& line_detail = line_details(r, ci);
    line_detail.max_endline = std::max(line_detail.max_endline, r.end.line + 1);
}

void lsp_analyzer::collect_usings(const range& r, const collection_info_t& ci)
{
    auto& line_detail = line_details(r, ci);
    if (auto cur = hlasm_ctx_.using_current(); !line_detail.active_using)
        line_detail.active_using = cur;
    else
        line_detail.using_overflow |= line_detail.active_using != cur;
}
void lsp_analyzer::collect_section(const range& r, const collection_info_t& ci)
{
    auto& line_detail = line_details(r, ci);
    if (auto cur = hlasm_ctx_.ord_ctx.current_section(); !line_detail.active_section)
        line_detail.active_section = cur;
    else
        line_detail.section_overflow |= line_detail.active_section != cur;
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
        const auto* md = macrodef.get();
        // add instruction occurrence of macro name
        const auto& macro_file = md->definition_location.resource_loc;

        auto m_i = std::make_shared<lsp::macro_info>(result.external,
            location(result.prototype.macro_name_range.start, macro_file),
            std::move(macrodef),
            std::move(result.variable_symbols),
            std::move(result.file_scopes),
            std::move(macro_occurrences_));
        m_i->file_occurrences_[macro_file].symbols.emplace_back(md->id, md, result.prototype.macro_name_range);

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

void lsp_analyzer::opencode_finished(parse_lib_provider& libs)
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
        if (def_stmt->instruction_ref().type != semantics::instruction_si_type::EMPTY)
        {
            const auto& r = def_stmt->stmt_range_ref();
            collect_endline(r, ci);
            collect_usings(r, ci);
            // no section collection for deferred statements
        }

        collect_occurrence(def_stmt->label_ref(), collector);
        collect_occurrence(def_stmt->instruction_ref(), collector, nullptr);
        collect_occurrence(def_stmt->deferred_ref(), collector);
    }
    else if (auto res_stmt = statement.access_resolved())
    {
        if (res_stmt->instruction_ref().type != semantics::instruction_si_type::EMPTY)
        {
            const auto& r = res_stmt->stmt_range_ref();
            collect_endline(r, ci);
            collect_usings(r, ci);
            collect_section(r, ci);
        }

        collect_occurrence(res_stmt->label_ref(), collector);
        collect_occurrence(res_stmt->instruction_ref(), collector, &res_stmt->opcode_ref());
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

void lsp_analyzer::collect_occurrence(
    const semantics::instruction_si& instruction, occurrence_collector& collector, const op_code* op)
{
    if (instruction.type == semantics::instruction_si_type::CONC)
        collector.get_occurrence(std::get<semantics::concat_chain>(instruction.value));
    else if (instruction.type == semantics::instruction_si_type::ORD
        && any(collector.collector_kind & lsp::occurrence_kind::INSTR))
    {
        if (!op)
        {
            auto opcode = hlasm_ctx_.get_operation_code(std::get<context::id_index>(instruction.value));
            if (!opcode.opcode.empty() || opcode.is_macro())
                collector.occurrences.emplace_back(opcode.opcode, opcode.get_macro_details(), instruction.field_range);
        }
        else if (!op->value.empty() && op->type != context::instruction_type::MAC || op->mac_def)
            collector.occurrences.emplace_back(op->value, op->mac_def, instruction.field_range);
    }
    else if (instruction.type == semantics::instruction_si_type::ORD
        && any(collector.collector_kind & lsp::occurrence_kind::INSTR_LIKE))
    {
        if (const auto& op = std::get<context::id_index>(instruction.value); !op.empty())
            collector.occurrences.emplace_back(lsp::occurrence_kind::INSTR_LIKE, op, instruction.field_range, false);
    }
}

void lsp_analyzer::collect_occurrence(const semantics::operands_si& operands, occurrence_collector& collector)
{
    std::ranges::for_each(operands.value, [&collector](const auto& op) { op->apply(collector); });
}

void lsp_analyzer::collect_occurrence(const semantics::deferred_operands_si& operands, occurrence_collector& collector)
{
    std::ranges::for_each(operands.vars, [&collector](const auto& v) { collector.get_occurrence(*v); });
}

bool lsp_analyzer::is_LCL_GBL(
    const processing::resolved_statement& statement, context::SET_t_enum& set_type, bool& global) const
{
    const auto& code = statement.opcode_ref();

    const auto instr = std::ranges::find(LCL_GBL_instructions_, code.value, &LCL_GBL_instr::name);

    if (instr == LCL_GBL_instructions_.end())
        return false;

    set_type = instr->type;
    global = instr->global;
    return true;
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

    const auto& name = var->access_basic()->name;

    if (std::ranges::find(opencode_var_defs_, name, &lsp::variable_symbol_definition::name) != opencode_var_defs_.end())
        return;

    opencode_var_defs_.emplace_back(name, type, global, hlasm_ctx_.current_statement_source(), var->symbol_range.start);
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

namespace {
std::optional<std::pair<int, int>> get_branch_operand(const context::machine_instruction* m) noexcept
{
    auto ba = m->branch_argument();
    if (!ba.valid())
        return std::nullopt;

    return std::make_pair(ba.unknown_target() ? -1 : ba.target(), ba.branches_if_nonzero() ? ba.nonzero_arg() : -1);
}
std::optional<std::pair<int, int>> get_branch_operand(const context::mnemonic_code* m) noexcept
{
    auto ba = get_branch_operand(m->instruction());
    if (!ba)
        return ba;

    auto& [target, nonzero] = *ba;
    if (nonzero >= 0)
    {
        for (auto pos = nonzero; const auto& t : m->operand_transformations())
        {
            if (pos < t.skip)
                break;
            pos -= t.skip;
            if (pos == 0 && t.insert && t.type == context::mnemonic_transformation_kind::value)
            {
                if (t.value)
                {
                    nonzero = -1;
                    break;
                }
                return std::nullopt; // NOP, NOPR, etc.
            }
            pos -= t.insert;
            nonzero -= t.insert;
        }
    }
    if (target >= 0)
    {
        for (auto pos = target; const auto& t : m->operand_transformations())
        {
            if (pos < t.skip)
                break;
            pos -= t.skip + t.insert;
            target -= t.insert;
        }
    }

    return ba;
}
std::optional<std::pair<int, int>> get_branch_operand(context::id_index id) noexcept
{
    if (const auto [mi, mn] = context::instruction::find_machine_instruction_or_mnemonic(id.to_string_view()); mn)
        return get_branch_operand(mn);
    else
        return get_branch_operand(mi);
}

context::processing_stack_t get_opencode_stackframe(context::processing_stack_t sf)
{
    if (sf.empty())
        return sf;
    while (!sf.parent().empty())
        sf = sf.parent();
    return sf;
}

std::optional<position> extract_symbol_position(semantics::operand& op, context::ordinary_assembly_context& ord_ctx)
{
    if (op.type != semantics::operand_type::MACH)
        return std::nullopt;
    auto* mach = op.access_mach();
    if (!mach)
        return std::nullopt;
    const auto* expr = mach->access_expr();
    if (!expr)
        return std::nullopt;
    const auto* rel_symbol =
        dynamic_cast<const expressions::mach_expr_binary<expressions::rel_addr>*>(expr->expression.get());

    const auto* symbol_name = rel_symbol
        ? dynamic_cast<const expressions::mach_expr_symbol*>(rel_symbol->right_expression())
        : dynamic_cast<const expressions::mach_expr_symbol*>(expr->expression.get());

    if (!symbol_name)
        return std::nullopt;

    const auto* symbol = ord_ctx.get_symbol(symbol_name->value);
    if (!symbol)
        return std::nullopt;

    const auto symbol_oc_loc = get_opencode_stackframe(symbol->proc_stack());

    if (symbol_oc_loc.empty())
        return std::nullopt;

    return symbol_oc_loc.frame().pos;
}

bool symbol_value_zerolike(semantics::operand& op,
    context::ordinary_assembly_context& ord_ctx,
    const context::dependency_evaluation_context& dep_ctx,
    const library_info& li)
{
    if (op.type != semantics::operand_type::MACH)
        return false;
    auto* mach = op.access_mach();
    if (!mach)
        return false;
    const auto* expr = mach->access_expr();
    if (!expr)
        return false;
    context::ordinary_assembly_dependency_solver solver(ord_ctx, dep_ctx, li);
    auto value = expr->expression->evaluate(solver, drop_diagnostic_op);
    if (value.value_kind() != context::symbol_value_kind::ABS)
        return false;
    return value.get_abs() == 0;
}

} // namespace

void lsp_analyzer::collect_branch_info(
    const std::vector<std::pair<std::unique_ptr<context::postponed_statement>, context::dependency_evaluation_context>>&
        stmts,
    const library_info& li)
{
    const auto& opencode_loc = hlasm_ctx_.opencode_location();
    auto& [stmt_occurrences, stmt_ranges] = opencode_occurrences_[opencode_loc];
    const collection_info_t ci { &stmt_occurrences, stmt_occurrences.size(), &stmt_ranges, true };

    for (const auto& [stmt, dep_ctx] : stmts)
    {
        if (!stmt)
            continue;

        const auto* rs = stmt->resolved_stmt();
        const auto& opcode = rs->opcode_ref();
        if (opcode.type != context::instruction_type::MACH)
            continue;

        const auto transfer = get_branch_operand(opcode.value);

        if (!transfer.has_value())
            continue;

        const auto loc = get_opencode_stackframe(stmt->location_stack());
        if (loc.empty() && *loc.frame().resource_loc != opencode_loc)
            continue;

        const auto& [target, condition] = *transfer;
        const auto& ops = rs->operands_ref().value;

        if (condition >= 0 && condition < ops.size()
            && symbol_value_zerolike(*ops[condition], hlasm_ctx_.ord_ctx, dep_ctx, li))
            continue;

        if (!dep_ctx.loctr_address || !dep_ctx.loctr_address->is_simple())
            continue;
        if (dep_ctx.loctr_address->bases().front().first.owner->kind == context::section_kind::DUMMY)
            continue;

        const auto pos = loc.frame().pos;

        auto& ld = line_details(range(pos), ci);
        bool branch_somewhere = true;
        if (target >= 0 && target < ops.size())
        {
            const auto symbol_pos = extract_symbol_position(*ops[target], hlasm_ctx_.ord_ctx);
            if (symbol_pos.has_value())
            {
                branch_somewhere = false;
                if (const auto rel = pos.line <=> symbol_pos->line; rel < 0)
                    ld.branches_down = true;
                else if (rel > 0)
                    ld.branches_up = true;
            }
        }
        if (branch_somewhere)
            ld.branches_somewhere = true;
        ld.offset_to_jump_opcode = (unsigned char)std::min(pos.column, (size_t)80);
    }
}

} // namespace hlasm_plugin::parser_library::processing
