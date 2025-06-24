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
#include "context/ordinary_assembly/ordinary_assembly_dependency_solver.h"
#include "context/well_known.h"
#include "ebcdic_encoding.h"
#include "expressions/mach_expr_term.h"
#include "instructions/instruction.h"
#include "ordinary_processor.h"
#include "processing/branching_provider.h"
#include "processing/handler_map.h"
#include "processing/instruction_sets/asm_processor.h"
#include "processing/instruction_sets/instruction_processor.h"
#include "processing/statement.h"
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
        auto status = ordinary_processor::get_instruction_processing_status(*instruction, hlasm_ctx, nullptr);

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
            find_seq(resolved->label_ref());
            find_ord(*resolved);
        }
        if (opcode == context::well_known::MACRO)
        {
            process_MACRO();
        }
        else if (opcode == context::well_known::MEND)
        {
            process_MEND();
        }
        else if (macro_nest_ == 0 && opcode == context::well_known::COPY)
        {
            process_COPY(*resolved);
        }
        else if (opcode == context::well_known::END)
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

lookahead_processor::lookahead_processor(const analyzing_context& ctx,
    branching_provider& branch_provider,
    processing_state_listener& listener,
    parse_lib_provider& lib_provider,
    lookahead_start_data start,
    diagnosable_ctx& diag_ctx)
    : statement_processor(processing_kind::LOOKAHEAD, ctx, diag_ctx)
    , finished_flag_(start.action == lookahead_action::ORD && start.targets.empty())
    , result_(std::move(start))
    , macro_nest_(0)
    , branch_provider_(branch_provider)
    , listener_(listener)
    , lib_provider_(lib_provider)
    , to_find_(std::move(start.targets))
    , target_(start.target)
    , action(start.action)
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
                    asm_processor::common_copy_postprocess(result, *extract, *ctx.hlasm_ctx, &diag_ctx);
                });
        }
    }
}

namespace {
template<void (lookahead_processor::*ptr)(context::id_index, const resolved_statement&)>
constexpr auto fn = +[](lookahead_processor* self, context::id_index name, const resolved_statement& stmt) {
    (self->*ptr)(name, stmt);
};
} // namespace

struct lookahead_processor::handler_table
{
    using id_index = context::id_index;
    using callback = void(lookahead_processor*, context::id_index, const resolved_statement&);
    static constexpr auto value = make_handler_map<callback>({
        { id_index("CSECT"), fn<&lookahead_processor::assign_section_attributes> },
        { id_index("DSECT"), fn<&lookahead_processor::assign_section_attributes> },
        { id_index("RSECT"), fn<&lookahead_processor::assign_section_attributes> },
        { id_index("COM"), fn<&lookahead_processor::assign_section_attributes> },
        { id_index("DXD"), fn<&lookahead_processor::assign_section_attributes> },
        { id_index("LOCTR"), fn<&lookahead_processor::assign_section_attributes> },
        { id_index("EQU"), fn<&lookahead_processor::assign_EQU_attributes> },
        { id_index("DC"), fn<&lookahead_processor::assign_data_def_attributes> },
        { id_index("DS"), fn<&lookahead_processor::assign_data_def_attributes> },
        { id_index("CXD"), fn<&lookahead_processor::assign_cxd_attributes> },
    });

    static constexpr auto find(id_index id) noexcept { return value.find(id); }
};

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
            if (auto expr_op = asm_op->access_expr(); !expr_op) // complex, string, ...
                length_attr = 1;
            else if (auto t = dynamic_cast<const expressions::mach_expr_symbol*>(expr_op->expression->leftmost_term()))
            {
                auto len_symbol = hlasm_ctx.ord_ctx.get_symbol(t->value);

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
    const auto& ops = statement.operands_ref().value;

    const auto data_op = ops.empty() ? nullptr : ops.front()->access_data_def();
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

void lookahead_processor::assign_machine_attributes(context::id_index symbol_name, size_t len)
{
    using context::symbol_attributes;
    register_attr_ref(symbol_name, symbol_attributes::make_machine_attrs((symbol_attributes::len_attr)len));
}

void lookahead_processor::assign_assembler_attributes(
    context::id_index symbol_name, const resolved_statement& statement)
{
    if (const auto handler = handler_table::find(statement.opcode_ref().value))
        handler(this, symbol_name, statement);
    else
        register_attr_ref(symbol_name, context::symbol_attributes(context::symbol_origin::MACH, 'M'_ebcdic));
}

void lookahead_processor::assign_cxd_attributes(context::id_index symbol_name, const resolved_statement&)
{
    register_attr_ref(symbol_name, context::symbol_attributes(context::symbol_origin::ASM, 'A'_ebcdic, 4));
}

void lookahead_processor::find_seq(const semantics::label_si& label)
{
    if (label.type != semantics::label_si_type::SEQ)
        return;

    const auto& symbol = std::get<semantics::seq_sym>(label.value);

    branch_provider_.register_sequence_symbol(symbol.name, symbol.symbol_range);

    if (symbol.name == target_)
    {
        finished_flag_ = true;
        result_ = lookahead_processing_result(symbol.name, symbol.symbol_range);
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
    using enum context::instruction_type;
    switch (const auto& opcode = statement.opcode_ref(); opcode.type)
    {
        case CA:
            register_attr_ref(id, context::symbol_attributes(context::symbol_origin::MACH, 'U'_ebcdic));
            break;
        case UNDEF:
        case MAC:
            register_attr_ref(id, context::symbol_attributes(context::symbol_origin::MACH, 'M'_ebcdic));
            break;
        case MACH:
            assign_machine_attributes(id, opcode.instr_mach->size_in_bits() / 8);
            break;
        case MNEMO:
            assign_machine_attributes(id, opcode.instr_mnemo->size_in_bits() / 8);
            break;
        case ASM:
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
    hlasm_ctx.ord_ctx.add_symbol_reference(name, attributes, li);
}

} // namespace hlasm_plugin::parser_library::processing
