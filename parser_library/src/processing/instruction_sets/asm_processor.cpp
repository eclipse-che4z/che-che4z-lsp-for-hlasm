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

#include "asm_processor.h"

#include "checking/instr_operand.h"
#include "context/literal_pool.h"
#include "context/ordinary_assembly/ordinary_assembly_dependency_solver.h"
#include "data_def_postponed_statement.h"
#include "ebcdic_encoding.h"
#include "expressions/mach_expr_term.h"
#include "expressions/mach_expr_visitor.h"
#include "postponed_statement_impl.h"


namespace hlasm_plugin::parser_library::processing {

namespace {
diagnostic_consumer_transform drop_diags([](diagnostic_op) {});

std::optional<context::A_t> try_get_abs_value(
    const semantics::simple_expr_operand* op, context::dependency_solver& dep_solver)
{
    if (op->has_dependencies(dep_solver))
        return std::nullopt;

    auto val = op->expression->evaluate(dep_solver, drop_diags);

    if (val.value_kind() != context::symbol_value_kind::ABS)
        return std::nullopt;
    return val.get_abs();
}

std::optional<context::A_t> try_get_abs_value(const semantics::operand* op, context::dependency_solver& dep_solver)
{
    auto expr_op = dynamic_cast<const semantics::simple_expr_operand*>(op);
    if (!expr_op)
        return std::nullopt;
    return try_get_abs_value(expr_op, dep_solver);
}
} // namespace

void asm_processor::process_sect(const context::section_kind kind, rebuilt_statement stmt)
{
    auto sect_name = find_label_symbol(stmt);

    using context::section_kind;
    const auto do_other_private_sections_exist = [this](context::id_index sect_name, section_kind kind) {
        for (auto k : { section_kind::COMMON, section_kind::EXECUTABLE, section_kind::READONLY })
        {
            if (k == kind)
                continue;
            if (hlasm_ctx.ord_ctx.section_defined(sect_name, k))
                return true;
        }
        return false;
    };

    const auto& processing_stack = hlasm_ctx.processing_stack();
    if (hlasm_ctx.ord_ctx.symbol_defined(sect_name)
        && (sect_name != context::id_storage::empty_id && !hlasm_ctx.ord_ctx.section_defined(sect_name, kind)
            || sect_name == context::id_storage::empty_id && kind != section_kind::DUMMY
                && do_other_private_sections_exist(sect_name, kind)))
    {
        add_diagnostic(diagnostic_op::error_E031("symbol", stmt.label_ref().field_range));
    }
    else
    {
        auto sym_loc = processing_stack.back().proc_location;
        sym_loc.pos.column = 0;
        hlasm_ctx.ord_ctx.set_section(sect_name, kind, std::move(sym_loc));
    }
    context::ordinary_assembly_dependency_solver dep_solver(hlasm_ctx.ord_ctx);
    hlasm_ctx.ord_ctx.symbol_dependencies.add_dependency(
        std::make_unique<postponed_statement_impl>(std::move(stmt), hlasm_ctx.processing_stack()),
        dep_solver.derive_current_dependency_evaluation_context());
}

void asm_processor::process_LOCTR(rebuilt_statement stmt)
{
    auto loctr_name = find_label_symbol(stmt);

    if (loctr_name == context::id_storage::empty_id)
        add_diagnostic(diagnostic_op::error_E053(stmt.label_ref().field_range));

    if (hlasm_ctx.ord_ctx.symbol_defined(loctr_name) && !hlasm_ctx.ord_ctx.counter_defined(loctr_name))
    {
        add_diagnostic(diagnostic_op::error_E031("symbol", stmt.label_ref().field_range));
    }
    else
    {
        auto sym_loc = hlasm_ctx.processing_stack().back().proc_location;
        sym_loc.pos.column = 0;
        hlasm_ctx.ord_ctx.set_location_counter(loctr_name, std::move(sym_loc));
    }
    context::ordinary_assembly_dependency_solver dep_solver(hlasm_ctx.ord_ctx);
    hlasm_ctx.ord_ctx.symbol_dependencies.add_dependency(
        std::make_unique<postponed_statement_impl>(std::move(stmt), hlasm_ctx.processing_stack()),
        dep_solver.derive_current_dependency_evaluation_context());
}

void asm_processor::process_EQU(rebuilt_statement stmt)
{
    auto loctr = hlasm_ctx.ord_ctx.align(context::no_align);
    context::ordinary_assembly_dependency_solver dep_solver(hlasm_ctx.ord_ctx, loctr);

    auto symbol_name = find_label_symbol(stmt);

    if (symbol_name == context::id_storage::empty_id)
    {
        if (stmt.label_ref().type == semantics::label_si_type::EMPTY)
            add_diagnostic(diagnostic_op::error_E053(stmt.label_ref().field_range));
        return;
    }

    if (hlasm_ctx.ord_ctx.symbol_defined(symbol_name))
    {
        add_diagnostic(diagnostic_op::error_E031("symbol", stmt.label_ref().field_range));
        return;
    }
    // type attribute operand
    context::symbol_attributes::type_attr t_attr = context::symbol_attributes::undef_type;
    if (stmt.operands_ref().value.size() >= 3 && stmt.operands_ref().value[2]->type == semantics::operand_type::ASM)
    {
        auto asm_op = stmt.operands_ref().value[2]->access_asm();
        auto expr_op = asm_op->access_expr();

        if (expr_op && !expr_op->has_dependencies(dep_solver))
        {
            auto t_value = expr_op->expression->evaluate(dep_solver, *this);
            if (t_value.value_kind() == context::symbol_value_kind::ABS && t_value.get_abs() >= 0
                && t_value.get_abs() <= 255)
                t_attr = (context::symbol_attributes::type_attr)t_value.get_abs();
            else
                add_diagnostic(diagnostic_op::error_A134_EQU_type_att_format(asm_op->operand_range));
        }
        else
            add_diagnostic(diagnostic_op::error_A134_EQU_type_att_format(asm_op->operand_range));
    }

    // length attribute operand
    context::symbol_attributes::len_attr length_attr = context::symbol_attributes::undef_length;
    if (stmt.operands_ref().value.size() >= 2 && stmt.operands_ref().value[1]->type == semantics::operand_type::ASM)
    {
        auto asm_op = stmt.operands_ref().value[1]->access_asm();
        auto expr_op = asm_op->access_expr();

        if (expr_op && !expr_op->has_dependencies(dep_solver))
        {
            auto length_value = expr_op->expression->evaluate(dep_solver, *this);
            if (length_value.value_kind() == context::symbol_value_kind::ABS && length_value.get_abs() >= 0
                && length_value.get_abs() <= 65535)
                length_attr = (context::symbol_attributes::len_attr)length_value.get_abs();
            else
                add_diagnostic(diagnostic_op::error_A133_EQU_len_att_format(asm_op->operand_range));
        }
        else
            add_diagnostic(diagnostic_op::error_A133_EQU_len_att_format(asm_op->operand_range));
    }

    // value operand
    if (stmt.operands_ref().value.size() != 0 && stmt.operands_ref().value[0]->type == semantics::operand_type::ASM)
    {
        auto asm_op = stmt.operands_ref().value[0]->access_asm();
        auto expr_op = asm_op->access_expr();

        if (expr_op)
        {
            auto holder(expr_op->expression->get_dependencies(dep_solver));

            if (length_attr == context::symbol_attributes::undef_length)
            {
                auto l_term = expr_op->expression->leftmost_term();
                if (auto symbol_term = dynamic_cast<const expressions::mach_expr_symbol*>(l_term))
                {
                    auto len_symbol = hlasm_ctx.ord_ctx.get_symbol(symbol_term->value);

                    if (len_symbol != nullptr && len_symbol->kind() != context::symbol_value_kind::UNDEF)
                        length_attr = len_symbol->attributes().length();
                    else
                        length_attr = 1;
                }
                else
                    length_attr = 1;
            }

            context::symbol_attributes attrs(context::symbol_origin::EQU, t_attr, length_attr);

            if (!holder.contains_dependencies())
            {
                create_symbol(
                    stmt.stmt_range_ref(), symbol_name, expr_op->expression->evaluate(dep_solver, *this), attrs);
            }
            else
            {
                if (!holder.is_address())
                {
                    bool cycle_ok = create_symbol(stmt.stmt_range_ref(), symbol_name, context::symbol_value(), attrs);

                    if (cycle_ok)
                    {
                        const auto& stmt_range = stmt.stmt_range_ref();
                        add_dependency(stmt_range,
                            symbol_name,
                            &*expr_op->expression,
                            std::make_unique<postponed_statement_impl>(std::move(stmt), hlasm_ctx.processing_stack()),
                            dep_solver.derive_current_dependency_evaluation_context());
                    }
                }
                else
                    create_symbol(stmt.stmt_range_ref(),
                        symbol_name,
                        *holder.unresolved_address,
                        attrs); // TODO: this is almost certainly wrong - unresolved space_sp are ignored
            }
        }
    }
}

template<checking::data_instr_type instr_type>
void asm_processor::process_data_instruction(rebuilt_statement stmt)
{
    if (const auto& ops = stmt.operands_ref().value; ops.empty()
        || std::any_of(
            ops.begin(), ops.end(), [](const auto& op) { return op->type == semantics::operand_type::EMPTY; }))
    {
        context::ordinary_assembly_dependency_solver dep_solver(hlasm_ctx.ord_ctx);
        hlasm_ctx.ord_ctx.symbol_dependencies.add_dependency(
            std::make_unique<postponed_statement_impl>(std::move(stmt), hlasm_ctx.processing_stack()),
            dep_solver.derive_current_dependency_evaluation_context());
        return;
    }

    // enforce alignment of the first operand
    context::alignment al = stmt.operands_ref().value.front()->access_data_def()->value->get_alignment();
    context::address loctr = hlasm_ctx.ord_ctx.align(al);
    context::ordinary_assembly_dependency_solver dep_solver(hlasm_ctx.ord_ctx, loctr);

    // process label
    auto label = find_label_symbol(stmt);

    if (label != context::id_storage::empty_id)
    {
        if (!hlasm_ctx.ord_ctx.symbol_defined(label))
        {
            auto data_op = stmt.operands_ref().value.front()->access_data_def();

            context::symbol_attributes::type_attr type =
                ebcdic_encoding::a2e[(unsigned char)data_op->value->get_type_attribute()];

            context::symbol_attributes::len_attr len = context::symbol_attributes::undef_length;
            context::symbol_attributes::scale_attr scale = context::symbol_attributes::undef_scale;

            if (!data_op->value->length
                || !data_op->value->length->get_dependencies(dep_solver).contains_dependencies())
            {
                len = data_op->value->get_length_attribute(dep_solver, drop_diags);
            }
            if (data_op->value->scale && !data_op->value->scale->get_dependencies(dep_solver).contains_dependencies())
            {
                scale = data_op->value->get_scale_attribute(dep_solver, drop_diags);
            }
            create_symbol(stmt.stmt_range_ref(),
                label,
                loctr,
                context::symbol_attributes(context::symbol_origin::DAT,
                    type,
                    len,
                    scale,
                    data_op->value->get_integer_attribute(dep_solver, drop_diags)));
        }
        else
            add_diagnostic(diagnostic_op::error_E031("symbol", stmt.label_ref().field_range));
    }

    const auto& operands = stmt.operands_ref().value;

    const context::resolvable* l_dep = nullptr;
    const context::resolvable* s_dep = nullptr;
    if (label != context::id_storage::empty_id)
    {
        auto data_op = operands.front()->access_data_def();

        if (data_op->value->length && data_op->value->length->get_dependencies(dep_solver).contains_dependencies())
            l_dep = data_op->value->length.get();

        if (data_op->value->scale && data_op->value->scale->get_dependencies(dep_solver).contains_dependencies())
            s_dep = data_op->value->scale.get();
    }

    // TODO issue warning when alignment is bigger than section's alignment
    // hlasm_ctx.ord_ctx.current_section()->current_location_counter().

    std::vector<data_def_dependency<instr_type>> dependencies;
    std::vector<context::space_ptr> dependencies_spaces;

    // Why is this so complicated?
    // 1. We cannot represent the individual operands because of bitfields.
    // 2. We cannot represent the whole area as a single dependency when the alignment requirements are growing.
    // Therefore, we split the operands into chunks depending on the alignent.
    // Whenever the alignment requirement increases between consecutive operands, we start a new chunk.
    for (auto it = operands.begin(); it != operands.end();)
    {
        const auto start = it;

        const auto initial_alignment = (*it)->access_data_def()->value->get_alignment();
        context::address op_loctr = hlasm_ctx.ord_ctx.align(initial_alignment);
        data_def_dependency_solver op_solver(dep_solver, &op_loctr);

        auto current_alignment = initial_alignment;

        // has_length_dependencies specifies whether the length of the data instruction can be resolved right now or
        // must be postponed
        bool has_length_dependencies = false;

        for (; it != operands.end(); ++it)
        {
            const auto& op = *it;

            auto data_op = op->access_data_def();
            auto op_align = data_op->value->get_alignment();

            // leave for the next round to make sure that the actual alignment is computed correctly
            if (op_align.boundary > current_alignment.boundary)
                break;
            current_alignment = op_align;

            has_length_dependencies |= data_op->get_length_dependencies(op_solver).contains_dependencies();

            // some types require operands that consist only of one symbol
            (void)data_op->value->check_single_symbol_ok(diagnostic_collector(this));
        }

        const auto* b = &*start;
        const auto* e = b + (it - start);

        if (has_length_dependencies)
        {
            dependencies.emplace_back(b, e, std::move(op_loctr));
            dependencies_spaces.emplace_back(hlasm_ctx.ord_ctx.register_ordinary_space(current_alignment));
        }
        else
        {
            auto length = data_def_dependency<instr_type>::get_operands_length(b, e, op_solver, drop_diags);
            hlasm_ctx.ord_ctx.reserve_storage_area(length, context::no_align);
        }
    }

    auto dep_stmt = std::make_unique<data_def_postponed_statement<instr_type>>(
        std::move(stmt), hlasm_ctx.processing_stack(), std::move(dependencies));
    const auto& deps = dep_stmt->get_dependencies();

    auto adder = hlasm_ctx.ord_ctx.symbol_dependencies.add_dependencies(
        std::move(dep_stmt), dep_solver.derive_current_dependency_evaluation_context());
    adder.add_dependency();

    bool cycle_ok = true;

    if (l_dep)
        cycle_ok &= adder.add_dependency(label, context::data_attr_kind::L, l_dep);
    if (s_dep)
        cycle_ok &= adder.add_dependency(label, context::data_attr_kind::S, s_dep);

    if (!cycle_ok)
        add_diagnostic(diagnostic_op::error_E033(operands.front()->operand_range));

    auto sp = dependencies_spaces.begin();
    for (const auto& d : deps)
        adder.add_dependency(std::move(*sp++), &d);

    adder.finish();
}

void asm_processor::process_DC(rebuilt_statement stmt)
{
    process_data_instruction<checking::data_instr_type::DC>(std::move(stmt));
}

void asm_processor::process_DS(rebuilt_statement stmt)
{
    process_data_instruction<checking::data_instr_type::DS>(std::move(stmt));
}

void asm_processor::process_COPY(rebuilt_statement stmt)
{
    find_sequence_symbol(stmt);

    if (stmt.operands_ref().value.size() == 1 && stmt.operands_ref().value.front()->type == semantics::operand_type::ASM
        && stmt.operands_ref().value.front()->access_asm()->access_expr())
    {
        process_copy(stmt, ctx, lib_provider, this);
    }
    else
    {
        context::ordinary_assembly_dependency_solver dep_solver(hlasm_ctx.ord_ctx);
        hlasm_ctx.ord_ctx.symbol_dependencies.add_dependency(
            std::make_unique<postponed_statement_impl>(std::move(stmt), hlasm_ctx.processing_stack()),
            dep_solver.derive_current_dependency_evaluation_context());
    }
}

void asm_processor::process_EXTRN(rebuilt_statement stmt) { process_external(std::move(stmt), external_type::strong); }

void asm_processor::process_WXTRN(rebuilt_statement stmt) { process_external(std::move(stmt), external_type::weak); }

void asm_processor::process_external(rebuilt_statement stmt, external_type t)
{
    if (auto label_type = stmt.label_ref().type; label_type != semantics::label_si_type::EMPTY)
    {
        if (label_type != semantics::label_si_type::SEQ)
            add_diagnostic(diagnostic_op::warning_A249_sequence_symbol_expected(stmt.label_ref().field_range));
        else
            find_sequence_symbol(stmt);
    }

    const auto add_external = [s_kind = t == external_type::strong ? context::section_kind::EXTERNAL
                                                                   : context::section_kind::WEAK_EXTERNAL,
                                  this](context::id_index name, range op_range) {
        if (hlasm_ctx.ord_ctx.symbol_defined(name))
            add_diagnostic(diagnostic_op::error_E031("external symbol", op_range));
        else
            hlasm_ctx.ord_ctx.create_external_section(
                name, s_kind, hlasm_ctx.current_statement_location(), hlasm_ctx.processing_stack());
    };
    for (const auto& op : stmt.operands_ref().value)
    {
        auto op_asm = op->access_asm();
        if (!op_asm)
            continue;

        if (auto expr = op_asm->access_expr())
        {
            if (auto sym = dynamic_cast<const expressions::mach_expr_symbol*>(expr->expression.get()))
                add_external(sym->value, expr->operand_range);
        }
        else if (auto complex = op_asm->access_complex())
        {
            if (context::to_upper_copy(complex->value.identifier) != "PART")
                continue;
            for (const auto& nested : complex->value.values)
            {
                if (const auto* string_val =
                        dynamic_cast<const semantics::complex_assembler_operand::string_value_t*>(nested.get());
                    !string_val->value.empty())
                    add_external(hlasm_ctx.ids().add(string_val->value), string_val->op_range);
            }
        }
    }
    context::ordinary_assembly_dependency_solver dep_solver(hlasm_ctx.ord_ctx);
    hlasm_ctx.ord_ctx.symbol_dependencies.add_dependency(
        std::make_unique<postponed_statement_impl>(std::move(stmt), hlasm_ctx.processing_stack()),
        dep_solver.derive_current_dependency_evaluation_context());
}

void asm_processor::process_ORG(rebuilt_statement stmt)
{
    find_sequence_symbol(stmt);

    auto label = find_label_symbol(stmt);
    auto loctr = hlasm_ctx.ord_ctx.align(context::no_align);

    if (label != context::id_storage::empty_id)
    {
        if (hlasm_ctx.ord_ctx.symbol_defined(label))
            add_diagnostic(diagnostic_op::error_E031("symbol", stmt.label_ref().field_range));
        else
            create_symbol(stmt.stmt_range_ref(), label, loctr, context::symbol_attributes::make_org_attrs());
    }
    context::ordinary_assembly_dependency_solver dep_solver(hlasm_ctx.ord_ctx, loctr);

    const semantics::expr_assembler_operand* reloc_expr = nullptr;
    bool undefined_absolute_part = false;
    size_t boundary = 0;
    int offset = 0;

    for (size_t i = 0; i < stmt.operands_ref().value.size(); ++i)
    {
        if (stmt.operands_ref().value[i]->type != semantics::operand_type::ASM)
            continue;

        auto asm_op = stmt.operands_ref().value[i]->access_asm();
        assert(asm_op);
        auto expr = asm_op->access_expr();
        if (!expr)
        {
            if (i == 0)
                add_diagnostic(diagnostic_op::error_A245_ORG_expression(stmt.stmt_range_ref()));
            else
                add_diagnostic(diagnostic_op::error_A115_ORG_op_format(stmt.stmt_range_ref()));
            break;
        }

        if (i == 0)
        {
            auto deps = expr->expression->get_dependencies(dep_solver);
            undefined_absolute_part =
                deps.undefined_attr_refs.size() || deps.undefined_symbols.size() || deps.unresolved_spaces.size();
            if (!deps.unresolved_address)
            {
                add_diagnostic(diagnostic_op::error_A245_ORG_expression(stmt.stmt_range_ref()));
                return;
            }
            reloc_expr = expr;
        }

        if (i == 1)
        {
            auto val = try_get_abs_value(expr, dep_solver);
            if (!val || *val < 2 || *val > 4096 || ((*val & (*val - 1)) != 0)) // check range and test for power of 2
            {
                add_diagnostic(diagnostic_op::error_A116_ORG_boundary_operand(stmt.stmt_range_ref()));
                return;
            }
            boundary = (size_t)*val;
        }
        if (i == 2)
        {
            auto val = try_get_abs_value(expr, dep_solver);
            if (!val)
            {
                add_diagnostic(diagnostic_op::error_A115_ORG_op_format(stmt.stmt_range_ref()));
                return;
            }
            offset = *val;
        }
    }

    if (reloc_expr)
    {
        auto reloc_val = !undefined_absolute_part
            ? reloc_expr->expression->evaluate(dep_solver, drop_diags).get_reloc()
            : *reloc_expr->expression->get_dependencies(dep_solver).unresolved_address;

        if (!check_address_for_ORG(stmt.stmt_range_ref(), reloc_val, loctr, boundary, offset))
            return;

        if (undefined_absolute_part)
            hlasm_ctx.ord_ctx.set_location_counter_value(reloc_val,
                boundary,
                offset,
                reloc_expr->expression.get(),
                std::make_unique<postponed_statement_impl>(std::move(stmt), hlasm_ctx.processing_stack()),
                dep_solver.derive_current_dependency_evaluation_context());
        else
            hlasm_ctx.ord_ctx.set_location_counter_value(reloc_val, boundary, offset);
    }
    else
        hlasm_ctx.ord_ctx.set_available_location_counter_value(boundary, offset);
}

void asm_processor::process_OPSYN(rebuilt_statement stmt)
{
    const auto& operands = stmt.operands_ref().value;

    auto label = find_label_symbol(stmt);
    if (label == context::id_storage::empty_id)
    {
        if (stmt.label_ref().type == semantics::label_si_type::EMPTY)
            add_diagnostic(diagnostic_op::error_E053(stmt.label_ref().field_range));
        return;
    }

    context::id_index operand = context::id_storage::empty_id;
    if (operands.size() == 1) // covers also the " , " case
    {
        auto asm_op = operands.front()->access_asm();
        if (asm_op)
        {
            auto expr_op = asm_op->access_expr();
            if (expr_op)
            {
                if (auto expr = dynamic_cast<const expressions::mach_expr_symbol*>(expr_op->expression.get()))
                    operand = expr->value;
            }
        }
    }

    if (operand == context::id_storage::empty_id)
    {
        if (hlasm_ctx.get_operation_code(label))
            hlasm_ctx.remove_mnemonic(label);
        else
            add_diagnostic(diagnostic_op::error_E049(*label, stmt.label_ref().field_range));
    }
    else
    {
        if (hlasm_ctx.get_operation_code(operand))
            hlasm_ctx.add_mnemonic(label, operand);
        else
            add_diagnostic(diagnostic_op::error_A246_OPSYN(operands.front()->operand_range));
    }

    context::ordinary_assembly_dependency_solver dep_solver(hlasm_ctx.ord_ctx);
    hlasm_ctx.ord_ctx.symbol_dependencies.add_dependency(
        std::make_unique<postponed_statement_impl>(std::move(stmt), hlasm_ctx.processing_stack()),
        dep_solver.derive_current_dependency_evaluation_context());
}

asm_processor::asm_processor(analyzing_context ctx,
    branching_provider& branch_provider,
    workspaces::parse_lib_provider& lib_provider,
    statement_fields_parser& parser,
    opencode_provider& open_code)
    : low_language_processor(ctx, branch_provider, lib_provider, parser)
    , table_(create_table(*ctx.hlasm_ctx))
    , open_code_(&open_code)
{}

void asm_processor::process(std::shared_ptr<const processing::resolved_statement> stmt)
{
    auto rebuilt_stmt = preprocess(stmt);

    register_literals(rebuilt_stmt, context::no_align, hlasm_ctx.ord_ctx.next_unique_id());

    auto it = table_.find(rebuilt_stmt.opcode_ref().value);
    if (it != table_.end())
    {
        auto& [key, func] = *it;
        func(std::move(rebuilt_stmt));
    }
    else
    {
        context::ordinary_assembly_dependency_solver dep_solver(hlasm_ctx.ord_ctx);
        hlasm_ctx.ord_ctx.symbol_dependencies.add_dependency(
            std::make_unique<postponed_statement_impl>(std::move(rebuilt_stmt), hlasm_ctx.processing_stack()),
            dep_solver.derive_current_dependency_evaluation_context());
    }
}

bool asm_processor::process_copy(const semantics::complete_statement& stmt,
    analyzing_context ctx,
    workspaces::parse_lib_provider& lib_provider,
    diagnosable_ctx* diagnoser)
{
    auto& expr = stmt.operands_ref().value.front()->access_asm()->access_expr()->expression;
    auto sym_expr = dynamic_cast<expressions::mach_expr_symbol*>(expr.get());

    if (!sym_expr)
    {
        if (diagnoser)
            diagnoser->add_diagnostic(diagnostic_op::error_E058(stmt.operands_ref().value.front()->operand_range));
        return false;
    }

    auto tmp = ctx.hlasm_ctx->copy_members().find(sym_expr->value);

    if (tmp == ctx.hlasm_ctx->copy_members().end())
    {
        bool result = lib_provider.parse_library(
            *sym_expr->value, ctx, workspaces::library_data { processing_kind::COPY, sym_expr->value });

        if (!result)
        {
            if (diagnoser)
                diagnoser->add_diagnostic(diagnostic_op::error_E058(stmt.operands_ref().value.front()->operand_range));
            return false;
        }
    }
    auto whole_copy_stack = ctx.hlasm_ctx->whole_copy_stack();

    auto cycle_tmp = std::find(whole_copy_stack.begin(), whole_copy_stack.end(), sym_expr->value);

    if (cycle_tmp != whole_copy_stack.end())
    {
        if (diagnoser)
            diagnoser->add_diagnostic(diagnostic_op::error_E062(stmt.stmt_range_ref()));
        return false;
    }

    ctx.hlasm_ctx->enter_copy_member(sym_expr->value);

    return true;
}

asm_processor::process_table_t asm_processor::create_table(context::hlasm_context& h_ctx)
{
    process_table_t table;
    table.emplace(h_ctx.ids().add("CSECT"),
        [this](rebuilt_statement stmt) { process_sect(context::section_kind::EXECUTABLE, std::move(stmt)); });
    table.emplace(h_ctx.ids().add("DSECT"),
        [this](rebuilt_statement stmt) { process_sect(context::section_kind::DUMMY, std::move(stmt)); });
    table.emplace(h_ctx.ids().add("RSECT"),
        [this](rebuilt_statement stmt) { process_sect(context::section_kind::READONLY, std::move(stmt)); });
    table.emplace(h_ctx.ids().add("COM"),
        [this](rebuilt_statement stmt) { process_sect(context::section_kind::COMMON, std::move(stmt)); });
    table.emplace(h_ctx.ids().add("LOCTR"), [this](rebuilt_statement stmt) { process_LOCTR(std::move(stmt)); });
    table.emplace(h_ctx.ids().add("EQU"), [this](rebuilt_statement stmt) { process_EQU(std::move(stmt)); });
    table.emplace(h_ctx.ids().add("DC"), [this](rebuilt_statement stmt) { process_DC(std::move(stmt)); });
    table.emplace(h_ctx.ids().add("DS"), [this](rebuilt_statement stmt) { process_DS(std::move(stmt)); });
    table.emplace(h_ctx.ids().well_known.COPY, [this](rebuilt_statement stmt) { process_COPY(std::move(stmt)); });
    table.emplace(h_ctx.ids().add("EXTRN"), [this](rebuilt_statement stmt) { process_EXTRN(std::move(stmt)); });
    table.emplace(h_ctx.ids().add("WXTRN"), [this](rebuilt_statement stmt) { process_WXTRN(std::move(stmt)); });
    table.emplace(h_ctx.ids().add("ORG"), [this](rebuilt_statement stmt) { process_ORG(std::move(stmt)); });
    table.emplace(h_ctx.ids().add("OPSYN"), [this](rebuilt_statement stmt) { process_OPSYN(std::move(stmt)); });
    table.emplace(h_ctx.ids().add("AINSERT"), [this](rebuilt_statement stmt) { process_AINSERT(std::move(stmt)); });
    table.emplace(h_ctx.ids().add("CCW"), [this](rebuilt_statement stmt) { process_CCW(std::move(stmt)); });
    table.emplace(h_ctx.ids().add("CCW0"), [this](rebuilt_statement stmt) { process_CCW(std::move(stmt)); });
    table.emplace(h_ctx.ids().add("CCW1"), [this](rebuilt_statement stmt) { process_CCW(std::move(stmt)); });
    table.emplace(h_ctx.ids().add("CNOP"), [this](rebuilt_statement stmt) { process_CNOP(std::move(stmt)); });
    table.emplace(h_ctx.ids().add("START"), [this](rebuilt_statement stmt) { process_START(std::move(stmt)); });
    table.emplace(h_ctx.ids().add("ALIAS"), [this](rebuilt_statement stmt) { process_ALIAS(std::move(stmt)); });
    table.emplace(h_ctx.ids().add("END"), [this](rebuilt_statement stmt) { process_END(std::move(stmt)); });
    table.emplace(h_ctx.ids().add("LTORG"), [this](rebuilt_statement stmt) { process_LTORG(std::move(stmt)); });
    table.emplace(h_ctx.ids().add("USING"), [this](rebuilt_statement stmt) { process_USING(std::move(stmt)); });
    table.emplace(h_ctx.ids().add("DROP"), [this](rebuilt_statement stmt) { process_DROP(std::move(stmt)); });
    table.emplace(h_ctx.ids().add("PUSH"), [this](rebuilt_statement stmt) { process_PUSH(std::move(stmt)); });
    table.emplace(h_ctx.ids().add("POP"), [this](rebuilt_statement stmt) { process_POP(std::move(stmt)); });

    return table;
}

context::id_index asm_processor::find_sequence_symbol(const rebuilt_statement& stmt)
{
    semantics::seq_sym symbol;
    switch (stmt.label_ref().type)
    {
        case semantics::label_si_type::SEQ:
            symbol = std::get<semantics::seq_sym>(stmt.label_ref().value);
            branch_provider.register_sequence_symbol(symbol.name, symbol.symbol_range);
            return symbol.name;
        default:
            return context::id_storage::empty_id;
    }
}

namespace {
class AINSERT_operand_visitor final : public expressions::mach_expr_visitor
{
public:
    // Inherited via mach_expr_visitor
    void visit(const expressions::mach_expr_constant&) override {}
    void visit(const expressions::mach_expr_data_attr&) override {}
    void visit(const expressions::mach_expr_data_attr_literal&) override {}
    void visit(const expressions::mach_expr_symbol& expr) override { value = expr.value; }
    void visit(const expressions::mach_expr_location_counter&) override {}
    void visit(const expressions::mach_expr_default&) override {}
    void visit(const expressions::mach_expr_literal&) override {}

    context::id_index value = nullptr;
};
} // namespace

void asm_processor::process_AINSERT(rebuilt_statement stmt)
{
    static constexpr std::string_view AINSERT = "AINSERT";
    const auto& ops = stmt.operands_ref();

    if (ops.value.size() != 2)
    {
        add_diagnostic(diagnostic_op::error_A011_exact(AINSERT, 2, ops.field_range));
        return;
    }

    auto second_op = dynamic_cast<const semantics::expr_assembler_operand*>(ops.value[1].get());
    if (!second_op)
    {
        add_diagnostic(diagnostic_op::error_A156_AINSERT_second_op_format(ops.value[1]->operand_range));
        return;
    }

    AINSERT_operand_visitor visitor;
    second_op->expression->apply(visitor);
    auto [value] = visitor;

    if (!value)
        return;
    processing::ainsert_destination dest;
    if (*value == "FRONT")
        dest = processing::ainsert_destination::front;
    else if (*value == "BACK")
        dest = processing::ainsert_destination::back;
    else
    {
        add_diagnostic(diagnostic_op::error_A156_AINSERT_second_op_format(ops.value[1]->operand_range));
        return;
    }

    if (auto arg = dynamic_cast<const semantics::string_assembler_operand*>(ops.value[0].get()))
    {
        const auto& record = arg->value;
        if (record.size() > checking::string_max_length)
        {
            add_diagnostic(diagnostic_op::error_A157_AINSERT_first_op_size(ops.value[0]->operand_range));
            return;
        }
        if (record.empty())
        {
            add_diagnostic(diagnostic_op::error_A021_cannot_be_empty(AINSERT, arg->operand_range));
            return;
        }

        open_code_->ainsert(record, dest);
    }
    else
    {
        add_diagnostic(diagnostic_op::error_A301_op_apostrophes_missing(AINSERT, ops.value[0]->operand_range));
    }
}

void asm_processor::process_CCW(rebuilt_statement stmt)
{
    constexpr context::alignment ccw_align = context::doubleword;
    constexpr size_t ccw_length = 8U;

    auto loctr = hlasm_ctx.ord_ctx.align(ccw_align);
    context::ordinary_assembly_dependency_solver dep_solver(hlasm_ctx.ord_ctx, loctr);
    find_sequence_symbol(stmt);

    if (auto label = find_label_symbol(stmt); label != context::id_storage::empty_id)
    {
        if (hlasm_ctx.ord_ctx.symbol_defined(label))
            add_diagnostic(diagnostic_op::error_E031("symbol", stmt.label_ref().field_range));
        else
            create_symbol(stmt.stmt_range_ref(), label, loctr, context::symbol_attributes::make_ccw_attrs());
    }

    hlasm_ctx.ord_ctx.reserve_storage_area(ccw_length, ccw_align);

    hlasm_ctx.ord_ctx.symbol_dependencies.add_dependency(
        std::make_unique<postponed_statement_impl>(std::move(stmt), hlasm_ctx.processing_stack()),
        dep_solver.derive_current_dependency_evaluation_context());
}

void asm_processor::process_CNOP(rebuilt_statement stmt)
{
    auto loctr = hlasm_ctx.ord_ctx.align(context::halfword);
    context::ordinary_assembly_dependency_solver dep_solver(hlasm_ctx.ord_ctx, loctr);
    find_sequence_symbol(stmt);

    if (auto label = find_label_symbol(stmt); label != context::id_storage::empty_id)
    {
        if (hlasm_ctx.ord_ctx.symbol_defined(label))
            add_diagnostic(diagnostic_op::error_E031("symbol", stmt.label_ref().field_range));
        else
            create_symbol(stmt.stmt_range_ref(), label, loctr, context::symbol_attributes::make_cnop_attrs());
    }

    if (stmt.operands_ref().value.size() == 2)
    {
        std::optional<int> byte_value = try_get_abs_value(stmt.operands_ref().value[0].get(), dep_solver);
        std::optional<int> boundary_value = try_get_abs_value(stmt.operands_ref().value[1].get(), dep_solver);
        // For now, the implementation ignores the instruction, if the operands have dependencies. Most uses of this
        // instruction should by covered anyway. It will still generate the label correctly.
        if (byte_value.has_value() && boundary_value.has_value())
            hlasm_ctx.ord_ctx.reserve_storage_area(
                0, context::alignment { (size_t)*byte_value, (size_t)*boundary_value });
    }

    hlasm_ctx.ord_ctx.symbol_dependencies.add_dependency(
        std::make_unique<postponed_statement_impl>(std::move(stmt), hlasm_ctx.processing_stack()),
        dep_solver.derive_current_dependency_evaluation_context());
}


void asm_processor::process_START(rebuilt_statement stmt)
{
    auto sect_name = find_label_symbol(stmt);

    if (std::any_of(hlasm_ctx.ord_ctx.sections().begin(), hlasm_ctx.ord_ctx.sections().end(), [](const auto& s) {
            return s->kind == context::section_kind::EXECUTABLE || s->kind == context::section_kind::READONLY;
        }))
    {
        add_diagnostic(diagnostic_op::error_E073(stmt.stmt_range_ref()));
        return;
    }

    if (hlasm_ctx.ord_ctx.symbol_defined(sect_name))
    {
        add_diagnostic(diagnostic_op::error_E031("symbol", stmt.label_ref().field_range));
        return;
    }

    const auto& processing_stack = hlasm_ctx.processing_stack();
    auto sym_loc = processing_stack.back().proc_location;
    sym_loc.pos.column = 0;
    hlasm_ctx.ord_ctx.set_section(sect_name, context::section_kind::EXECUTABLE, std::move(sym_loc));
    context::ordinary_assembly_dependency_solver dep_solver(hlasm_ctx.ord_ctx);

    const auto& ops = stmt.operands_ref().value;
    if (ops.size() != 1)
    {
        hlasm_ctx.ord_ctx.symbol_dependencies.add_dependency(
            std::make_unique<postponed_statement_impl>(std::move(stmt), hlasm_ctx.processing_stack()),
            dep_solver.derive_current_dependency_evaluation_context());
        return;
    }

    auto initial_offset = try_get_abs_value(ops.front().get(), dep_solver);
    if (!initial_offset.has_value())
    {
        add_diagnostic(diagnostic_op::error_A250_absolute_with_known_symbols(ops.front()->operand_range));
        return;
    }

    size_t start_section_alignment = hlasm_ctx.section_alignment().boundary;
    size_t start_section_alignment_mask = start_section_alignment - 1;

    auto offset = initial_offset.value();
    if (offset & start_section_alignment_mask)
    {
        // TODO: generate informational message?
        offset += start_section_alignment_mask;
        offset &= ~start_section_alignment_mask;
    }

    hlasm_ctx.ord_ctx.set_available_location_counter_value(start_section_alignment, offset);
}
void asm_processor::process_END(rebuilt_statement stmt)
{
    const auto& label = stmt.label_ref();
    context::ordinary_assembly_dependency_solver dep_solver(hlasm_ctx.ord_ctx);

    if (!(label.type == semantics::label_si_type::EMPTY || label.type == semantics::label_si_type::SEQ))
    {
        add_diagnostic(diagnostic_op::warning_A249_sequence_symbol_expected(stmt.label_ref().field_range));
    }
    if (!stmt.operands_ref().value.empty() && !(stmt.operands_ref().value[0]->type == semantics::operand_type::EMPTY))
    {
        if (stmt.operands_ref().value[0]->access_asm() != nullptr
            && stmt.operands_ref().value[0]->access_asm()->kind == semantics::asm_kind::EXPR)
        {
            auto symbol = stmt.operands_ref().value[0]->access_asm()->access_expr()->expression.get()->evaluate(
                dep_solver, drop_diags);

            if (symbol.value_kind() == context::symbol_value_kind::ABS)
            {
                add_diagnostic(
                    diagnostic_op::error_E032(std::to_string(symbol.get_abs()), stmt.operands_ref().field_range));
            }
        }
    }

    hlasm_ctx.ord_ctx.symbol_dependencies.add_dependency(
        std::make_unique<postponed_statement_impl>(std::move(stmt), hlasm_ctx.processing_stack()),
        dep_solver.derive_current_dependency_evaluation_context());

    hlasm_ctx.end_reached();
}
void asm_processor::process_ALIAS(rebuilt_statement stmt)
{
    auto symbol_name = find_label_symbol(stmt);
    if (symbol_name->empty())
    {
        add_diagnostic(diagnostic_op::error_A163_ALIAS_mandatory_label(stmt.stmt_range_ref()));
        return;
    }

    context::ordinary_assembly_dependency_solver dep_solver(hlasm_ctx.ord_ctx);
    hlasm_ctx.ord_ctx.symbol_dependencies.add_dependency(
        std::make_unique<postponed_statement_impl>(std::move(stmt), hlasm_ctx.processing_stack()),
        dep_solver.derive_current_dependency_evaluation_context());
}
void asm_processor::process_LTORG(rebuilt_statement stmt)
{
    constexpr size_t sectalgn = 8;
    auto loctr = hlasm_ctx.ord_ctx.align(context::alignment { 0, sectalgn });

    find_sequence_symbol(stmt);


    if (auto label = find_label_symbol(stmt); label != context::id_storage::empty_id)
    {
        if (hlasm_ctx.ord_ctx.symbol_defined(label))
            add_diagnostic(diagnostic_op::error_E031("symbol", stmt.label_ref().field_range));
        else
            create_symbol(stmt.stmt_range_ref(),
                label,
                loctr,
                context::symbol_attributes(context::symbol_origin::EQU, ebcdic_encoding::to_ebcdic('U'), 1));
    }

    hlasm_ctx.ord_ctx.generate_pool(*this, hlasm_ctx.using_current());

    context::ordinary_assembly_dependency_solver dep_solver(hlasm_ctx.ord_ctx);
    hlasm_ctx.ord_ctx.symbol_dependencies.add_dependency(
        std::make_unique<postponed_statement_impl>(std::move(stmt), hlasm_ctx.processing_stack()),
        dep_solver.derive_current_dependency_evaluation_context());
}

void asm_processor::process_USING(rebuilt_statement stmt)
{
    using namespace expressions;

    auto loctr = hlasm_ctx.ord_ctx.align(context::no_align);
    context::ordinary_assembly_dependency_solver dep_solver(hlasm_ctx.ord_ctx, loctr);

    auto label = find_using_label(stmt);

    if (label)
    {
        if (!hlasm_ctx.ord_ctx.symbol_defined(label))
        {
            hlasm_ctx.ord_ctx.register_using_label(label);
        }
        else if (!hlasm_ctx.ord_ctx.is_using_label(label))
        {
            add_diagnostic(diagnostic_op::error_E031("symbol", stmt.label_ref().field_range));
            return;
        }
    }
    mach_expr_ptr b;
    mach_expr_ptr e;

    const auto& ops = stmt.operands_ref().value;

    if (ops.size() < 2 || ops.size() > 17)
    {
        add_diagnostic(diagnostic_op::error_A012_from_to("USING", 2, 17, stmt.operands_ref().field_range));
        return;
    }

    if (ops.front()->type != semantics::operand_type::ASM)
    {
        add_diagnostic(diagnostic_op::error_A104_USING_first_format(ops.front()->operand_range));
        return;
    }

    switch (auto asm_op = ops.front()->access_asm(); asm_op->kind)
    {
        case hlasm_plugin::parser_library::semantics::asm_kind::EXPR:
            b = asm_op->access_expr()->expression->clone();
            break;

        case hlasm_plugin::parser_library::semantics::asm_kind::BASE_END: {
            auto using_op = asm_op->access_base_end();
            b = using_op->base->clone();
            e = using_op->end->clone();
            break;
        }
        default:
            add_diagnostic(diagnostic_op::error_A104_USING_first_format(asm_op->operand_range));
            return;
    }

    std::vector<mach_expr_ptr> bases;
    bases.reserve(ops.size() - 1);
    for (const auto& expr : std::span(ops).subspan(1))
    {
        if (expr->type != semantics::operand_type::ASM)
        {
            add_diagnostic(diagnostic_op::error_A164_USING_mapping_format(expr->operand_range));
            return;
        }
        else if (auto asm_expr = expr->access_asm()->access_expr(); !asm_expr)
        {
            add_diagnostic(diagnostic_op::error_A164_USING_mapping_format(expr->operand_range));
            return;
        }
        else
            bases.push_back(asm_expr->expression->clone());
    }

    hlasm_ctx.using_add(label,
        std::move(b),
        std::move(e),
        std::move(bases),
        dep_solver.derive_current_dependency_evaluation_context(),
        hlasm_ctx.processing_stack());
}

void asm_processor::process_DROP(rebuilt_statement stmt)
{
    using namespace expressions;

    auto loctr = hlasm_ctx.ord_ctx.align(context::no_align);
    context::ordinary_assembly_dependency_solver dep_solver(hlasm_ctx.ord_ctx, loctr);

    if (auto label = find_label_symbol(stmt); label != context::id_storage::empty_id)
    {
        if (hlasm_ctx.ord_ctx.symbol_defined(label))
        {
            add_diagnostic(diagnostic_op::error_E031("symbol", stmt.label_ref().field_range));
        }
        else
        {
            add_diagnostic(diagnostic_op::warn_A251_unexpected_label(stmt.label_ref().field_range));
            create_symbol(stmt.stmt_range_ref(), label, loctr, context::symbol_attributes(context::symbol_origin::EQU));
        }
    }

    const auto& ops = stmt.operands_ref().value;

    std::vector<mach_expr_ptr> bases;
    if (!ops.empty()
        && !(ops.size() == 2 && ops[0]->type == semantics::operand_type::EMPTY
            && ops[1]->type == semantics::operand_type::EMPTY))
    {
        bases.reserve(ops.size());
        for (const auto& op : ops)
        {
            if (auto asm_op = op->access_asm(); !asm_op)
                add_diagnostic(diagnostic_op::error_A141_DROP_op_format(op->operand_range));
            else if (auto expr = asm_op->access_expr(); !expr)
                add_diagnostic(diagnostic_op::error_A141_DROP_op_format(op->operand_range));
            else
                bases.push_back(expr->expression->clone());
        }
    }

    hlasm_ctx.using_remove(
        std::move(bases), dep_solver.derive_current_dependency_evaluation_context(), hlasm_ctx.processing_stack());
}

namespace {
bool asm_expr_quals(const semantics::operand_ptr& op, std::string_view value)
{
    auto asm_op = op->access_asm();
    if (!asm_op)
        return false;
    auto expr = asm_op->access_expr();
    return expr && expr->get_value() == value;
}
} // namespace

void asm_processor::process_PUSH(rebuilt_statement stmt)
{
    const auto& ops = stmt.operands_ref().value;

    if (std::any_of(ops.begin(), ops.end(), [](const auto& op) { return asm_expr_quals(op, "USING"); }))
        hlasm_ctx.using_push();

    context::ordinary_assembly_dependency_solver dep_solver(hlasm_ctx.ord_ctx);
    hlasm_ctx.ord_ctx.symbol_dependencies.add_dependency(
        std::make_unique<postponed_statement_impl>(std::move(stmt), hlasm_ctx.processing_stack()),
        dep_solver.derive_current_dependency_evaluation_context());
}

void asm_processor::process_POP(rebuilt_statement stmt)
{
    const auto& ops = stmt.operands_ref().value;

    if (std::any_of(ops.begin(), ops.end(), [](const auto& op) { return asm_expr_quals(op, "USING"); })
        && !hlasm_ctx.using_pop())
        add_diagnostic(diagnostic_op::error_A165_POP_USING(stmt.stmt_range_ref()));

    context::ordinary_assembly_dependency_solver dep_solver(hlasm_ctx.ord_ctx);
    hlasm_ctx.ord_ctx.symbol_dependencies.add_dependency(
        std::make_unique<postponed_statement_impl>(std::move(stmt), hlasm_ctx.processing_stack()),
        dep_solver.derive_current_dependency_evaluation_context());
}

} // namespace hlasm_plugin::parser_library::processing
