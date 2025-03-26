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

#include "macrodef_processor.h"

#include <functional>
#include <iterator>
#include <ranges>

#include "context/instruction.h"
#include "processing/branching_provider.h"
#include "processing/instruction_sets/asm_processor.h"
#include "processing/instruction_sets/macro_processor.h"
#include "processing/processing_state_listener.h"
#include "semantics/operand_impls.h"

namespace hlasm_plugin::parser_library::processing {

macrodef_processor::macrodef_processor(const analyzing_context& ctx,
    processing_state_listener& listener,
    branching_provider& branching_provider_,
    macrodef_start_data start,
    diagnosable_ctx& diag_ctx)
    : statement_processor(processing_kind::MACRO, ctx, diag_ctx)
    , listener_(listener)
    , branching_provider_(branching_provider_)
    , start_(std::move(start))
    , initial_copy_nest_(hlasm_ctx.current_copy_stack().size())
    , macro_nest_(1)
    , expecting_prototype_(true)
    , expecting_MACRO_(start_.is_external)
    , finished_flag_(false)
    , table_(create_table())
{
    result_.definition_location = hlasm_ctx.current_statement_location();
    result_.external = start_.is_external;
    if (start_.is_external)
        result_.prototype.macro_name = start_.external_name;

    result_.invalid = true; // result starts invalid until mandatory statements are encountered
}


std::optional<context::id_index> macrodef_processor::resolve_concatenation(
    const semantics::concat_chain& concat, const range&) const
{
    // TODO: should the identifier limit be enforced here?
    return hlasm_ctx.ids().add(semantics::concatenation_point::to_string(concat));
}

std::optional<processing_status> macrodef_processor::get_processing_status(
    const std::optional<context::id_index>& instruction, const range& r) const
{
    assert(instruction.has_value());
    if (expecting_prototype_ && !expecting_MACRO_)
    {
        processing_format format(processing_kind::MACRO, processing_form::MAC);
        context::id_index id;
        if (instruction->empty())
        {
            add_diagnostic(diagnostic_op::error_E042(r));

            id = context::id_storage::well_known::ASPACE;
        }
        else
        {
            id = *instruction;
        }
        return std::make_pair(format, op_code(id, context::instruction_type::MAC, nullptr));
    }
    else
    {
        return get_macro_processing_status(instruction, hlasm_ctx);
    }
}

void macrodef_processor::process_statement(context::shared_stmt_ptr statement)
{
    bool expecting_tmp = expecting_prototype_ || expecting_MACRO_;

    bumped_macro_nest = false;
    bool handled = process_statement(*statement);

    if (!expecting_tmp && !handled)
    {
        result_.definition.push_back(statement);
        add_correct_copy_nest();
    }
}

void macrodef_processor::end_processing()
{
    if (!finished_flag_)
        add_diagnostic(diagnostic_op::error_E046(
            result_.prototype.macro_name.to_string_view(), range(hlasm_ctx.current_statement_position())));

    hlasm_ctx.pop_statement_processing();

    listener_.finish_macro_definition(std::move(result_));

    finished_flag_ = true;
}

bool macrodef_processor::terminal_condition(const statement_provider_kind prov_kind) const
{
    return prov_kind == statement_provider_kind::MACRO || prov_kind == statement_provider_kind::OPEN;
}

bool macrodef_processor::finished() { return finished_flag_; }

processing_status macrodef_processor::get_macro_processing_status(
    const std::optional<context::id_index>& instruction, context::hlasm_context& hlasm_ctx)
{
    if (instruction && !instruction->empty())
    {
        auto code = hlasm_ctx.get_operation_code(*instruction);
        if (auto** ca_instr = std::get_if<const context::ca_instruction*>(&code.opcode_detail); ca_instr)
        {
            processing_format format(processing_kind::MACRO,
                processing_form::CA,
                (*ca_instr)->operandless() ? operand_occurrence::ABSENT : operand_occurrence::PRESENT);

            return std::make_pair(format, op_code(code.opcode, context::instruction_type::CA, nullptr));
        }
        else if (code.opcode == context::id_storage::well_known::COPY)
        {
            processing_format format(processing_kind::MACRO, processing_form::ASM, operand_occurrence::PRESENT);

            return std::make_pair(format, op_code(code.opcode, context::instruction_type::ASM, nullptr));
        }
    }

    if (instruction && instruction->empty())
    {
        processing_format format(processing_kind::MACRO, processing_form::CA, operand_occurrence::ABSENT);

        return std::make_pair(format, op_code(context::id_index(), context::instruction_type::CA, nullptr));
    }

    processing_format format(processing_kind::MACRO, processing_form::DEFERRED);
    return std::make_pair(format, op_code());
}

void macrodef_processor::update_outer_position(const context::hlasm_statement& stmt)
{
    if (hlasm_ctx.current_copy_stack().size() != initial_copy_nest_)
        return;
    if (const auto& scope = hlasm_ctx.current_scope();
        scope.is_in_macro() && scope.this_macro->get_current_copy_nest().size() >= 2)
        return;

    curr_outer_position_ = stmt.statement_position();
}

bool macrodef_processor::process_statement(const context::hlasm_statement& statement)
{
    assert(!finished_flag_);

    auto res_stmt = statement.access_resolved();

    if (expecting_MACRO_)
    {
        result_.definition_location = hlasm_ctx.current_statement_location();

        if (!res_stmt || res_stmt->opcode_ref().value != context::id_storage::well_known::MACRO)
        {
            add_diagnostic(
                diagnostic_op::error_E059(start_.external_name.to_string_view(), statement.stmt_range_ref()));
            result_.invalid = true;
            finished_flag_ = true;
            return false;
        }
        else
            expecting_MACRO_ = false;
    }
    else if (expecting_prototype_)
    {
        if (!res_stmt)
        {
            add_diagnostic(diagnostic_op::error_E071(statement.stmt_range_ref()));
            result_.invalid = true;
            return false;
        }
        result_.invalid = false;
        process_prototype(*res_stmt);
        expecting_prototype_ = false;
    }
    else
    {
        update_outer_position(statement);

        if (res_stmt)
        {
            process_sequence_symbol(res_stmt->label_ref());

            if (auto found = table_.find(res_stmt->opcode_ref().value); found != table_.end())
            {
                auto& [key, func] = *found;
                return func(*res_stmt);
            }
        }
        else if (auto def_stmt = statement.access_deferred())
        {
            process_sequence_symbol(def_stmt->label);
        }
    }
    return false;
}

void macrodef_processor::process_prototype(const resolved_statement& statement)
{
    std::vector<context::id_index> param_names;

    process_prototype_label(statement, param_names);

    process_prototype_instruction(statement);

    process_prototype_operand(statement, param_names);
}

void macrodef_processor::process_prototype_label(
    const resolved_statement& statement, std::vector<context::id_index>& param_names)
{
    if (statement.label_ref().type == semantics::label_si_type::VAR)
    {
        auto var = std::get<semantics::vs_ptr>(statement.label_ref().value).get();
        if (var->created || var->subscript.size())
            add_diagnostic(diagnostic_op::error_E043(var->symbol_range));
        else
        {
            result_.prototype.name_param = var->access_basic()->name;
            result_.variable_symbols.emplace_back(var->access_basic()->name,
                context::statement_id { result_.definition.size() },
                var->symbol_range.start);
            param_names.push_back(result_.prototype.name_param);
        }
    }
    else if (statement.label_ref().type != semantics::label_si_type::EMPTY)
        add_diagnostic(diagnostic_op::error_E044(statement.label_ref().field_range));
}

void macrodef_processor::process_prototype_instruction(const resolved_statement& statement)
{
    auto macro_name = statement.opcode_ref().value;
    if (start_.is_external && macro_name != start_.external_name)
    {
        add_diagnostic(
            diagnostic_op::error_E060(start_.external_name.to_string_view(), statement.instruction_ref().field_range));
        result_.invalid = true;
        finished_flag_ = true;
        return;
    }
    result_.prototype.macro_name = statement.opcode_ref().value;
    result_.prototype.macro_name_range = statement.instruction_ref().field_range;
}

void macrodef_processor::process_prototype_operand(
    const resolved_statement& statement, std::vector<context::id_index>& param_names)
{
    using namespace semantics;
    for (auto& op : statement.operands_ref().value)
    {
        if (op->type == operand_type::EMPTY)
        {
            result_.prototype.symbolic_params.emplace_back(nullptr);
            continue;
        }

        if (op->type != operand_type::MAC)
        {
            result_.prototype.symbolic_params.emplace_back(nullptr);
            add_diagnostic(diagnostic_op::error_E043(op->operand_range));
            continue;
        }

        auto tmp = op->access_mac();
        assert(tmp);

        auto& tmp_chain = tmp->chain;

        if (concat_chain_matches<var_sym_conc>(tmp_chain)) // if operand is varsym
        {
            auto var = std::get<var_sym_conc>(tmp_chain[0].value).symbol.get();

            if (test_varsym_validity(var, param_names, tmp->operand_range, true))
            {
                auto var_id = var->access_basic()->name;
                param_names.push_back(var_id);
                result_.prototype.symbolic_params.emplace_back(nullptr, var_id);
                result_.variable_symbols.emplace_back(var->access_basic()->name,
                    context::statement_id { result_.definition.size() },
                    var->symbol_range.start);
            }
        }
        else if (tmp_chain.size() > 1)
        {
            if (concat_chain_starts_with<var_sym_conc, equals_conc>(tmp_chain)) // if operand is in form of key param
            {
                auto var = std::get<var_sym_conc>(tmp_chain[0].value).symbol.get();

                if (test_varsym_validity(var, param_names, tmp->operand_range, false))
                {
                    auto var_id = var->access_basic()->name;

                    param_names.push_back(var_id);

                    diagnostic_adder add_diags(diag_ctx, op->operand_range);

                    result_.prototype.symbolic_params.emplace_back(
                        macro_processor::create_macro_data(tmp_chain.begin() + 2, tmp_chain.end(), add_diags), var_id);
                    result_.variable_symbols.emplace_back(var->access_basic()->name,
                        context::statement_id { result_.definition.size() },
                        var->symbol_range.start);
                }
            }
            else
                add_diagnostic(diagnostic_op::error_E043(op->operand_range));
        }
        else if (tmp_chain.empty()) // if operand is empty
            result_.prototype.symbolic_params.emplace_back(nullptr);
    }
}

bool macrodef_processor::test_varsym_validity(const semantics::variable_symbol* var,
    const std::vector<context::id_index>& param_names,
    range op_range,
    bool add_empty)
{
    if (var->created || !var->subscript.empty())
    {
        add_diagnostic(diagnostic_op::error_E043(var->symbol_range));
        result_.prototype.symbolic_params.emplace_back(nullptr);
        return false;
    }

    if (std::ranges::find(param_names, var->access_basic()->name) != param_names.end())
    {
        add_diagnostic(diagnostic_op::error_E011("Symbolic parameter", op_range));
        if (add_empty)
            result_.prototype.symbolic_params.emplace_back(nullptr);
        return false;
    }
    return true;
}

macrodef_processor::process_table_t macrodef_processor::create_table()
{
    process_table_t table;
    table.emplace(context::id_storage::well_known::SETA,
        [this](const resolved_statement& stmt) { return process_SET(stmt, context::SET_t_enum::A_TYPE); });
    table.emplace(context::id_storage::well_known::SETB,
        [this](const resolved_statement& stmt) { return process_SET(stmt, context::SET_t_enum::B_TYPE); });
    table.emplace(context::id_storage::well_known::SETC,
        [this](const resolved_statement& stmt) { return process_SET(stmt, context::SET_t_enum::C_TYPE); });
    table.emplace(context::id_storage::well_known::LCLA,
        [this](const resolved_statement& stmt) { return process_LCL_GBL(stmt, context::SET_t_enum::A_TYPE, false); });
    table.emplace(context::id_storage::well_known::LCLB,
        [this](const resolved_statement& stmt) { return process_LCL_GBL(stmt, context::SET_t_enum::B_TYPE, false); });
    table.emplace(context::id_storage::well_known::LCLC,
        [this](const resolved_statement& stmt) { return process_LCL_GBL(stmt, context::SET_t_enum::C_TYPE, false); });
    table.emplace(context::id_storage::well_known::GBLA,
        [this](const resolved_statement& stmt) { return process_LCL_GBL(stmt, context::SET_t_enum::A_TYPE, true); });
    table.emplace(context::id_storage::well_known::GBLB,
        [this](const resolved_statement& stmt) { return process_LCL_GBL(stmt, context::SET_t_enum::B_TYPE, true); });
    table.emplace(context::id_storage::well_known::GBLC,
        [this](const resolved_statement& stmt) { return process_LCL_GBL(stmt, context::SET_t_enum::C_TYPE, true); });
    table.emplace(
        context::id_storage::well_known::MACRO, [this](const resolved_statement&) { return process_MACRO(); });
    table.emplace(context::id_storage::well_known::MEND, [this](const resolved_statement&) { return process_MEND(); });
    table.emplace(
        context::id_storage::well_known::COPY, [this](const resolved_statement& stmt) { return process_COPY(stmt); });
    return table;
}

bool macrodef_processor::process_MACRO()
{
    ++macro_nest_;
    bumped_macro_nest = true;
    return false;
}

bool macrodef_processor::process_MEND()
{
    assert(macro_nest_ != 0);
    --macro_nest_;

    if (macro_nest_ == 0)
        finished_flag_ = true;

    return false;
}

struct empty_statement_t final : public resolved_statement
{
    explicit empty_statement_t(range r)
        : label(r)
        , instruction(r)
        , operands(r, {})
        , remarks({})
    {}

    static const processing_status status;

    semantics::label_si label;
    semantics::instruction_si instruction;
    semantics::operands_si operands;
    semantics::remarks_si remarks;

    const range& stmt_range_ref() const override { return instruction.field_range; }
    const semantics::label_si& label_ref() const override { return label; }
    const semantics::instruction_si& instruction_ref() const override { return instruction; }
    const semantics::operands_si& operands_ref() const override { return operands; }
    const semantics::remarks_si& remarks_ref() const override { return remarks; }
    std::span<const semantics::literal_si> literals() const override { return {}; }
    const op_code& opcode_ref() const override { return status.second; }
    processing_format format_ref() const override { return status.first; }
    std::span<const diagnostic_op> diagnostics() const override { return {}; }
};

const processing_status empty_statement_t::status(
    processing_format(processing_kind::ORDINARY, processing_form::CA, operand_occurrence::ABSENT),
    op_code(context::id_storage::well_known::ANOP, context::instruction_type::CA, nullptr));

bool macrodef_processor::process_COPY(const resolved_statement& statement)
{
    // substitute copy for anop to not be processed again
    result_.definition.push_back(std::make_shared<empty_statement_t>(statement.stmt_range_ref()));
    add_correct_copy_nest();

    if (auto extract = asm_processor::extract_copy_id(statement, nullptr); extract.has_value())
    {
        if (ctx.hlasm_ctx->copy_members().contains(extract->name))
        {
            if (asm_processor::common_copy_postprocess(true, *extract, *ctx.hlasm_ctx, &diag_ctx))
            {
                result_.used_copy_members.insert(ctx.hlasm_ctx->current_copy_stack().back().copy_member_definition);
            }
        }
        else
        {
            branching_provider_.request_external_processing(
                extract->name, processing_kind::COPY, [extract, this](bool result) {
                    asm_processor::common_copy_postprocess(result, *extract, *ctx.hlasm_ctx, &diag_ctx);
                });
        }
    }

    return true;
}

bool macrodef_processor::process_LCL_GBL(const resolved_statement& statement, context::SET_t_enum set_type, bool global)
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

            add_SET_sym_to_res(var, set_type, global);
        }
    }
    return false;
}

bool macrodef_processor::process_SET(const resolved_statement& statement, context::SET_t_enum set_type)
{
    if (statement.label_ref().type != semantics::label_si_type::VAR)
        return false;

    auto var = std::get<semantics::vs_ptr>(statement.label_ref().value).get();

    add_SET_sym_to_res(var, set_type, false);

    return false;
}

void macrodef_processor::add_SET_sym_to_res(
    const semantics::variable_symbol* var, context::SET_t_enum set_type, bool global)
{
    if (macro_nest_ > 1)
        return;
    if (var->created)
        return;

    const auto& name = var->access_basic()->name;

    if (std::ranges::find(result_.variable_symbols, name, &lsp::variable_symbol_definition::name)
        != result_.variable_symbols.end())
        return;

    result_.variable_symbols.emplace_back(
        name, set_type, global, context::statement_id { result_.definition.size() }, var->symbol_range.start);
}

void macrodef_processor::process_sequence_symbol(const semantics::label_si& label)
{
    if (macro_nest_ == 1 && label.type == semantics::label_si_type::SEQ)
    {
        auto& seq = std::get<semantics::seq_sym>(label.value);

        if (result_.sequence_symbols.find(seq.name) != result_.sequence_symbols.end())
        {
            add_diagnostic(diagnostic_op::error_E044(seq.symbol_range));
        }
        else
        {
            auto& sym = result_.sequence_symbols[seq.name];
            if (!sym)
                sym = std::make_unique<context::macro_sequence_symbol>(seq.name,
                    location(label.field_range.start, hlasm_ctx.current_statement_source()),
                    context::statement_id { result_.definition.size() });
        }
    }
}

void macrodef_processor::add_correct_copy_nest()
{
    result_.nests.push_back({ context::copy_nest_item {
        { curr_outer_position_, result_.definition_location.resource_loc }, result_.prototype.macro_name } });

    for (size_t i = initial_copy_nest_; i < hlasm_ctx.current_copy_stack().size(); ++i)
    {
        const auto& nest = hlasm_ctx.current_copy_stack()[i];
        result_.nests.back().emplace_back(context::copy_nest_item {
            location { nest.current_statement_position(), nest.definition_location()->resource_loc }, nest.name() });
    }

    if (initial_copy_nest_ < hlasm_ctx.current_copy_stack().size())
        result_.used_copy_members.insert(hlasm_ctx.current_copy_stack().back().copy_member_definition);

    const bool in_inner_macro = macro_nest_ > 1 + bumped_macro_nest;
    auto& [scope, start_new_slice] = result_.file_scopes[result_.nests.back().back().loc.resource_loc];

    const context::statement_id current_statement_id = { result_.definition.size() - 1 };
    if (scope.empty() || start_new_slice)
    {
        scope.emplace_back(current_statement_id, in_inner_macro);
        start_new_slice = false;
    }
    else if (!last_in_inner_macro_ && in_inner_macro) // add new scope when inner macro started
    {
        scope.emplace_back(current_statement_id, in_inner_macro);
    }
    else
    {
        scope.back().end_statement = current_statement_id;
        // remmember to start a new scope when inner macro ended
        start_new_slice = last_in_inner_macro_ && !in_inner_macro;
    }

    last_in_inner_macro_ = in_inner_macro;
}

} // namespace hlasm_plugin::parser_library::processing
