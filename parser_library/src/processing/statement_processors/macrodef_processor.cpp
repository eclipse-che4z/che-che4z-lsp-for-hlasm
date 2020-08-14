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

#include "processing/context_manager.h"
#include "processing/instruction_sets/asm_processor.h"
#include "processing/instruction_sets/macro_processor.h"
#include "processing/statement.h"
#include "semantics/concatenation.h"
#include "semantics/statement.h"

namespace hlasm_plugin::parser_library::processing {

macrodef_processor::macrodef_processor(context::hlasm_context& hlasm_context,
    processing_state_listener& listener,
    workspaces::parse_lib_provider& provider,
    macrodef_start_data start)
    : statement_processor(processing_kind::MACRO, hlasm_context)
    , listener_(listener)
    , provider_(provider)
    , start_(std::move(start))
    , initial_copy_nest_(hlasm_ctx.current_copy_stack().size())
    , macro_nest_(1)
    , curr_line_(0)
    , expecting_prototype_(true)
    , expecting_MACRO_(start_.is_external)
    , omit_next_(false)
    , finished_flag_(false)
{
    result_.definition_location = hlasm_ctx.processing_stack().back().proc_location;
    if (start_.is_external)
        result_.prototype.macro_name = start_.external_name;
}

processing_status macrodef_processor::get_processing_status(const semantics::instruction_si& instruction) const
{
    if (expecting_prototype_ && !expecting_MACRO_)
    {
        processing_format format(processing_kind::MACRO, processing_form::MAC);
        context::id_index id;
        if (instruction.type == semantics::instruction_si_type::EMPTY)
        {
            add_diagnostic(diagnostic_op::error_E042(instruction.field_range));

            id = hlasm_ctx.ids().add("ASPACE");
        }
        else
        {
            id = instruction.type == semantics::instruction_si_type::ORD
                ? std::get<context::id_index>(instruction.value)
                : hlasm_ctx.ids().add(
                    semantics::concatenation_point::to_string(std::get<semantics::concat_chain>(instruction.value)));
        }
        return std::make_pair(format, op_code(id, context::instruction_type::MAC));
    }
    else
    {
        return get_macro_processing_status(instruction, hlasm_ctx);
    }
}

void macrodef_processor::process_statement(context::shared_stmt_ptr statement)
{
    bool expecting_tmp = expecting_prototype_ || expecting_MACRO_;

    process_statement(*statement);

    if (!expecting_tmp && !omit_next_)
    {
        result_.definition.push_back(statement);
        add_correct_copy_nest();
    }
}

void macrodef_processor::process_statement(context::unique_stmt_ptr statement)
{
    bool expecting_tmp = expecting_prototype_ || expecting_MACRO_;

    process_statement(*statement);

    if (!expecting_tmp && !omit_next_)
    {
        result_.definition.push_back(std::move(statement));
        add_correct_copy_nest();
    }
}

void macrodef_processor::end_processing()
{
    if (!finished_flag_)
        add_diagnostic(diagnostic_op::error_E046(*result_.prototype.macro_name,
            range(hlasm_ctx.processing_stack().back().proc_location.pos,
                hlasm_ctx.processing_stack().back().proc_location.pos)));

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
    const semantics::instruction_si& instruction, context::hlasm_context& hlasm_ctx)
{
    if (instruction.type == semantics::instruction_si_type::ORD)
    {
        auto id = std::get<context::id_index>(instruction.value);
        auto code = hlasm_ctx.get_operation_code(id);
        if (code.machine_opcode && code.machine_source == context::instruction::instruction_array::CA)
        {
            id = code.machine_opcode;
            auto operandless = std::find_if(context::instruction::ca_instructions.begin(),
                context::instruction::ca_instructions.end(),
                [&](auto& instr) {
                    return instr.name == *id;
                })->operandless;

            processing_format format(processing_kind::MACRO,
                processing_form::CA,
                operandless ? operand_occurence::ABSENT : operand_occurence::PRESENT);

            return std::make_pair(format, op_code(id, context::instruction_type::CA));
        }
        else if (code.machine_opcode == hlasm_ctx.ids().add("COPY"))
        {
            processing_format format(processing_kind::MACRO, processing_form::ASM, operand_occurence::PRESENT);

            return std::make_pair(format, op_code(code.machine_opcode, context::instruction_type::ASM));
        }
    }

    if (instruction.type == semantics::instruction_si_type::EMPTY)
    {
        processing_format format(processing_kind::MACRO, processing_form::CA, operand_occurence::ABSENT);

        return std::make_pair(format, op_code(context::id_storage::empty_id, context::instruction_type::CA));
    }

    processing_format format(processing_kind::MACRO, processing_form::DEFERRED);
    return std::make_pair(format, op_code());
}

void macrodef_processor::collect_diags() const {}

void macrodef_processor::process_statement(const context::hlasm_statement& statement)
{
    if (finished_flag_)
        throw std::runtime_error("bad operation");

    omit_next_ = false;

    if (expecting_MACRO_)
    {
        result_.definition_location = hlasm_ctx.processing_stack().back().proc_location;

        auto res_stmt = statement.access_resolved();

        if (!res_stmt || res_stmt->opcode_ref().value != macro_id)
        {
            range r = res_stmt ? res_stmt->stmt_range_ref() : range(statement.statement_position());
            add_diagnostic(diagnostic_op::error_E059(*start_.external_name, r));
            result_.invalid = true;
            finished_flag_ = true;
            return;
        }
        else
            expecting_MACRO_ = false;
    }
    else if (expecting_prototype_)
    {
        assert(statement.access_resolved());
        process_prototype(*statement.access_resolved());
        expecting_prototype_ = false;
    }
    else
    {
        if (hlasm_ctx.current_copy_stack().size() - initial_copy_nest_ == 0)
            curr_outer_position_ = statement.statement_position();

        if (auto res_stmt = statement.access_resolved())
        {
            process_sequence_symbol(res_stmt->label_ref());

            if (res_stmt->opcode_ref().value == macro_id)
                process_MACRO();
            else if (res_stmt->opcode_ref().value == mend_id)
                process_MEND();
            else if (res_stmt->opcode_ref().value == copy_id)
                process_COPY(*res_stmt);
        }
        else if (auto def_stmt = statement.access_deferred())
        {
            process_sequence_symbol(def_stmt->label_ref());
        }
        else
            assert(false);

        ++curr_line_;
    }
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
        if (var->created || var->subscript.size() != 0)
            add_diagnostic(diagnostic_op::error_E043(var->symbol_range));
        else
        {
            result_.prototype.name_param = var->access_basic()->name;
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
        add_diagnostic(diagnostic_op::error_E060(*start_.external_name, statement.instruction_ref().field_range));
        result_.invalid = true;
        finished_flag_ = true;
        return;
    }
    result_.prototype.macro_name = statement.opcode_ref().value;
}

void macrodef_processor::process_prototype_operand(
    const resolved_statement& statement, std::vector<context::id_index>& param_names)
{
    processing::context_manager mngr(hlasm_ctx);

    for (auto& op : statement.operands_ref().value)
    {
        if (op->type == semantics::operand_type::EMPTY)
        {
            result_.prototype.symbolic_params.emplace_back(nullptr, nullptr);
            continue;
        }

        auto tmp = op->access_mac();
        assert(tmp);

        auto& tmp_chain = tmp->chain;

        semantics::concatenation_point::clear_concat_chain(tmp_chain);

        if (tmp_chain.size() == 1 && tmp_chain[0]->type == semantics::concat_type::VAR) // if operand is varsym
        {
            auto var = tmp_chain[0]->access_var()->symbol.get();

            if (test_varsym_validity(var, param_names, tmp->operand_range, true))
            {
                auto var_id = var->access_basic()->name;
                param_names.push_back(var_id);
                result_.prototype.symbolic_params.emplace_back(nullptr, var_id);
            }
        }
        else if (tmp_chain.size() > 1)
        {
            if (tmp_chain[0]->type == semantics::concat_type::VAR
                && tmp_chain[1]->type == semantics::concat_type::EQU) // if operand is in form of key param
            {
                auto var = tmp_chain[0]->access_var()->symbol.get();

                if (test_varsym_validity(var, param_names, tmp->operand_range, false))
                {
                    auto var_id = var->access_basic()->name;

                    param_names.push_back(var_id);

                    diagnostic_adder add_diags(this, op->operand_range);

                    result_.prototype.symbolic_params.emplace_back(
                        macro_processor::create_macro_data(tmp_chain.begin() + 2, tmp_chain.end(), add_diags), var_id);
                }
            }
            else
                add_diagnostic(diagnostic_op::error_E043(op->operand_range));
        }
        else if (tmp_chain.size() == 0) // if operand is empty
            result_.prototype.symbolic_params.emplace_back(nullptr, nullptr);
    }

    collect_diags_from_child(mngr);
}

bool macrodef_processor::test_varsym_validity(const semantics::variable_symbol* var,
    const std::vector<context::id_index>& param_names,
    range op_range,
    bool add_empty)
{
    if (var->created || !var->subscript.empty())
    {
        add_diagnostic(diagnostic_op::error_E043(var->symbol_range));
        result_.prototype.symbolic_params.emplace_back(nullptr, nullptr);
        return false;
    }

    auto var_id = var->access_basic()->name;

    if (std::find(param_names.begin(), param_names.end(), var_id) != param_names.end())
    {
        add_diagnostic(diagnostic_op::error_E011("Symbolic parameter", op_range));
        if (add_empty)
            result_.prototype.symbolic_params.emplace_back(nullptr, nullptr);
        return false;
    }
    return true;
}

void macrodef_processor::process_MACRO() { ++macro_nest_; }

void macrodef_processor::process_MEND()
{
    assert(macro_nest_ != 0);
    --macro_nest_;

    if (macro_nest_ == 0)
        finished_flag_ = true;
}

void macrodef_processor::process_COPY(const resolved_statement& statement)
{
    // substitute copy for anop to not be processed again
    result_.definition.push_back(
        std::make_unique<resolved_statement_impl>(semantics::statement_si(statement.stmt_range_ref(),
                                                      semantics::label_si(statement.stmt_range_ref()),
                                                      semantics::instruction_si(statement.stmt_range_ref()),
                                                      semantics::operands_si(statement.stmt_range_ref(), {}),
                                                      semantics::remarks_si(statement.stmt_range_ref(), {})),
            op_code(hlasm_ctx.ids().add("ANOP"), context::instruction_type::CA),
            processing_format(processing_kind::ORDINARY, processing_form::CA, operand_occurence::ABSENT)));
    add_correct_copy_nest();

    if (statement.operands_ref().value.size() == 1 && statement.operands_ref().value.front()->access_asm())
    {
        asm_processor::process_copy(statement, hlasm_ctx, provider_, this);
    }
    else
        add_diagnostic(diagnostic_op::error_E058(statement.operands_ref().field_range));

    omit_next_ = true;
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
            result_.sequence_symbols.emplace(seq.name,
                std::make_unique<context::macro_sequence_symbol>(
                    seq.name, hlasm_ctx.processing_stack().back().proc_location, curr_line_));
        }
    }
}

void macrodef_processor::add_correct_copy_nest()
{
    result_.nests.push_back({ location(curr_outer_position_, result_.definition_location.file) });

    for (size_t i = initial_copy_nest_; i < hlasm_ctx.current_copy_stack().size(); ++i)
    {
        auto& nest = hlasm_ctx.current_copy_stack()[i];
        auto pos = nest.cached_definition[nest.current_statement].get_base()->statement_position();
        auto loc = location(pos, nest.definition_location.file);
        result_.nests.back().push_back(std::move(loc));
    }
}

} // namespace hlasm_plugin::parser_library::processing
