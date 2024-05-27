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

#include "lookahead_processor.h"

#include "context/hlasm_context.h"
#include "context/instruction.h"
#include "context/ordinary_assembly/ordinary_assembly_dependency_solver.h"
#include "ebcdic_encoding.h"
#include "expressions/mach_expr_term.h"
#include "ordinary_processor.h"
#include "processing/branching_provider.h"
#include "processing/instruction_sets/asm_processor.h"
#include "semantics/operand_impls.h"

namespace hlasm_plugin::parser_library::processing {


std::optional<context::id_index> lookahead_processor::resolve_concatenation(
    const semantics::concat_chain&, const range&) const
{
    return std::nullopt;
}

std::optional<processing_status> lookahead_processor::get_processing_status(
    const std::optional<context::id_index>& instruction, const range&) const
{
    // Lookahead processor always returns value
    if (instruction.has_value() && !instruction->empty())
    {
        auto status = ordinary_processor::get_instruction_processing_status(*instruction, hlasm_ctx);

        if (status)
        {
            status->first.kind = processing_kind::LOOKAHEAD;

            if (status->second.type == context::instruction_type::CA
                || status->second.type == context::instruction_type::MAC)
                status->first.form = processing_form::IGNORED;

            return *status;
        }
    }

    return std::make_pair(processing_format(processing_kind::LOOKAHEAD, processing_form::IGNORED), op_code());
}

void lookahead_processor::process_statement(context::shared_stmt_ptr statement)
{
    if (auto resolved = statement->access_resolved())
    {
        auto opcode = resolved->opcode_ref().value;

        if (macro_nest_ == 0)
        {
            find_seq(*resolved);
            find_ord(*resolved);
        }
        if (opcode == context::id_storage::well_known::MACRO)
        {
            process_MACRO();
        }
        else if (opcode == context::id_storage::well_known::MEND)
        {
            process_MEND();
        }
        else if (macro_nest_ == 0 && opcode == context::id_storage::well_known::COPY)
        {
            process_COPY(*resolved);
        }
        else if (opcode == context::id_storage::well_known::END)
        {
            finished_flag_ = true;
        }
    }
}

void lookahead_processor::end_processing()
{
    hlasm_ctx.pop_statement_processing();

    for (auto&& symbol_name : to_find_)
        register_attr_ref(symbol_name, context::symbol_attributes(context::symbol_origin::UNKNOWN));

    listener_.finish_lookahead(std::move(result_));

    finished_flag_ = true;
}

bool lookahead_processor::terminal_condition(const statement_provider_kind prov_kind) const
{
    return prov_kind == statement_provider_kind::MACRO || prov_kind == statement_provider_kind::OPEN;
}

bool lookahead_processor::finished() { return finished_flag_; }

void lookahead_processor::collect_diags() const {}

lookahead_processor::lookahead_processor(const analyzing_context& ctx,
    branching_provider& branch_provider,
    processing_state_listener& listener,
    parse_lib_provider& lib_provider,
    lookahead_start_data start)
    : statement_processor(processing_kind::LOOKAHEAD, ctx)
    , finished_flag_(start.action == lookahead_action::ORD && start.targets.empty())
    , result_(std::move(start))
    , macro_nest_(0)
    , branch_provider_(branch_provider)
    , listener_(listener)
    , lib_provider_(lib_provider)
    , to_find_(std::move(start.targets))
    , target_(start.target)
    , action(start.action)
    , asm_proc_table_(create_table())
{}

void lookahead_processor::process_MACRO() { ++macro_nest_; }
void lookahead_processor::process_MEND() { macro_nest_ -= macro_nest_ == 0 ? 0 : 1; }
void lookahead_processor::process_COPY(const resolved_statement& statement)
{
    if (auto extract = asm_processor::extract_copy_id(statement, nullptr); extract.has_value())
    {
        if (ctx.hlasm_ctx->copy_members().contains(extract->name))
            asm_processor::common_copy_postprocess(true, *extract, *ctx.hlasm_ctx, nullptr);
        else
        {
            branch_provider_.request_external_processing(
                extract->name, processing_kind::COPY, [extract, this](bool result) {
                    asm_processor::common_copy_postprocess(result, *extract, *ctx.hlasm_ctx, this);
                });
        }
    }
}

lookahead_processor::process_table_t lookahead_processor::create_table()
{
    process_table_t table;
    using context::id_index;
    table.try_emplace(id_index("CSECT"), std::bind_front(&lookahead_processor::assign_section_attributes, this));
    table.try_emplace(id_index("DSECT"), std::bind_front(&lookahead_processor::assign_section_attributes, this));
    table.try_emplace(id_index("RSECT"), std::bind_front(&lookahead_processor::assign_section_attributes, this));
    table.try_emplace(id_index("COM"), std::bind_front(&lookahead_processor::assign_section_attributes, this));
    table.try_emplace(id_index("DXD"), std::bind_front(&lookahead_processor::assign_section_attributes, this));
    table.try_emplace(id_index("LOCTR"), std::bind_front(&lookahead_processor::assign_section_attributes, this));
    table.try_emplace(id_index("EQU"), std::bind_front(&lookahead_processor::assign_EQU_attributes, this));
    table.try_emplace(id_index("DC"), std::bind_front(&lookahead_processor::assign_data_def_attributes, this));
    table.try_emplace(id_index("DS"), std::bind_front(&lookahead_processor::assign_data_def_attributes, this));
    table.try_emplace(id_index("CXD"), std::bind_front(&lookahead_processor::assign_cxd_attributes, this));

    return table;
}

void lookahead_processor::assign_EQU_attributes(context::id_index symbol_name, const resolved_statement& statement)
{
    library_info_transitional li(lib_provider_);
    context::ordinary_assembly_dependency_solver dep_solver(hlasm_ctx.ord_ctx, li);
    // type attribute operand
    context::symbol_attributes::type_attr t_attr = context::symbol_attributes::undef_type;
    if (statement.operands_ref().value.size() >= 3
        && statement.operands_ref().value[2]->type == semantics::operand_type::ASM)
    {
        auto asm_op = statement.operands_ref().value[2]->access_asm();
        auto expr_op = asm_op->access_expr();

        if (expr_op && !expr_op->has_error(dep_solver) && !expr_op->has_dependencies(dep_solver, nullptr))
        {
            auto t_value = expr_op->expression->evaluate(dep_solver, drop_diagnostic_op);
            if (t_value.value_kind() == context::symbol_value_kind::ABS && t_value.get_abs() >= 0
                && t_value.get_abs() <= 255)
                t_attr = (context::symbol_attributes::type_attr)t_value.get_abs();
        }
    }

    // length attribute operand
    context::symbol_attributes::len_attr length_attr = context::symbol_attributes::undef_length;
    if (statement.operands_ref().value.size() >= 2
        && statement.operands_ref().value[1]->type == semantics::operand_type::ASM)
    {
        auto asm_op = statement.operands_ref().value[1]->access_asm();
        auto expr_op = asm_op->access_expr();

        if (expr_op && !expr_op->has_error(dep_solver) && !expr_op->has_dependencies(dep_solver, nullptr))
        {
            auto length_value = expr_op->expression->evaluate(dep_solver, drop_diagnostic_op);
            if (length_value.value_kind() == context::symbol_value_kind::ABS && length_value.get_abs() >= 0
                && length_value.get_abs() <= 65535)
                length_attr = (context::symbol_attributes::len_attr)length_value.get_abs();
        }
    }

    if (length_attr == context::symbol_attributes::undef_length)
    {
        if (statement.operands_ref().value.size() >= 1
            && statement.operands_ref().value[0]->type == semantics::operand_type::ASM)
        {
            auto asm_op = statement.operands_ref().value[0]->access_asm();
            auto expr_op = asm_op->access_expr();
            if (!expr_op)
                return;

            auto l_term = expr_op->expression->leftmost_term();
            if (auto symbol_term = dynamic_cast<const expressions::mach_expr_symbol*>(l_term))
            {
                auto len_symbol = hlasm_ctx.ord_ctx.get_symbol(symbol_term->value);

                if (len_symbol != nullptr && len_symbol->kind() != context::symbol_value_kind::UNDEF)
                    length_attr = len_symbol->attributes().length();
            }
            else
                length_attr = 1;
        }
    }

    register_attr_ref(symbol_name, context::symbol_attributes(context::symbol_origin::DAT, t_attr, length_attr));
}

void lookahead_processor::assign_data_def_attributes(context::id_index symbol_name, const resolved_statement& statement)
{
    if (statement.operands_ref().value.empty())
        return;

    auto data_op = statement.operands_ref().value.front()->access_data_def();

    if (!data_op)
    {
        register_attr_ref(symbol_name,
            context::symbol_attributes(context::symbol_origin::DAT,
                'U'_ebcdic,
                context::symbol_attributes::undef_length,
                context::symbol_attributes::undef_scale));
        return;
    }

    context::symbol_attributes::type_attr type =
        ebcdic_encoding::to_ebcdic((unsigned char)data_op->value->get_type_attribute());

    context::symbol_attributes::len_attr len = context::symbol_attributes::undef_length;
    context::symbol_attributes::scale_attr scale = context::symbol_attributes::undef_scale;

    library_info_transitional li(lib_provider_);
    context::ordinary_assembly_dependency_solver dep_solver(hlasm_ctx.ord_ctx, li);

    if (!data_op->value->length || !data_op->value->length->get_dependencies(dep_solver).contains_dependencies())
    {
        len = data_op->value->get_length_attribute(dep_solver, drop_diagnostic_op);
    }
    if (data_op->value->scale && !data_op->value->scale->get_dependencies(dep_solver).contains_dependencies())
    {
        scale = data_op->value->get_scale_attribute(dep_solver, drop_diagnostic_op);
    }

    register_attr_ref(symbol_name, context::symbol_attributes(context::symbol_origin::DAT, type, len, scale));
}

void lookahead_processor::assign_section_attributes(context::id_index symbol_name, const resolved_statement&)
{
    register_attr_ref(symbol_name, context::symbol_attributes::make_section_attrs());
}

void lookahead_processor::assign_machine_attributes(context::id_index symbol_name, const resolved_statement& statement)
{
    const auto [mi, _] =
        context::instruction::find_machine_instruction_or_mnemonic(statement.opcode_ref().value.to_string_view());

    register_attr_ref(symbol_name,
        context::symbol_attributes::make_machine_attrs((context::symbol_attributes::len_attr)mi->size_in_bits() / 8));
}

void lookahead_processor::assign_assembler_attributes(
    context::id_index symbol_name, const resolved_statement& statement)
{
    auto it = asm_proc_table_.find(statement.opcode_ref().value);
    if (it != asm_proc_table_.end())
    {
        auto& [key, func] = *it;
        func(symbol_name, statement);
    }
    else
        register_attr_ref(symbol_name, context::symbol_attributes(context::symbol_origin::MACH, 'M'_ebcdic));
}

void lookahead_processor::assign_cxd_attributes(context::id_index symbol_name, const resolved_statement&)
{
    register_attr_ref(symbol_name, context::symbol_attributes(context::symbol_origin::ASM, 'A'_ebcdic, 4));
}

void lookahead_processor::find_seq(const semantics::core_statement& statement)
{
    if (statement.label_ref().type == semantics::label_si_type::SEQ)
    {
        const auto& symbol = std::get<semantics::seq_sym>(statement.label_ref().value);

        branch_provider_.register_sequence_symbol(symbol.name, symbol.symbol_range);

        if (symbol.name == target_)
        {
            finished_flag_ = true;
            result_ = lookahead_processing_result(symbol.name, symbol.symbol_range);
        }
    }
}

void lookahead_processor::find_ord(const resolved_statement& statement)
{
    // checks
    if (statement.label_ref().type != semantics::label_si_type::ORD)
        return;

    auto [valid, id] =
        hlasm_ctx.try_get_symbol_name(std::get<semantics::ord_symbol_string>(statement.label_ref().value).symbol);
    if (!valid)
        return;

    if (auto it = std::ranges::find(to_find_, id); it != to_find_.end())
    {
        std::swap(*it, to_find_.back());
        to_find_.pop_back();
    }

    // find attributes
    // if found ord symbol on CA, macro or undefined instruction, only type attribute is assigned
    // 'U' for CA and 'M' for undefined and macro
    switch (statement.opcode_ref().type)
    {
        case context::instruction_type::CA:
        case context::instruction_type::UNDEF:
        case context::instruction_type::MAC:
            register_attr_ref(id,
                context::symbol_attributes(context::symbol_origin::MACH,
                    statement.opcode_ref().type == context::instruction_type::CA ? 'U'_ebcdic : 'M'_ebcdic));
            break;
        case context::instruction_type::MACH:
            assign_machine_attributes(id, statement);
            break;
        case context::instruction_type::ASM:
            assign_assembler_attributes(id, statement);
            break;
        default:
            assert(false);
            break;
    }

    finished_flag_ = action == lookahead_action::ORD && to_find_.empty();
}

void lookahead_processor::register_attr_ref(context::id_index name, context::symbol_attributes attributes)
{
    library_info_transitional li(lib_provider_);
    hlasm_ctx.ord_ctx.add_symbol_reference(
        context::symbol(name, context::symbol_value(), attributes, location(), {}), li);
}

} // namespace hlasm_plugin::parser_library::processing
