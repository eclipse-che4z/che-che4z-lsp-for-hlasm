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

namespace hlasm_plugin::parser_library::processing {

macrodef_processor::macrodef_processor(analyzing_context ctx,
    processing_state_listener& listener,
    workspaces::parse_lib_provider& provider,
    macrodef_start_data start)
    : statement_processor(processing_kind::MACRO, std::move(ctx))
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
    , table_(create_table())
{
    result_.definition_location = hlasm_ctx.processing_stack().back().proc_location;
    result_.external = start_.is_external;
    if (start_.is_external)
        result_.prototype.macro_name = start_.external_name;

    result_.invalid = true; // result starts invalid until mandatory statements are encountered
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
        if (auto** ca_instr = std::get_if<const context::ca_instruction*>(&code.opcode_detail); ca_instr)
        {
            processing_format format(processing_kind::MACRO,
                processing_form::CA,
                (*ca_instr)->operandless ? operand_occurence::ABSENT : operand_occurence::PRESENT);

            return std::make_pair(format, op_code(code.opcode, context::instruction_type::CA));
        }
        else if (code.opcode == hlasm_ctx.ids().add("COPY"))
        {
            processing_format format(processing_kind::MACRO, processing_form::ASM, operand_occurence::PRESENT);

            return std::make_pair(format, op_code(code.opcode, context::instruction_type::ASM));
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
        result_.invalid = false;
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

            if (auto found = table_.find(res_stmt->opcode_ref().value); found != table_.end())
            {
                auto& [key, func] = *found;
                func(*res_stmt);
            }
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
        if (var->created || var->subscript.size())
            add_diagnostic(diagnostic_op::error_E043(var->symbol_range));
        else
        {
            result_.prototype.name_param = var->access_basic()->name;
            result_.variable_symbols.emplace_back(var->access_basic()->name, curr_line_, var->symbol_range.start);
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
    result_.prototype.macro_name_range = statement.instruction_ref().field_range;
}

void macrodef_processor::process_prototype_operand(
    const resolved_statement& statement, std::vector<context::id_index>& param_names)
{
    for (auto& op : statement.operands_ref().value)
    {
        if (op->type == semantics::operand_type::EMPTY)
        {
            result_.prototype.symbolic_params.emplace_back(nullptr, nullptr);
            continue;
        }

        auto tmp = op->access_mac()->access_chain();
        assert(tmp);

        auto& tmp_chain = tmp->chain;

        if (tmp_chain.size() == 1 && tmp_chain[0]->type == semantics::concat_type::VAR) // if operand is varsym
        {
            auto var = tmp_chain[0]->access_var()->symbol.get();

            if (test_varsym_validity(var, param_names, tmp->operand_range, true))
            {
                auto var_id = var->access_basic()->name;
                param_names.push_back(var_id);
                result_.prototype.symbolic_params.emplace_back(nullptr, var_id);
                result_.variable_symbols.emplace_back(var->access_basic()->name, curr_line_, var->symbol_range.start);
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

                    diagnostic_adder add_diags(
                        [this](diagnostic_op diag) { this->add_diagnostic(std::move(diag)); }, op->operand_range);

                    result_.prototype.symbolic_params.emplace_back(
                        macro_processor::create_macro_data(tmp_chain.begin() + 2, tmp_chain.end(), add_diags), var_id);
                    result_.variable_symbols.emplace_back(
                        var->access_basic()->name, curr_line_, var->symbol_range.start);
                }
            }
            else
                add_diagnostic(diagnostic_op::error_E043(op->operand_range));
        }
        else if (tmp_chain.empty()) // if operand is empty
            result_.prototype.symbolic_params.emplace_back(nullptr, nullptr);
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

macrodef_processor::process_table_t macrodef_processor::create_table()
{
    process_table_t table;
    table.emplace(hlasm_ctx.ids().add("SETA"),
        [this](const resolved_statement& stmt) { process_SET(stmt, context::SET_t_enum::A_TYPE); });
    table.emplace(hlasm_ctx.ids().add("SETB"),
        [this](const resolved_statement& stmt) { process_SET(stmt, context::SET_t_enum::B_TYPE); });
    table.emplace(hlasm_ctx.ids().add("SETC"),
        [this](const resolved_statement& stmt) { process_SET(stmt, context::SET_t_enum::C_TYPE); });
    table.emplace(hlasm_ctx.ids().add("LCLA"),
        [this](const resolved_statement& stmt) { process_LCL_GBL(stmt, context::SET_t_enum::A_TYPE, false); });
    table.emplace(hlasm_ctx.ids().add("LCLB"),
        [this](const resolved_statement& stmt) { process_LCL_GBL(stmt, context::SET_t_enum::B_TYPE, false); });
    table.emplace(hlasm_ctx.ids().add("LCLC"),
        [this](const resolved_statement& stmt) { process_LCL_GBL(stmt, context::SET_t_enum::C_TYPE, false); });
    table.emplace(hlasm_ctx.ids().add("GBLA"),
        [this](const resolved_statement& stmt) { process_LCL_GBL(stmt, context::SET_t_enum::A_TYPE, true); });
    table.emplace(hlasm_ctx.ids().add("GBLB"),
        [this](const resolved_statement& stmt) { process_LCL_GBL(stmt, context::SET_t_enum::B_TYPE, true); });
    table.emplace(hlasm_ctx.ids().add("GBLC"),
        [this](const resolved_statement& stmt) { process_LCL_GBL(stmt, context::SET_t_enum::C_TYPE, true); });
    table.emplace(hlasm_ctx.ids().add("MACRO"), [this](const resolved_statement&) { process_MACRO(); });
    table.emplace(hlasm_ctx.ids().add("MEND"), [this](const resolved_statement&) { process_MEND(); });
    table.emplace(hlasm_ctx.ids().add("COPY"), [this](const resolved_statement& stmt) { process_COPY(stmt); });
    return table;
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

    auto empty_sem = std::make_shared<semantics::statement_si>(statement.stmt_range_ref(),
        semantics::label_si(statement.stmt_range_ref()),
        semantics::instruction_si(statement.stmt_range_ref()),
        semantics::operands_si(statement.stmt_range_ref(), {}),
        semantics::remarks_si(statement.stmt_range_ref(), {}));

    auto empty = std::make_unique<resolved_statement_impl>(std::move(empty_sem),
        processing_status(processing_format(processing_kind::ORDINARY, processing_form::CA, operand_occurence::ABSENT),
            op_code(hlasm_ctx.ids().add("ANOP"), context::instruction_type::CA)));

    result_.definition.push_back(std::move(empty));
    add_correct_copy_nest();

    if (statement.operands_ref().value.size() == 1 && statement.operands_ref().value.front()->access_asm())
    {
        if (asm_processor::process_copy(statement, ctx, provider_, this))
        {
            result_.used_copy_members.insert(ctx.hlasm_ctx->current_copy_stack().back().copy_member_definition);
        }
    }
    else
        add_diagnostic(diagnostic_op::error_E058(statement.operands_ref().field_range));

    omit_next_ = true;
}

void macrodef_processor::process_LCL_GBL(const resolved_statement& statement, context::SET_t_enum set_type, bool global)
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
}

void macrodef_processor::process_SET(const resolved_statement& statement, context::SET_t_enum set_type)
{
    if (statement.label_ref().type != semantics::label_si_type::VAR)
        return;

    auto var = std::get<semantics::vs_ptr>(statement.label_ref().value).get();

    add_SET_sym_to_res(var, set_type, false);
}

void macrodef_processor::add_SET_sym_to_res(
    const semantics::variable_symbol* var, context::SET_t_enum set_type, bool global)
{
    if (macro_nest_ > 1)
        return;
    if (var->created)
        return;

    if (std::find_if(result_.variable_symbols.begin(),
            result_.variable_symbols.end(),
            [&](const auto& def) { return def.name == var->access_basic()->name; })
        != result_.variable_symbols.end())
        return;

    result_.variable_symbols.emplace_back(
        var->access_basic()->name, set_type, global, curr_line_, var->symbol_range.start);
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
                    location(label.field_range.start, hlasm_ctx.current_statement_location().file),
                    curr_line_);
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
        result_.nests.back().emplace_back(pos, nest.definition_location.file);
    }

    if (initial_copy_nest_ < hlasm_ctx.current_copy_stack().size())
        result_.used_copy_members.insert(hlasm_ctx.current_copy_stack().back().copy_member_definition);

    const auto& current_file = result_.nests.back().back().file;
    bool in_inner_macro = macro_nest_ > 1;


    if (result_.file_scopes[current_file].empty())
    {
        if (curr_line_ == 1)
            result_.file_scopes[current_file].emplace_back(0, curr_line_, in_inner_macro);
        else
            result_.file_scopes[current_file].emplace_back(curr_line_, in_inner_macro);
    }
    else
    {
        bool inner_macro_ended = last_in_inner_macro_ && !in_inner_macro;
        bool inner_macro_started = !last_in_inner_macro_ && in_inner_macro;
        if (inner_macro_ended) // add new scope when inner macro ended
            result_.file_scopes[current_file].emplace_back(curr_line_, in_inner_macro);
        else if (!in_inner_macro
            || inner_macro_started) // if we are not in inner macro, update the end of old scope. Update also when inner
                                    // macro just started, since we use half-open intervals.
        {
            auto& last_scope = result_.file_scopes[current_file].back();
            last_scope.end_statement = curr_line_;
        }
    }

    last_in_inner_macro_ = in_inner_macro;
}

} // namespace hlasm_plugin::parser_library::processing
