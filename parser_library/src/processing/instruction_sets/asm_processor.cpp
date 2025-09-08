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

#include <charconv>
#include <memory>
#include <optional>
#include <ranges>

#include "analyzing_context.h"
#include "checking/asm_instr_check.h"
#include "checking/diagnostic_collector.h"
#include "context/common_types.h"
#include "context/hlasm_context.h"
#include "context/literal_pool.h"
#include "context/ordinary_assembly/location_counter.h"
#include "context/ordinary_assembly/ordinary_assembly_dependency_solver.h"
#include "context/ordinary_assembly/symbol_dependency_tables.h"
#include "context/well_known.h"
#include "data_def_postponed_statement.h"
#include "diagnosable_ctx.h"
#include "ebcdic_encoding.h"
#include "expressions/mach_expr_term.h"
#include "expressions/mach_expr_visitor.h"
#include "output_handler.h"
#include "parse_lib_provider.h"
#include "postponed_statement_impl.h"
#include "processing/branching_provider.h"
#include "processing/handler_map.h"
#include "processing/opencode_provider.h"
#include "processing/processing_manager.h"
#include "processing/statement.h"
#include "processing/statement_fields_parser.h"
#include "range.h"
#include "semantics/operand_impls.h"
#include "utils/string_operations.h"
#include "utils/unicode_text.h"

namespace hlasm_plugin::parser_library::processing {

namespace {
std::optional<context::A_t> try_get_abs_value(
    const semantics::expr_assembler_operand* op, context::dependency_solver& dep_solver)
{
    if (op->has_dependencies(dep_solver, nullptr))
        return std::nullopt;

    auto val = op->expression->evaluate(dep_solver, drop_diagnostic_op);

    if (val.value_kind() != context::symbol_value_kind::ABS)
        return std::nullopt;
    return val.get_abs();
}

std::optional<context::A_t> try_get_abs_value(
    const semantics::assembler_operand* asm_op, context::dependency_solver& dep_solver)
{
    auto* expr_op = asm_op->access_expr();
    if (!expr_op)
        return std::nullopt;
    return try_get_abs_value(expr_op, dep_solver);
}

std::optional<context::A_t> try_get_abs_value(const semantics::operand* op, context::dependency_solver& dep_solver)
{
    auto* asm_op = op->access_asm();
    if (!asm_op)
        return std::nullopt;
    return try_get_abs_value(asm_op, dep_solver);
}

std::optional<int> try_get_number(std::string_view s)
{
    int v = 0;
    const char* b = s.data();
    const char* e = b + s.size();
    if (auto ec = std::from_chars(b, e, v); ec.ec == std::errc {} && ec.ptr == e)
        return v;
    return std::nullopt;
}

template<auto ptr, auto... others>
constexpr auto fn = +[](asm_processor* self, rebuilt_statement&& stmt) { (self->*ptr)(std::move(stmt), others...); };
} // namespace

struct asm_processor::handler_table
{
    using wk = context::well_known;
    using id_index = context::id_index;
    using callback = void(asm_processor* self, rebuilt_statement&& stmt);
    static constexpr auto value = make_handler_map<callback>({
        { id_index("CSECT"), fn<&asm_processor::process_sect, context::section_kind::EXECUTABLE> },
        { id_index("DSECT"), fn<&asm_processor::process_sect, context::section_kind::DUMMY> },
        { id_index("RSECT"), fn<&asm_processor::process_sect, context::section_kind::READONLY> },
        { id_index("COM"), fn<&asm_processor::process_sect, context::section_kind::COMMON> },
        { id_index("LOCTR"), fn<&asm_processor::process_LOCTR> },
        { id_index("EQU"), fn<&asm_processor::process_EQU> },
        { id_index("DC"), fn<&asm_processor::process_DC> },
        { id_index("DS"), fn<&asm_processor::process_DS> },
        { wk::COPY, fn<&asm_processor::process_COPY> },
        { id_index("DXD"), fn<&asm_processor::process_DXD> },
        { id_index("EXTRN"), fn<&asm_processor::process_EXTRN> },
        { id_index("WXTRN"), fn<&asm_processor::process_WXTRN> },
        { id_index("ORG"), fn<&asm_processor::process_ORG> },
        { id_index("OPSYN"), fn<&asm_processor::process_OPSYN> },
        { id_index("AINSERT"), fn<&asm_processor::process_AINSERT> },
        { id_index("CCW"), fn<&asm_processor::process_CCW> },
        { id_index("CCW0"), fn<&asm_processor::process_CCW> },
        { id_index("CCW1"), fn<&asm_processor::process_CCW> },
        { id_index("CNOP"), fn<&asm_processor::process_CNOP> },
        { id_index("START"), fn<&asm_processor::process_START> },
        { id_index("ALIAS"), fn<&asm_processor::process_ALIAS> },
        { id_index("END"), fn<&asm_processor::process_END> },
        { id_index("LTORG"), fn<&asm_processor::process_LTORG> },
        { id_index("USING"), fn<&asm_processor::process_USING> },
        { id_index("DROP"), fn<&asm_processor::process_DROP> },
        { id_index("PUSH"), fn<&asm_processor::process_PUSH> },
        { id_index("POP"), fn<&asm_processor::process_POP> },
        { id_index("MNOTE"), fn<&asm_processor::process_MNOTE> },
        { id_index("CXD"), fn<&asm_processor::process_CXD> },
        { id_index("TITLE"), fn<&asm_processor::process_TITLE> },
        { id_index("PUNCH"), fn<&asm_processor::process_PUNCH> },
        { id_index("CATTR"), fn<&asm_processor::process_CATTR> },
        { id_index("XATTR"), fn<&asm_processor::process_XATTR> },
    });

    static constexpr auto find(id_index id) noexcept { return value.find(id); }
};

void asm_processor::process_sect(rebuilt_statement&& stmt, const context::section_kind kind)
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

    if (!sect_name.empty() && hlasm_ctx.ord_ctx.symbol_defined(sect_name)
            && !hlasm_ctx.ord_ctx.section_defined(sect_name, kind)
        || sect_name.empty() && kind != section_kind::DUMMY && do_other_private_sections_exist(sect_name, kind))
    {
        add_diagnostic(diagnostic_op::error_E031("symbol", stmt.label_ref().field_range));
    }
    else
        hlasm_ctx.ord_ctx.set_section(sect_name, kind, lib_info);

    context::ordinary_assembly_dependency_solver dep_solver(hlasm_ctx.ord_ctx, lib_info);
    hlasm_ctx.ord_ctx.symbol_dependencies().add_postponed_statement(
        std::make_unique<postponed_statement_impl>(std::move(stmt), hlasm_ctx.processing_stack()),
        std::move(dep_solver).derive_current_dependency_evaluation_context());
}

void asm_processor::process_LOCTR(rebuilt_statement&& stmt)
{
    auto loctr_name = find_label_symbol(stmt);

    if (loctr_name.empty())
        add_diagnostic(diagnostic_op::error_E053(stmt.label_ref().field_range));

    if (hlasm_ctx.ord_ctx.symbol_defined(loctr_name) && !hlasm_ctx.ord_ctx.counter_defined(loctr_name))
        add_diagnostic(diagnostic_op::error_E031("symbol", stmt.label_ref().field_range));
    else
        hlasm_ctx.ord_ctx.set_location_counter(loctr_name, lib_info);

    context::ordinary_assembly_dependency_solver dep_solver(hlasm_ctx.ord_ctx, lib_info);
    hlasm_ctx.ord_ctx.symbol_dependencies().add_postponed_statement(
        std::make_unique<postponed_statement_impl>(std::move(stmt), hlasm_ctx.processing_stack()),
        std::move(dep_solver).derive_current_dependency_evaluation_context());
}

namespace {
struct override_symbol_candidates final : public context::dependency_solver_redirect
{
    std::variant<const context::symbol*, context::symbol_candidate> get_symbol_candidate(
        context::id_index name) const override
    {
        if (auto r = dependency_solver_redirect::get_symbol_candidate(name);
            std::holds_alternative<const context::symbol*>(r) && std::get<const context::symbol*>(r) == nullptr)
            return context::symbol_candidate { false };
        else
            return r;
    }

    explicit override_symbol_candidates(context::dependency_solver& solver)
        : dependency_solver_redirect(solver)
    {}
};

template<std::size_t n>
auto extract_asm_operands(std::span<const semantics::operand_ptr> ops)
{
    std::array<const semantics::assembler_operand*, n> result = {};

    std::ranges::transform(ops | std::views::take(n), result.data(), [](const auto& p) { return p->access_asm(); });

    return result;
}
} // namespace

void asm_processor::process_EQU(rebuilt_statement&& stmt)
{
    using context::symbol_attributes;

    auto loctr = hlasm_ctx.ord_ctx.align(context::no_align);
    context::ordinary_assembly_dependency_solver dep_solver(hlasm_ctx.ord_ctx, std::move(loctr), lib_info);

    auto symbol_name = find_label_symbol(stmt);

    if (symbol_name.empty())
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

    const auto& ops = stmt.operands_ref().value;

    if (ops.empty() || ops.size() > 5)
    {
        add_diagnostic(diagnostic_op::error_A012_from_to("EQU", 1, 5, stmt.stmt_range_ref()));
        return;
    }

    override_symbol_candidates dep_solver_override(dep_solver);
    const auto [value, length, type, prog_type, asm_type] = extract_asm_operands<5>(ops);

    // assembler type attribute
    symbol_attributes::assembler_type a_attr = symbol_attributes::assembler_type::NONE;
    if (asm_type)
    {
        std::string_view a_value;
        if (const auto* expr = asm_type->access_expr())
            a_value = expr->get_value();
        // relies on to_upper_case in the parser
        a_attr = context::assembler_type_from_string(a_value);
        if (a_attr == symbol_attributes::assembler_type::NONE)
            add_diagnostic(diagnostic_op::error_A135_EQU_asm_type_val_format(asm_type->operand_range));
    }

    // program type attribute
    symbol_attributes::program_type p_attr {};
    if (prog_type)
    {
        const auto p_value = try_get_abs_value(prog_type, dep_solver_override);
        if (!p_value)
            add_diagnostic(diagnostic_op::error_A174_EQU_prog_type_val_format(prog_type->operand_range));
        else
            p_attr = symbol_attributes::program_type((std::uint32_t)*p_value);
    }

    // type attribute operand
    symbol_attributes::type_attr t_attr = symbol_attributes::undef_type;
    if (type)
    {
        const auto t_value = try_get_abs_value(type, dep_solver_override);
        if (!t_value || t_value < 0 || t_value > 255)
            add_diagnostic(diagnostic_op::error_A134_EQU_type_att_format(type->operand_range));
        else
            t_attr = (symbol_attributes::type_attr)*t_value;
    }

    // length attribute operand
    symbol_attributes::len_attr l_attr = symbol_attributes::undef_length;
    if (length)
    {
        const auto l_value = try_get_abs_value(length, dep_solver_override);
        if (!l_value || l_value < 0 || l_value > 65535)
            add_diagnostic(diagnostic_op::error_A133_EQU_len_att_format(length->operand_range));
        else
            l_attr = (symbol_attributes::len_attr)*l_value;
    }

    // value operand
    if (!value)
    {
        add_diagnostic(diagnostic_op::error_A132_EQU_value_format(ops.front()->operand_range));
        return;
    }
    const auto expr_op = value->access_asm()->access_expr();
    if (!expr_op)
    {
        add_diagnostic(diagnostic_op::error_A132_EQU_value_format(value->operand_range));
        return;
    }

    if (l_attr == symbol_attributes::undef_length)
    {
        auto l_term = expr_op->expression->leftmost_term();
        if (auto symbol_term = dynamic_cast<const expressions::mach_expr_symbol*>(l_term))
        {
            auto len_symbol = hlasm_ctx.ord_ctx.get_symbol(symbol_term->value);

            if (len_symbol != nullptr && len_symbol->kind() != context::symbol_value_kind::UNDEF)
                l_attr = len_symbol->attributes().length();
            else
                l_attr = 1;
        }
        else
            l_attr = 1;
    }

    const auto s_attr = symbol_attributes::undef_scale;
    const auto i_attr = context::integer_type::undefined;

    const symbol_attributes attrs(context::symbol_origin::EQU, t_attr, l_attr, s_attr, i_attr, p_attr, a_attr);

    if (auto holder = expr_op->expression->get_dependencies(dep_solver); !holder.contains_dependencies())
        create_symbol(symbol_name, expr_op->expression->evaluate(dep_solver, diag_ctx), attrs);
    else if (holder.is_address() && holder.unresolved_spaces.empty())
        create_symbol(symbol_name, std::move(*holder.unresolved_address), attrs);
    else
    {
        const auto stmt_range = stmt.stmt_range_ref();
        create_symbol(symbol_name, context::symbol_value(), attrs);
        const auto adder = hlasm_ctx.ord_ctx.symbol_dependencies().add_dependencies(
            std::make_unique<postponed_statement_impl>(std::move(stmt), hlasm_ctx.processing_stack()),
            std::move(dep_solver).derive_current_dependency_evaluation_context(),
            lib_info);
        if (!adder.add_dependency(symbol_name, expr_op->expression.get()))
            add_diagnostic(diagnostic_op::error_E033(stmt_range));
    }
}

template<checking::data_instr_type instr_type>
void asm_processor::process_data_instruction(rebuilt_statement&& stmt)
{
    const auto& ops = stmt.operands_ref().value;
    if (ops.empty() || std::ranges::find(ops, semantics::operand_type::EMPTY, &semantics::operand::type) != ops.end())
    {
        context::ordinary_assembly_dependency_solver dep_solver(hlasm_ctx.ord_ctx, lib_info);
        hlasm_ctx.ord_ctx.symbol_dependencies().add_postponed_statement(
            std::make_unique<postponed_statement_impl>(std::move(stmt), hlasm_ctx.processing_stack()),
            std::move(dep_solver).derive_current_dependency_evaluation_context());
        return;
    }

    const semantics::operand* first_op = ops.front().get();

    // enforce alignment of the first operand
    context::alignment al = first_op->access_data_def()->value->get_alignment();
    context::address loctr = hlasm_ctx.ord_ctx.align(al);
    context::ordinary_assembly_dependency_solver dep_solver(hlasm_ctx.ord_ctx, loctr, lib_info);

    // process label
    auto label = find_label_symbol(stmt);

    const context::resolvable* l_dep = nullptr;
    const context::resolvable* s_dep = nullptr;

    if (!label.empty())
    {
        if (!hlasm_ctx.ord_ctx.symbol_defined(label))
        {
            auto data_op = first_op->access_data_def();

            context::symbol_attributes::type_attr type =
                ebcdic_encoding::to_ebcdic((unsigned char)data_op->value->get_type_attribute());

            context::symbol_attributes::program_type prog_type {};
            if (data_op->value->program_type
                && !data_op->value->program_type->get_dependencies(dep_solver).contains_dependencies())
            {
                prog_type = data_op->value->get_program_attribute(dep_solver, drop_diagnostic_op);
            }

            auto& symbol = hlasm_ctx.ord_ctx.create_symbol(label,
                std::move(loctr),
                context::symbol_attributes(context::symbol_origin::DAT,
                    type,
                    context::symbol_attributes::undef_length,
                    context::symbol_attributes::undef_scale,
                    data_op->value->get_integer_attribute(),
                    prog_type));

            if (!data_op->value->length
                || !data_op->value->length->get_dependencies(dep_solver).contains_dependencies())
                symbol.set_length(data_op->value->get_length_attribute(dep_solver, drop_diagnostic_op));
            else
                l_dep = data_op->value->length.get();

            if (const auto* data_type = data_op->value->access_data_def_type(); data_type && data_type->ignores_scale())
                symbol.set_scale(0);
            else if (!data_op->value->scale
                || !data_op->value->scale->get_dependencies(dep_solver).contains_dependencies())
                symbol.set_scale(data_op->value->get_scale_attribute(dep_solver, drop_diagnostic_op));
            else // TODO: HLASM does not seem to be tracking the attribute dependency correctly
                s_dep = data_op->value->scale.get();

            hlasm_ctx.ord_ctx.symbol_dependencies().add_defined(label, nullptr, lib_info);
        }
        else
            add_diagnostic(diagnostic_op::error_E031("symbol", stmt.label_ref().field_range));
    }

    // TODO issue warning when alignment is bigger than section's alignment
    // hlasm_ctx.ord_ctx.current_section()->current_location_counter().

    std::vector<data_def_dependency<instr_type>> dependencies;
    std::vector<context::space_ptr> dependencies_spaces;

    // Why is this so complicated?
    // 1. We cannot represent the individual operands because of bitfields.
    // 2. We cannot represent the whole area as a single dependency when the alignment requirements are growing.
    // Therefore, we split the operands into chunks depending on the alignment.
    // Whenever the alignment requirement increases between consecutive operands, we start a new chunk.
    for (auto it = ops.begin(); it != ops.end();)
    {
        const auto start = it;

        const auto initial_alignment = (*it)->access_data_def()->value->get_alignment();
        context::address op_loctr = hlasm_ctx.ord_ctx.align(initial_alignment);
        data_def_dependency_solver op_solver(dep_solver, &op_loctr);

        auto current_alignment = initial_alignment;

        // has_length_dependencies specifies whether the length of the data instruction can be resolved right now or
        // must be postponed
        bool has_length_dependencies = false;

        for (; it != ops.end(); ++it)
        {
            const auto& op = *it;

            auto data_op = op->access_data_def();
            auto op_align = data_op->value->get_alignment();

            // leave for the next round to make sure that the actual alignment is computed correctly
            if (op_align.boundary > current_alignment.boundary)
                break;
            current_alignment = op_align;

            has_length_dependencies |= data_op->get_length_dependencies(op_solver).contains_dependencies();

            if (const auto* pt = data_op->value->program_type.get())
            {
                if (pt->get_dependencies(dep_solver).contains_dependencies()
                    || pt->evaluate(dep_solver, drop_diagnostic_op).value_kind() != context::symbol_value_kind::ABS)
                    add_diagnostic(diagnostic_op::error_A175_data_prog_type_deps(pt->get_range()));
            }
        }

        const auto* const b = std::to_address(start);
        const auto* const e = std::to_address(it);

        if (has_length_dependencies)
        {
            dependencies.emplace_back(b, e, std::move(op_loctr));
            dependencies_spaces.emplace_back(hlasm_ctx.ord_ctx.register_ordinary_space(current_alignment));
        }
        else
        {
            auto length = data_def_dependency<instr_type>::get_operands_length(b, e, op_solver, drop_diagnostic_op);
            hlasm_ctx.ord_ctx.reserve_storage_area(length, context::no_align);
        }
    }

    auto dep_stmt = std::make_unique<data_def_postponed_statement<instr_type>>(
        std::move(stmt), hlasm_ctx.processing_stack(), std::move(dependencies));
    const auto& deps = dep_stmt->get_dependencies();

    const auto adder = hlasm_ctx.ord_ctx.symbol_dependencies().add_dependencies(
        std::move(dep_stmt), std::move(dep_solver).derive_current_dependency_evaluation_context(), lib_info);

    bool cycle_ok = true;

    if (l_dep)
        cycle_ok &= adder.add_dependency(label, context::data_attr_kind::L, l_dep);
    if (s_dep)
        cycle_ok &= adder.add_dependency(label, context::data_attr_kind::S, s_dep);

    if (!cycle_ok)
        add_diagnostic(diagnostic_op::error_E033(first_op->operand_range));

    for (auto sp = dependencies_spaces.begin(); const auto& d : deps)
        adder.add_dependency(std::move(*sp++), &d);
}

void asm_processor::process_DC(rebuilt_statement&& stmt)
{
    process_data_instruction<checking::data_instr_type::DC>(std::move(stmt));
}

void asm_processor::process_DS(rebuilt_statement&& stmt)
{
    process_data_instruction<checking::data_instr_type::DS>(std::move(stmt));
}

void asm_processor::process_COPY(rebuilt_statement&& stmt)
{
    find_sequence_symbol(stmt);

    if (stmt.operands_ref().value.size() == 1 && stmt.operands_ref().value.front()->type == semantics::operand_type::ASM
        && stmt.operands_ref().value.front()->access_asm()->access_expr())
    {
        if (auto extract = extract_copy_id(stmt, &diag_ctx); extract.has_value())
        {
            if (ctx.hlasm_ctx->copy_members().contains(extract->name))
                common_copy_postprocess(true, *extract, *ctx.hlasm_ctx, &diag_ctx);
            else
            {
                branch_provider.request_external_processing(
                    extract->name, processing_kind::COPY, [extract, this](bool result) {
                        common_copy_postprocess(result, *extract, *ctx.hlasm_ctx, &diag_ctx);
                    });
            }
        }
    }
    else
    {
        context::ordinary_assembly_dependency_solver dep_solver(hlasm_ctx.ord_ctx, lib_info);
        hlasm_ctx.ord_ctx.symbol_dependencies().add_postponed_statement(
            std::make_unique<postponed_statement_impl>(std::move(stmt), hlasm_ctx.processing_stack()),
            std::move(dep_solver).derive_current_dependency_evaluation_context());
    }
}

void asm_processor::process_DXD(rebuilt_statement&& stmt)
{
    if (const auto name = find_label_symbol(stmt); name.empty())
        add_diagnostic(diagnostic_op::error_E053(stmt.label_ref().field_range));
    else if (hlasm_ctx.ord_ctx.symbol_defined(name))
        add_diagnostic(diagnostic_op::error_E031("external symbol", stmt.label_ref().field_range));
    else
        hlasm_ctx.ord_ctx.create_external_section(name, context::section_kind::EXTERNAL_DSECT);

    // TODO: S' and I' attributes do not currently work with DXD in HLASM, revisit when fixed

    context::ordinary_assembly_dependency_solver dep_solver(hlasm_ctx.ord_ctx, lib_info);
    hlasm_ctx.ord_ctx.symbol_dependencies().add_postponed_statement(
        std::make_unique<postponed_statement_impl>(std::move(stmt), hlasm_ctx.processing_stack()),
        std::move(dep_solver).derive_current_dependency_evaluation_context());
}

void asm_processor::process_EXTRN(rebuilt_statement&& stmt)
{
    process_external(std::move(stmt), external_type::strong);
}

void asm_processor::process_WXTRN(rebuilt_statement&& stmt) { process_external(std::move(stmt), external_type::weak); }

void asm_processor::process_external(rebuilt_statement&& stmt, external_type t)
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
            hlasm_ctx.ord_ctx.create_external_section(name, s_kind, op_range.start);
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
            if (utils::to_upper_copy(complex->value.identifier) != "PART")
                continue;
            for (const auto& nested : complex->value.values)
            {
                if (const auto* string_val =
                        dynamic_cast<const semantics::complex_assembler_operand::string_value_t*>(nested.get());
                    string_val && !string_val->value.empty())
                    add_external(hlasm_ctx.add_id(string_val->value), string_val->op_range);
            }
        }
    }
    context::ordinary_assembly_dependency_solver dep_solver(hlasm_ctx.ord_ctx, lib_info);
    hlasm_ctx.ord_ctx.symbol_dependencies().add_postponed_statement(
        std::make_unique<postponed_statement_impl>(std::move(stmt), hlasm_ctx.processing_stack()),
        std::move(dep_solver).derive_current_dependency_evaluation_context());
}

void asm_processor::process_ORG(rebuilt_statement&& stmt)
{
    find_sequence_symbol(stmt);

    auto label = find_label_symbol(stmt);
    auto loctr = hlasm_ctx.ord_ctx.align(context::no_align);

    if (!label.empty())
    {
        if (hlasm_ctx.ord_ctx.symbol_defined(label))
            add_diagnostic(diagnostic_op::error_E031("symbol", stmt.label_ref().field_range));
        else
            create_symbol(label, loctr, context::symbol_attributes::make_org_attrs());
    }

    const auto& ops = stmt.operands_ref().value;

    if (ops.empty()
        || (ops.size() == 2 && ops[0]->type == semantics::operand_type::EMPTY
            && ops[1]->type == semantics::operand_type::EMPTY))
    {
        hlasm_ctx.ord_ctx.set_available_location_counter_value();
        return;
    }

    context::ordinary_assembly_dependency_solver dep_solver(hlasm_ctx.ord_ctx, loctr, lib_info);

    const semantics::expr_assembler_operand* reloc_expr = nullptr;
    size_t boundary = 0;
    int offset = 0;


    for (size_t i = 0; i < ops.size(); ++i)
    {
        if (ops[i]->type != semantics::operand_type::ASM)
            continue;

        auto asm_op = ops[i]->access_asm();
        assert(asm_op);
        auto expr = asm_op->access_expr();
        if (!expr)
        {
            if (i != 0)
                add_diagnostic(diagnostic_op::error_A115_ORG_op_format(stmt.stmt_range_ref()));
            break;
        }

        if (i == 0)
        {
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

    if (!reloc_expr)
    {
        add_diagnostic(diagnostic_op::error_A245_ORG_expression(stmt.stmt_range_ref()));
        return;
    }

    context::address reloc_val;
    auto deps = reloc_expr->expression->get_dependencies(dep_solver);
    bool undefined_absolute_part = !deps.undefined_symbolics.empty() || !deps.unresolved_spaces.empty();

    if (!undefined_absolute_part)
    {
        if (auto res = reloc_expr->expression->evaluate(dep_solver, drop_diagnostic_op);
            res.value_kind() == context::symbol_value_kind::RELOC)
            reloc_val = std::move(res).get_reloc();
        else
        {
            add_diagnostic(diagnostic_op::error_A245_ORG_expression(stmt.stmt_range_ref()));
            return;
        }
    }
    else
    {
        if (deps.unresolved_address)
            reloc_val = std::move(*deps.unresolved_address);
        else
            reloc_val = loctr;
    }

    switch (check_address_for_ORG(reloc_val, loctr, boundary, offset))
    {
        case check_org_result::valid:
            break;

        case check_org_result::underflow:
            add_diagnostic(diagnostic_op::error_E068(stmt.stmt_range_ref()));
            return;

        case check_org_result::invalid_address:
            add_diagnostic(diagnostic_op::error_A115_ORG_op_format(stmt.stmt_range_ref()));
            return;
    }

    if (undefined_absolute_part)
        hlasm_ctx.ord_ctx.set_location_counter_value(boundary,
            offset,
            *reloc_expr->expression,
            std::make_unique<postponed_statement_impl>(std::move(stmt), hlasm_ctx.processing_stack()),
            std::move(dep_solver).derive_current_dependency_evaluation_context());
    else
        hlasm_ctx.ord_ctx.set_location_counter_value(reloc_val, boundary, offset);

    if (boundary > 1 && offset == 0)
    {
        hlasm_ctx.ord_ctx.align(context::alignment { 0, boundary });
    }
}

void asm_processor::process_OPSYN(rebuilt_statement&& stmt)
{
    const auto& operands = stmt.operands_ref().value;

    auto label = find_label_symbol(stmt);
    if (label.empty())
    {
        if (stmt.label_ref().type == semantics::label_si_type::EMPTY)
            add_diagnostic(diagnostic_op::error_E053(stmt.label_ref().field_range));
        return;
    }

    context::id_index operand;
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

    if (operand.empty())
    {
        if (!hlasm_ctx.remove_mnemonic(label))
            add_diagnostic(diagnostic_op::error_E049(label.to_string_view(), stmt.label_ref().field_range));
    }
    else
    {
        if (!hlasm_ctx.add_mnemonic(label, operand))
            add_diagnostic(diagnostic_op::error_A246_OPSYN(operands.front()->operand_range));
    }

    context::ordinary_assembly_dependency_solver dep_solver(hlasm_ctx.ord_ctx, lib_info);
    hlasm_ctx.ord_ctx.symbol_dependencies().add_postponed_statement(
        std::make_unique<postponed_statement_impl>(std::move(stmt), hlasm_ctx.processing_stack()),
        std::move(dep_solver).derive_current_dependency_evaluation_context());
}

asm_processor::asm_processor(const analyzing_context& ctx,
    branching_provider& branch_provider,
    parse_lib_provider& lib_provider,
    statement_fields_parser& parser,
    opencode_provider& open_code,
    const processing_manager& proc_mgr,
    output_handler* output,
    diagnosable_ctx& diag_ctx)
    : low_language_processor(ctx, branch_provider, lib_provider, parser, proc_mgr, diag_ctx)
    , open_code_(&open_code)
    , output(output)
{}

void asm_processor::process(std::shared_ptr<const processing::resolved_statement> stmt)
{
    auto rebuilt_stmt = preprocess(std::move(stmt));

    register_literals(rebuilt_stmt, context::no_align, hlasm_ctx.ord_ctx.next_unique_id());

    if (const auto handler = handler_table::find(rebuilt_stmt.opcode_ref().value))
    {
        handler(this, std::move(rebuilt_stmt));
    }
    else
    {
        context::ordinary_assembly_dependency_solver dep_solver(hlasm_ctx.ord_ctx, lib_info);
        hlasm_ctx.ord_ctx.symbol_dependencies().add_postponed_statement(
            std::make_unique<postponed_statement_impl>(std::move(rebuilt_stmt), hlasm_ctx.processing_stack()),
            std::move(dep_solver).derive_current_dependency_evaluation_context());
    }
}

std::optional<asm_processor::extract_copy_id_result> asm_processor::extract_copy_id(
    const processing::resolved_statement& stmt, diagnosable_ctx* diagnoser)
{
    if (stmt.operands_ref().value.size() != 1 || !stmt.operands_ref().value.front()->access_asm()
        || !stmt.operands_ref().value.front()->access_asm()->access_expr())
    {
        if (diagnoser)
            diagnoser->add_diagnostic(diagnostic_op::error_E058(stmt.operands_ref().field_range));
        return {};
    }

    auto& expr = stmt.operands_ref().value.front()->access_asm()->access_expr()->expression;
    auto sym_expr = dynamic_cast<const expressions::mach_expr_symbol*>(expr.get());

    if (!sym_expr)
    {
        if (diagnoser)
            diagnoser->add_diagnostic(diagnostic_op::error_E058(stmt.operands_ref().value.front()->operand_range));
        return {};
    }

    return asm_processor::extract_copy_id_result {
        sym_expr->value,
        stmt.operands_ref().value.front()->operand_range,
        stmt.stmt_range_ref(),
    };
}

bool asm_processor::common_copy_postprocess(
    bool processed, const extract_copy_id_result& data, context::hlasm_context& hlasm_ctx, diagnosable_ctx* diagnoser)
{
    if (!processed)
    {
        if (diagnoser)
            diagnoser->add_diagnostic(diagnostic_op::error_E058(data.operand));
        return false;
    }

    if (auto whole_copy_stack = hlasm_ctx.whole_copy_stack();
        std::ranges::find(whole_copy_stack, data.name) != whole_copy_stack.end())
    {
        if (diagnoser)
            diagnoser->add_diagnostic(diagnostic_op::error_E062(data.statement));
        return false;
    }

    hlasm_ctx.enter_copy_member(data.name);

    return true;
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
            return context::id_index();
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

    context::id_index value;
};
} // namespace

void asm_processor::process_AINSERT(rebuilt_statement&& stmt)
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
    const auto& [value] = visitor;

    if (value.empty())
        return;
    processing::ainsert_destination dest;
    if (value.to_string_view() == "FRONT")
        dest = processing::ainsert_destination::front;
    else if (value.to_string_view() == "BACK")
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

void asm_processor::process_CCW(rebuilt_statement&& stmt)
{
    constexpr context::alignment ccw_align = context::doubleword;
    constexpr size_t ccw_length = 8U;

    auto loctr = hlasm_ctx.ord_ctx.align(ccw_align);
    find_sequence_symbol(stmt);

    if (auto label = find_label_symbol(stmt); !label.empty())
    {
        if (hlasm_ctx.ord_ctx.symbol_defined(label))
            add_diagnostic(diagnostic_op::error_E031("symbol", stmt.label_ref().field_range));
        else
            create_symbol(label, loctr, context::symbol_attributes::make_ccw_attrs());
    }

    context::ordinary_assembly_dependency_solver dep_solver(hlasm_ctx.ord_ctx, std::move(loctr), lib_info);

    hlasm_ctx.ord_ctx.reserve_storage_area(ccw_length, ccw_align);

    hlasm_ctx.ord_ctx.symbol_dependencies().add_postponed_statement(
        std::make_unique<postponed_statement_impl>(std::move(stmt), hlasm_ctx.processing_stack()),
        std::move(dep_solver).derive_current_dependency_evaluation_context());
}

void asm_processor::process_CNOP(rebuilt_statement&& stmt)
{
    auto loctr = hlasm_ctx.ord_ctx.align(context::halfword);
    find_sequence_symbol(stmt);

    if (auto label = find_label_symbol(stmt); !label.empty())
    {
        if (hlasm_ctx.ord_ctx.symbol_defined(label))
            add_diagnostic(diagnostic_op::error_E031("symbol", stmt.label_ref().field_range));
        else
            create_symbol(label, loctr, context::symbol_attributes::make_cnop_attrs());
    }

    context::ordinary_assembly_dependency_solver dep_solver(hlasm_ctx.ord_ctx, std::move(loctr), lib_info);

    if (stmt.operands_ref().value.size() == 2)
    {
        std::optional<int> byte_value = try_get_abs_value(stmt.operands_ref().value[0].get(), dep_solver);
        std::optional<int> boundary_value = try_get_abs_value(stmt.operands_ref().value[1].get(), dep_solver);
        // For now, the implementation ignores the instruction, if the operands have dependencies. Most uses of this
        // instruction should by covered anyway. It will still generate the label correctly.
        if (byte_value.has_value() && boundary_value.has_value() && *byte_value >= 0 && *boundary_value > 0
            && ((*boundary_value) & (*boundary_value - 1)) == 0 && *byte_value < *boundary_value
            && *byte_value % 2 == 0)
            hlasm_ctx.ord_ctx.reserve_storage_area(
                0, context::alignment { (size_t)*byte_value, (size_t)*boundary_value });
    }

    hlasm_ctx.ord_ctx.symbol_dependencies().add_postponed_statement(
        std::make_unique<postponed_statement_impl>(std::move(stmt), hlasm_ctx.processing_stack()),
        std::move(dep_solver).derive_current_dependency_evaluation_context());
}


void asm_processor::process_START(rebuilt_statement&& stmt)
{
    using enum context::section_kind;
    auto sect_name = find_label_symbol(stmt);

    if (std::ranges::any_of(
            hlasm_ctx.ord_ctx.sections(), [](const auto& s) { return s->kind == EXECUTABLE || s->kind == READONLY; }))
    {
        add_diagnostic(diagnostic_op::error_E073(stmt.stmt_range_ref()));
        return;
    }

    if (hlasm_ctx.ord_ctx.symbol_defined(sect_name))
    {
        add_diagnostic(diagnostic_op::error_E031("symbol", stmt.label_ref().field_range));
        return;
    }

    const auto* section = hlasm_ctx.ord_ctx.set_section(sect_name, EXECUTABLE, lib_info);

    const auto& ops = stmt.operands_ref().value;
    if (ops.size() != 1)
    {
        context::ordinary_assembly_dependency_solver dep_solver(hlasm_ctx.ord_ctx, lib_info);
        hlasm_ctx.ord_ctx.symbol_dependencies().add_postponed_statement(
            std::make_unique<postponed_statement_impl>(std::move(stmt), hlasm_ctx.processing_stack()),
            std::move(dep_solver).derive_current_dependency_evaluation_context());
        return;
    }

    context::ordinary_assembly_dependency_solver dep_solver(hlasm_ctx.ord_ctx, lib_info);
    auto initial_offset = try_get_abs_value(ops.front().get(), dep_solver);
    if (!initial_offset.has_value())
    {
        add_diagnostic(diagnostic_op::error_A250_absolute_with_known_symbols(ops.front()->operand_range));
        return;
    }

    size_t start_section_alignment = hlasm_ctx.section_alignment().boundary;
    size_t start_section_alignment_mask = start_section_alignment - 1;

    uint32_t offset = initial_offset.value();
    if (offset & start_section_alignment_mask)
    {
        // TODO: generate informational message?
        offset += start_section_alignment_mask;
        offset &= ~start_section_alignment_mask;
    }

    section->current_location_counter().reserve_storage_area(offset, context::no_align);
}
void asm_processor::process_END(rebuilt_statement&& stmt)
{
    const auto& label = stmt.label_ref();
    context::ordinary_assembly_dependency_solver dep_solver(hlasm_ctx.ord_ctx, lib_info);

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
                dep_solver, drop_diagnostic_op);

            if (symbol.value_kind() == context::symbol_value_kind::ABS)
            {
                add_diagnostic(
                    diagnostic_op::error_E032(std::to_string(symbol.get_abs()), stmt.operands_ref().field_range));
            }
        }
    }

    hlasm_ctx.ord_ctx.symbol_dependencies().add_postponed_statement(
        std::make_unique<postponed_statement_impl>(std::move(stmt), hlasm_ctx.processing_stack()),
        std::move(dep_solver).derive_current_dependency_evaluation_context());

    hlasm_ctx.end_reached();
}
void asm_processor::process_ALIAS(rebuilt_statement&& stmt)
{
    auto symbol_name = find_label_symbol(stmt);
    if (symbol_name.empty())
    {
        add_diagnostic(diagnostic_op::error_A163_ALIAS_mandatory_label(stmt.stmt_range_ref()));
        return;
    }

    context::ordinary_assembly_dependency_solver dep_solver(hlasm_ctx.ord_ctx, lib_info);
    hlasm_ctx.ord_ctx.symbol_dependencies().add_postponed_statement(
        std::make_unique<postponed_statement_impl>(std::move(stmt), hlasm_ctx.processing_stack()),
        std::move(dep_solver).derive_current_dependency_evaluation_context());
}
void asm_processor::process_LTORG(rebuilt_statement&& stmt)
{
    constexpr size_t sectalgn = 8;
    auto loctr = hlasm_ctx.ord_ctx.align(context::alignment { 0, sectalgn });

    find_sequence_symbol(stmt);


    if (auto label = find_label_symbol(stmt); !label.empty())
    {
        if (hlasm_ctx.ord_ctx.symbol_defined(label))
            add_diagnostic(diagnostic_op::error_E031("symbol", stmt.label_ref().field_range));
        else
            create_symbol(
                label, std::move(loctr), context::symbol_attributes(context::symbol_origin::EQU, 'U'_ebcdic, 1));
    }

    hlasm_ctx.ord_ctx.generate_pool(diag_ctx, hlasm_ctx.using_current(), lib_info);

    context::ordinary_assembly_dependency_solver dep_solver(hlasm_ctx.ord_ctx, lib_info);
    hlasm_ctx.ord_ctx.symbol_dependencies().add_postponed_statement(
        std::make_unique<postponed_statement_impl>(std::move(stmt), hlasm_ctx.processing_stack()),
        std::move(dep_solver).derive_current_dependency_evaluation_context());
}

void asm_processor::process_USING(rebuilt_statement&& stmt)
{
    using namespace expressions;

    auto loctr = hlasm_ctx.ord_ctx.align(context::no_align);
    context::ordinary_assembly_dependency_solver dep_solver(hlasm_ctx.ord_ctx, std::move(loctr), lib_info);

    auto label = find_using_label(stmt);

    if (!label.empty())
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
        std::move(dep_solver).derive_current_dependency_evaluation_context(),
        hlasm_ctx.processing_stack());
}

void asm_processor::process_DROP(rebuilt_statement&& stmt)
{
    using namespace expressions;

    auto loctr = hlasm_ctx.ord_ctx.align(context::no_align);

    if (auto label = find_label_symbol(stmt); !label.empty())
    {
        if (hlasm_ctx.ord_ctx.symbol_defined(label))
        {
            add_diagnostic(diagnostic_op::error_E031("symbol", stmt.label_ref().field_range));
        }
        else
        {
            add_diagnostic(diagnostic_op::warn_A251_unexpected_label(stmt.label_ref().field_range));
            create_symbol(label, loctr, context::symbol_attributes(context::symbol_origin::EQU));
        }
    }

    context::ordinary_assembly_dependency_solver dep_solver(hlasm_ctx.ord_ctx, std::move(loctr), lib_info);

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

    hlasm_ctx.using_remove(std::move(bases),
        std::move(dep_solver).derive_current_dependency_evaluation_context(),
        hlasm_ctx.processing_stack());
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

void asm_processor::process_PUSH(rebuilt_statement&& stmt)
{
    if (std::ranges::any_of(stmt.operands_ref().value, [](const auto& op) { return asm_expr_quals(op, "USING"); }))
        hlasm_ctx.using_push();

    context::ordinary_assembly_dependency_solver dep_solver(hlasm_ctx.ord_ctx, lib_info);
    hlasm_ctx.ord_ctx.symbol_dependencies().add_postponed_statement(
        std::make_unique<postponed_statement_impl>(std::move(stmt), hlasm_ctx.processing_stack()),
        std::move(dep_solver).derive_current_dependency_evaluation_context());
}

void asm_processor::process_POP(rebuilt_statement&& stmt)
{
    if (std::ranges::any_of(stmt.operands_ref().value, [](const auto& op) { return asm_expr_quals(op, "USING"); })
        && !hlasm_ctx.using_pop())
        add_diagnostic(diagnostic_op::error_A165_POP_USING(stmt.stmt_range_ref()));

    context::ordinary_assembly_dependency_solver dep_solver(hlasm_ctx.ord_ctx, lib_info);
    hlasm_ctx.ord_ctx.symbol_dependencies().add_postponed_statement(
        std::make_unique<postponed_statement_impl>(std::move(stmt), hlasm_ctx.processing_stack()),
        std::move(dep_solver).derive_current_dependency_evaluation_context());
}

void asm_processor::process_MNOTE(rebuilt_statement&& stmt)
{
    static constexpr std::string_view MNOTE = "MNOTE";
    const auto& ops = stmt.operands_ref().value;

    std::optional<int> level;
    size_t first_op_len = 0;

    find_sequence_symbol(stmt);

    switch (ops.size())
    {
        case 1:
            level = 0;
            break;
        case 2:
            switch (ops[0]->type)
            {
                case semantics::operand_type::EMPTY:
                    level = 1;
                    break;
                case semantics::operand_type::ASM:
                    if (auto expr = ops[0]->access_asm()->access_expr(); !expr)
                    {
                        // fail
                    }
                    else if (dynamic_cast<const expressions::mach_expr_location_counter*>(expr->expression.get()))
                    {
                        level = 0;
                        first_op_len = 1;
                    }
                    else
                    {
                        const auto& val = expr->get_value();
                        first_op_len = val.size();
                        level = try_get_number(val);
                    }
                    break;

                default:
                    break;
            }
            break;
        default:
            add_diagnostic(diagnostic_op::error_A012_from_to(MNOTE, 1, 2, stmt.operands_ref().field_range));
            return;
    }
    if (!level.has_value() || level.value() < 0 || level.value() > 255)
    {
        add_diagnostic(diagnostic_op::error_A119_MNOTE_first_op_format(ops[0]->operand_range));
        return;
    }

    std::string_view text;

    const auto& r = ops.back()->operand_range;
    if (ops.back()->type != semantics::operand_type::ASM)
    {
        add_diagnostic(diagnostic_op::warning_A300_op_apostrophes_missing(MNOTE, r));
    }
    else
    {
        auto* string_op = ops.back()->access_asm();
        if (string_op->kind == semantics::asm_kind::STRING)
        {
            text = string_op->access_string()->value;
        }
        else
        {
            if (string_op->kind == semantics::asm_kind::EXPR)
            {
                text = string_op->access_expr()->get_value();
            }
            add_diagnostic(diagnostic_op::warning_A300_op_apostrophes_missing(MNOTE, r));
        }
    }

    if (text.size() > checking::MNOTE_max_message_length)
    {
        add_diagnostic(diagnostic_op::error_A117_MNOTE_message_size(r));
        text = text.substr(0, checking::MNOTE_max_message_length);
    }
    else if (text.size() + first_op_len > checking::MNOTE_max_operands_length)
    {
        add_diagnostic(diagnostic_op::error_A118_MNOTE_operands_size(r));
    }

    std::string sanitized;
    sanitized.reserve(text.size());
    utils::append_utf8_sanitized(sanitized, text);

    if (output)
        output->mnote(level.value(), sanitized);

    add_diagnostic(diagnostic_op::mnote_diagnostic(level.value(), sanitized, r));

    hlasm_ctx.update_mnote_max((unsigned)level.value());
}

void asm_processor::process_CXD(rebuilt_statement&& stmt)
{
    context::address loctr = hlasm_ctx.ord_ctx.align(context::fullword);
    constexpr uint32_t cxd_length = 4;

    // process label
    if (auto label = find_label_symbol(stmt); !label.empty())
    {
        if (!hlasm_ctx.ord_ctx.symbol_defined(label))
        {
            create_symbol(label,
                std::move(loctr),
                context::symbol_attributes(context::symbol_origin::ASM, 'A'_ebcdic, cxd_length));
        }
        else
            add_diagnostic(diagnostic_op::error_E031("symbol", stmt.label_ref().field_range));
    }

    hlasm_ctx.ord_ctx.reserve_storage_area(cxd_length, context::no_align);
}

struct title_label_visitor
{
    std::string operator()(const std::string& v) const { return v; }
    std::string operator()(const semantics::ord_symbol_string& v) const { return v.mixed_case; }
    std::string operator()(const semantics::concat_chain&) const { return {}; }
    std::string operator()(const semantics::seq_sym&) const { return {}; }
    std::string operator()(const semantics::vs_ptr&) const { return {}; }
};

void asm_processor::process_TITLE(rebuilt_statement&& stmt)
{
    const auto& label = stmt.label_ref();

    if (auto label_text = std::visit(title_label_visitor(), label.value); !label_text.empty())
    {
        if (hlasm_ctx.get_title_name().empty())
            hlasm_ctx.set_title_name(std::move(label_text));
        else
            add_diagnostic(diagnostic_op::warning_W016(label.field_range));
    }

    hlasm_ctx.ord_ctx.symbol_dependencies().add_postponed_statement(
        std::make_unique<postponed_statement_impl>(std::move(stmt), hlasm_ctx.processing_stack()),
        context::ordinary_assembly_dependency_solver(hlasm_ctx.ord_ctx, lib_info)
            .derive_current_dependency_evaluation_context());
}

void asm_processor::process_PUNCH(rebuilt_statement&& stmt)
{
    static constexpr std::string_view PUNCH = "PUNCH";
    find_sequence_symbol(stmt);

    const auto& ops = stmt.operands_ref().value;
    if (ops.size() != 1)
    {
        add_diagnostic(diagnostic_op::error_A011_exact(PUNCH, 1, stmt.operands_ref().field_range));
        return;
    }

    std::string_view text;

    const auto& r = ops.front()->operand_range;
    if (auto* asm_op = ops.front()->access_asm(); asm_op && asm_op->kind == semantics::asm_kind::STRING)
        text = asm_op->access_string()->value;
    else
    {
        add_diagnostic(diagnostic_op::warning_A300_op_apostrophes_missing(PUNCH, r));
        return;
    }

    if (text.empty())
    {
        add_diagnostic(diagnostic_op::warning_A302_punch_empty(r));
        return;
    }

    if (utils::length_utf32_no_validation(text) > checking::string_max_length)
    {
        add_diagnostic(diagnostic_op::error_A108_PUNCH_too_long(r));
        return;
    }

    if (!output)
        return;

    std::string sanitized;
    sanitized.reserve(text.size());
    utils::append_utf8_sanitized(sanitized, text);

    output->punch(sanitized);
}

asm_processor::cattr_ops_result asm_processor::cattr_ops(const semantics::operands_si& ops)
{
    using enum semantics::operand_type;
    if (ops.value.empty())
        return {};
    if (ops.value.size() == 2 && ops.value.front()->type == EMPTY && ops.value.back()->type == EMPTY)
        return {};

    for (const auto& op : ops.value)
    {
        const auto* asm_op = op->access_asm();
        if (!asm_op)
            continue;
        const auto* complex = asm_op->access_complex();
        if (!complex)
            continue;
        if (complex->value.identifier != "PART" || complex->value.values.size() != 1)
            continue;
        const auto* str = dynamic_cast<const semantics::complex_assembler_operand::string_value_t*>(
            complex->value.values.front().get());
        if (!str)
            continue;
        auto [_, result] = hlasm_ctx.try_get_symbol_name(str->value);

        return { ops.value.size() - 1, result, complex->operand_range };
    }

    return { ops.value.size(), {}, {} };
}

namespace {
constexpr bool can_switch_into_section(context::section_kind s) noexcept
{
    using enum context::section_kind;
    switch (s)
    {
        case DUMMY:
        case COMMON:
        case EXECUTABLE:
        case READONLY:
            return true;
        case EXTERNAL:
        case WEAK_EXTERNAL:
        case EXTERNAL_DSECT:
            return false;
    }
}
} // namespace

void asm_processor::handle_cattr_ops(context::id_index class_name,
    context::id_index part_name,
    const range& part_rng,
    size_t op_count,
    const rebuilt_statement& stmt)
{
    assert(!class_name.empty());
    auto* class_name_sect = hlasm_ctx.ord_ctx.get_section(class_name);

    if (auto* part_name_sect = part_name.empty() ? nullptr : hlasm_ctx.ord_ctx.get_section(part_name))
    {
        if (!part_name_sect->goff || part_name_sect->goff->parent != class_name_sect)
            add_diagnostic(diagnostic_op::error_A170_section_type_mismatch(part_rng));
        else if (op_count != 1)
            add_diagnostic(diagnostic_op::warn_A171_operands_ignored(stmt.operands_ref().field_range));
        if (can_switch_into_section(part_name_sect->kind))
            hlasm_ctx.ord_ctx.set_section(*part_name_sect);
        return;
    }

    if (!part_name.empty() && hlasm_ctx.ord_ctx.symbol_defined(part_name))
    {
        add_diagnostic(diagnostic_op::error_E031("symbol", part_rng));
        return;
    }

    if (class_name_sect)
    {
        if (can_switch_into_section(class_name_sect->kind))
            hlasm_ctx.ord_ctx.set_section(*class_name_sect);

        if (!class_name_sect->goff || part_name.empty() && class_name_sect->goff->partitioned)
        {
            add_diagnostic(diagnostic_op::error_A170_section_type_mismatch(stmt.label_ref().field_range));
            return;
        }
        else if (!part_name.empty() && !class_name_sect->goff->partitioned || op_count > !part_name.empty())
        {
            add_diagnostic(diagnostic_op::warn_A171_operands_ignored(stmt.operands_ref().field_range));
            return;
        }
    }
    else if (hlasm_ctx.ord_ctx.symbol_defined(class_name))
    {
        add_diagnostic(diagnostic_op::error_E031("symbol", stmt.label_ref().field_range));
        return;
    }
    else
    {
        class_name_sect = hlasm_ctx.ord_ctx.create_and_set_class(class_name, lib_info, nullptr, !part_name.empty());

        // TODO: sectalign? part
    }

    if (part_name.empty())
        return;

    hlasm_ctx.ord_ctx.create_and_set_class(part_name, lib_info, class_name_sect, false);
}

void asm_processor::process_CATTR(rebuilt_statement&& stmt)
{
    static constexpr size_t max_class_name_length = 16;
    context::id_index class_name = find_label_symbol(stmt);
    auto [op_count, part, part_rng] = cattr_ops(stmt.operands_ref());

    if (class_name.empty() || class_name.to_string_view().size() > max_class_name_length)
        add_diagnostic(diagnostic_op::error_A167_CATTR_label(stmt.label_ref().field_range));
    else if (!hlasm_ctx.goff())
        ; // NOP
    else if (!hlasm_ctx.ord_ctx.get_last_active_control_section())
        add_diagnostic(diagnostic_op::error_A169_no_section(stmt.stmt_range_ref()));
    else
        handle_cattr_ops(class_name, part, part_rng, op_count, stmt);

    context::ordinary_assembly_dependency_solver dep_solver(hlasm_ctx.ord_ctx, lib_info);
    hlasm_ctx.ord_ctx.symbol_dependencies().add_postponed_statement(
        std::make_unique<postponed_statement_impl>(std::move(stmt), hlasm_ctx.processing_stack()),
        std::move(dep_solver).derive_current_dependency_evaluation_context());
}

void asm_processor::process_XATTR(rebuilt_statement&& stmt)
{
    context::id_index class_name = find_label_symbol(stmt);

    if (!hlasm_ctx.goff())
        add_diagnostic(diagnostic_op::error_A166_GOFF_required(stmt.instruction_ref().field_range));
    else if (class_name.empty())
        add_diagnostic(diagnostic_op::error_A168_XATTR_label(stmt.label_ref().field_range));
    else
    {
        for (const auto& op : stmt.operands_ref().value)
        {
            const auto* asm_op = op->access_asm();
            if (!asm_op)
                continue;
            const auto* complex = asm_op->access_complex();
            if (!complex)
                continue;
            if (complex->value.identifier != "PSECT" || complex->value.values.size() != 1)
                continue;
            const auto* str = dynamic_cast<const semantics::complex_assembler_operand::string_value_t*>(
                complex->value.values.front().get());
            if (!str)
                continue;

            auto [valid, psect] = hlasm_ctx.try_get_symbol_name(str->value);
            if (!valid)
                continue;

            if (!hlasm_ctx.register_psect(class_name, psect))
                add_diagnostic(diagnostic_op::warn_A172_psect_redefinition(complex->operand_range));

            break;
        }
    }

    context::ordinary_assembly_dependency_solver dep_solver(hlasm_ctx.ord_ctx, lib_info);
    hlasm_ctx.ord_ctx.symbol_dependencies().add_postponed_statement(
        std::make_unique<postponed_statement_impl>(std::move(stmt), hlasm_ctx.processing_stack()),
        std::move(dep_solver).derive_current_dependency_evaluation_context());
}

} // namespace hlasm_plugin::parser_library::processing
