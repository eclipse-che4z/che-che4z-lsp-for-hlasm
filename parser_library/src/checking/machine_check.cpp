/*
 * Copyright (c) 2025 Broadcom.
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

#include "machine_check.h"

#include "checking/diagnostic_collector.h"
#include "checking/using_label_checker.h"
#include "context/ordinary_assembly/symbol_value.h"
#include "context/using.h"
#include "diagnostic_consumer.h"
#include "instr_operand.h"
#include "instructions/instruction.h"
#include "range.h"
#include "semantics/operand_impls.h"

namespace hlasm_plugin::parser_library::checking {

namespace {
constexpr bool check_value_parity(std::int32_t value, instructions::even_odd_register reg)
{
    using enum instructions::even_odd_register;
    switch (reg)
    {
        case NONE:
            return true;
        case ODD:
            return !!(value & 1);
        case EVEN:
            return !(value & 1);
    }
}

constexpr std::pair<long long, long long> compute_boundaries(instructions::parameter p)
{
    using enum instructions::machine_operand_type;

    const auto boundary = 1LL << (p.size - p.is_signed);

    const auto low = p.is_signed ? -boundary : (long long)p.min_register;
    const auto high = boundary - (p.type != LENGTH); // LENGTH operands are encoded as L-1 in the instructions;
                                                     // also 0 is tolerated, overall we get full 0-2^n range
    return { low, high };
}

struct op_bound_results
{
    long long low;
    long long high;
    bool passed;
};

constexpr op_bound_results check_op_value(std::int32_t value, instructions::parameter p)
{
    const auto [low, high] = compute_boundaries(p);
    const bool passed_parity = check_value_parity(value, p.evenodd);
    return { low, high, low <= value && value <= high && passed_parity };
}

std::pair<std::optional<context::symbol_value::abs_value_t>, bool> evaluate_abs_expression(
    const expressions::mach_expr_ptr& expr, context::dependency_solver& info, diagnostic_op_consumer& diags)
{
    if (!expr)
        return { std::nullopt, false };

    if (auto value = expr->evaluate(info, diags); value.value_kind() == context::symbol_value_kind::ABS)
        return { value.get_abs(), false };

    diags.add_diagnostic(diagnostic_op::error_ME010(expr->get_range()));
    return { std::nullopt, true };
}

diagnostic_op get_simple_operand_expected(
    const instructions::machine_operand_format& op_format, std::string_view instr_name, const auto& operand_range)
{
    using enum instructions::machine_operand_type;
    switch (op_format.identifier.type)
    {
        case REG: // R
            return diagnostic_op::error_M110(instr_name, operand_range);
        case MASK: // M
            return diagnostic_op::error_M111(instr_name, operand_range);
        case IMM: // I
            return diagnostic_op::error_M112(instr_name, operand_range);
        case VEC_REG: // V
            return diagnostic_op::error_M114(instr_name, operand_range);
        case RELOC_IMM: // RI
            return diagnostic_op::error_M113(instr_name, operand_range);
    }
    assert(false);
    return diagnostic_op::error_I999(instr_name, operand_range);
}

} // namespace

bool is_simple_operand(const instructions::machine_operand_format& operand)
{
    using enum instructions::machine_operand_type;
    return (operand.first.is_signed == false && operand.first.size == 0 && operand.second.is_signed == false
        && operand.second.size == 0 && operand.first.type == NONE && operand.second.type == NONE);
}

constexpr bool is_dipl_like(instructions::machine_operand_type type)
{
    using enum instructions::machine_operand_type;
    return type == DISP || type == DISP_IDX;
}

constexpr bool is_long_disp(instructions::machine_operand_format f)
{
    return f.identifier == instructions::dis_20s || f.identifier == instructions::dis_idx_20s;
}

bool check_op_count(std::pair<size_t, size_t> op_count,
    std::string_view name,
    std::span<const std::unique_ptr<semantics::operand>> ops,
    const range& stmt_range,
    const diagnostic_collector& add_diagnostic)
{
    const auto [low, high] = op_count;
    if (const auto s = ops.size(); s < low || s > high)
    {
        add_diagnostic(diagnostic_op::error_optional_number_of_operands(name, high - low, high, stmt_range));
        return false;
    }
    assert(ops.size() <= instructions::machine_instruction::max_operand_count);
    return true;
}

bool check_dependencies(std::span<const std::unique_ptr<semantics::operand>> ops,
    context::dependency_solver& dep_solver,
    diagnostic_collector& add_diagnostic)
{
    diagnostic_consumer_transform diags([&add_diagnostic](diagnostic_op d) { add_diagnostic(std::move(d)); });
    checking::using_label_checker lc(dep_solver, diags);
    std::vector<context::id_index> missing_symbols;

    for (const auto& op_p : ops)
    {
        const auto* op = op_p->access_mach();
        assert(op);
        if (op->has_dependencies(dep_solver, &missing_symbols))
        {
            for (const auto& sym : missing_symbols)
                add_diagnostic(diagnostic_op::error_E010("ordinary symbol", sym.to_string_view(), op->operand_range));
            if (missing_symbols.empty()) // this is a fallback message if somehow non-symbolic deps are not resolved
                add_diagnostic(diagnostic_op::error_E016(op->operand_range));
            return false;
        }

        op->apply_mach_visitor(lc);
    }

    return true;
}

machine_operand* evaluate_operands(machine_operand* out,
    const instructions::machine_instruction& mi,
    std::string_view mi_name,
    std::span<const std::unique_ptr<semantics::operand>> ops,
    std::span<const instructions::machine_operand_format> formats,
    std::span<const instructions::mnemonic_transformation> transforms,
    context::dependency_solver& solver,
    diagnostic_collector& add_diagnostic)
{
    diagnostic_consumer_transform diags([&add_diagnostic](diagnostic_op d) { add_diagnostic(std::move(d)); });

    const auto* fmt = formats.data();
    unsigned char op_id = 0;
    for (size_t processed = 0; const auto& op : ops)
    {
        while (!transforms.empty() && transforms.front().skip == processed)
        {
            if (transforms.front().insert)
                ++fmt;
            transforms = transforms.subspan(1);
            processed = 0;
        }

        const auto* mop = op->access_mach();
        assert(mop);

        if (!mop->displacement)
        {
            add_diagnostic(diagnostic_op::error_M003(mi_name, mop->operand_range));
            return nullptr;
        }

        if ((mop->first_par || mop->second_par) && is_simple_operand(*fmt))
        {
            add_diagnostic(get_simple_operand_expected(*fmt, mi_name, mop->operand_range));
            return nullptr;
        }

        if (mop->second_par && fmt->first.is_empty()) // invalid D(,B)
        {
            add_diagnostic(diagnostic_op::error_M104(mi_name, mop->operand_range));
            return nullptr;
        }

        const auto d = mop->displacement->evaluate(solver, diags);

        if (d.value_kind() == context::symbol_value_kind::UNDEF)
            return nullptr;

        auto [first_v, first_err] = evaluate_abs_expression(mop->first_par, solver, diags);
        if (first_err)
            return nullptr;

        auto [second_v, second_err] = evaluate_abs_expression(mop->second_par, solver, diags);
        if (second_err)
            return nullptr;

        if (fmt->first.is_empty()) // transform "D(F)" into "D(B)"
            std::swap(first_v, second_v);

        bool first_op_derived = false;
        if (!first_v.has_value() && fmt->first.type == instructions::machine_operand_type::LENGTH)
        {
            first_op_derived = true;
            first_v = mop->displacement->derive_length(mi.size_in_bits() / 8, solver);
        }

        if (d.value_kind() == context::symbol_value_kind::ABS)
        {
            *out = machine_operand {
                .displacement = d.get_abs(),
                .first_op = first_v.value_or(0),
                .second_op = second_v.value_or(0),
                .valid = true,
                .first_op_derived = first_op_derived,
                .source = op_id,
            };
        }
        else if (is_dipl_like(fmt->identifier.type))
        {
            if (second_v.has_value())
            {
                add_diagnostic(diagnostic_op::error_ME011(mop->operand_range));
                return nullptr;
            }
            const auto& reloc = d.get_reloc();
            if (!reloc.is_simple())
            {
                add_diagnostic(diagnostic_op::error_ME009(mop->operand_range));
                return nullptr;
            }
            const auto& base = reloc.bases().front();
            const bool long_displacement = is_long_disp(*fmt);
            auto translated_addr = solver.using_evaluate(base.qualifier, base.owner, reloc.offset(), long_displacement);
            if (translated_addr.reg == context::using_collection::invalid_register)
            {
                if (translated_addr.reg_offset)
                    diags.add_diagnostic(diagnostic_op::error_ME008(translated_addr.reg_offset, mop->operand_range));
                else
                    diags.add_diagnostic(diagnostic_op::error_ME007(mop->operand_range));
                return nullptr;
            }
            *out = machine_operand {
                .displacement = translated_addr.reg_offset,
                .first_op = first_v.value_or(0),
                .second_op = translated_addr.reg,
                .valid = true,
                .first_op_derived = first_op_derived,
                .source = op_id,
            };
        }
        else
        {
            add_diagnostic(get_simple_operand_expected(*fmt, mi_name, mop->operand_range));
            return nullptr;
        }

        ++fmt;
        ++out;
        ++op_id;
        ++processed;
    }

    return out;
}

machine_operand* apply_transforms(machine_operand* out,
    std::span<const machine_operand> ops,
    size_t max_op_count,
    std::span<const instructions::mnemonic_transformation> transforms)
{
    // add other
    for (size_t processed = 0, ops_cur = 0; ops_cur < max_op_count;)
    {
        if (!transforms.empty() && transforms.front().skip == processed)
        {
            const auto transform = transforms.front();
            transforms = transforms.subspan(1);
            processed = 0;

            static constexpr machine_operand def {};

            const auto& op = transform.source < ops.size() ? ops[transform.source] : def;
            int arg = transform.value;
            using enum instructions::mnemonic_transformation_kind;
            switch (transform.type)
            {
                case value:
                    break;
                case copy:
                    arg = op.displacement;
                    break;
                case or_with:
                    arg |= op.displacement;
                    break;
                case add_to:
                    arg += op.displacement;
                    break;
                case subtract_from:
                    arg -= op.displacement;
                    break;
                case complement:
                    arg = 1 + ~(unsigned)op.displacement & (1u << arg) - 1;
                    break;
            }
            *out++ = {
                .displacement = arg,
                .first_op = 0,
                .second_op = 0,
                .valid = op.valid || !transform.has_source() && transform.insert,
                .first_op_derived = false,
                .source = transform.source,
            };
            if (!transform.insert && ops_cur < ops.size())
                ++ops_cur;
            continue;
        }
        if (ops_cur >= ops.size())
            break;
        *out++ = ops[ops_cur];
        ++ops_cur;
        ++processed;
    }

    return out;
}

void check_operands(std::span<const machine_operand> ops,
    std::span<const instructions::machine_operand_format> formats,
    std::string_view mi_name,
    std::span<const std::unique_ptr<semantics::operand>> orig_ops,
    diagnostic_collector& add_diagnostics)
{
    assert(ops.size() <= formats.size());
    const auto get_range = [&orig_ops](unsigned char source) { return orig_ops[source]->operand_range; };
    static constexpr std::string_view reg_qual[] = { "", "odd", "even" };

    for (const auto* fmt = formats.data(); const auto& op : ops)
    {
        using enum instructions::machine_operand_type;
        if (const auto [low, high, passed] = check_op_value(op.displacement, fmt->identifier); !passed)
        {
            switch (fmt->identifier.type)
            {
                case MASK:
                    add_diagnostics(diagnostic_op::error_M121(mi_name, low, high, get_range(op.source)));
                    break;
                case REG:
                    add_diagnostics(diagnostic_op::error_M120(
                        mi_name, get_range(op.source), low, high, reg_qual[(int)fmt->identifier.evenodd]));
                    break;
                case IMM:
                    add_diagnostics(diagnostic_op::warn_M137(mi_name, low, high, get_range(op.source)));
                    break;
                case DISP:
                    add_diagnostics(diagnostic_op::error_M130(mi_name, low, high, get_range(op.source)));
                    break;
                case DISP_IDX:
                    add_diagnostics(diagnostic_op::error_M130(mi_name, low, high, get_range(op.source)));
                    break;
                case VEC_REG:
                    add_diagnostics(diagnostic_op::error_M124(mi_name, low, high, get_range(op.source)));
                    break;
                case IDX_REG:
                    add_diagnostics(diagnostic_op::error_M135(mi_name, low, high, get_range(op.source)));
                    break;
                case RELOC_IMM:
                    add_diagnostics(diagnostic_op::error_M123(mi_name, low, high, get_range(op.source)));
                    break;

                default:
                    assert(false);
                    break;
            }
        }
        if (const auto [low, high, passed] = check_op_value(op.first_op, fmt->first); !passed)
        {
            switch (fmt->first.type)
            {
                case LENGTH: // D(L,B)
                    add_diagnostics(diagnostic_op::error_M132(mi_name, low, high, get_range(op.source)));
                    break;
                case IDX_REG: // D(X,B)
                    add_diagnostics(diagnostic_op::error_M135(mi_name, low, high, get_range(op.source)));
                    break;
                case REG: // D(R,B)
                    add_diagnostics(diagnostic_op::error_M133(mi_name, low, high, get_range(op.source)));
                    break;
                case VEC_REG: // D(V,B)
                    add_diagnostics(diagnostic_op::error_M134(mi_name, low, high, get_range(op.source)));
                    break;

                default:
                    assert(false);
                    break;
            }
        }
        if (const auto [low, high, passed] = check_op_value(op.second_op, fmt->second); !passed)
        {
            assert(fmt->second.type == BASE);
            add_diagnostics(diagnostic_op::error_M131(mi_name, get_range(op.source)));
        }

        ++fmt;
    }
}

void check_machine_instruction_operands(const instructions::machine_instruction& mi,
    std::string_view mi_name,
    std::span<const std::unique_ptr<semantics::operand>> ops,
    const range& stmt_range,
    context::dependency_solver& dep_solver,
    diagnostic_collector& diags)
{
    if (!check_op_count(mi.operand_count(), mi_name, ops, stmt_range, diags))
        return;

    if (!check_dependencies(ops, dep_solver, diags))
        return;

    std::array<machine_operand, instructions::machine_instruction::max_operand_count> mach_operands;

    const auto formats = mi.operands();
    auto op_end = evaluate_operands(mach_operands.data(), mi, mi_name, ops, formats, {}, dep_solver, diags);
    if (!op_end)
        return;

    const std::span evaluated_operands(mach_operands.data(), op_end - mach_operands.data());

    check_operands(evaluated_operands, formats, mi_name, ops, diags);
}

void check_mnemonic_code_operands(const instructions::mnemonic_code& mn,
    std::string_view mi_name,
    std::span<const std::unique_ptr<semantics::operand>> ops,
    const range& stmt_range,
    context::dependency_solver& dep_solver,
    diagnostic_collector& diags)
{
    if (!check_op_count(mn.operand_count(), mi_name, ops, stmt_range, diags))
        return;

    if (!check_dependencies(ops, dep_solver, diags))
        return;

    std::array<machine_operand, instructions::machine_instruction::max_operand_count> mach_operands;

    const auto& mi = mn.instruction();
    const auto formats = mi.operands();
    const auto transforms = mn.operand_transformations();

    const auto* op_end =
        evaluate_operands(mach_operands.data(), mi, mi_name, ops, formats, transforms, dep_solver, diags);
    if (!op_end)
        return;

    const std::span evaluated_operands(mach_operands.data(), op_end - mach_operands.data());

    std::array<machine_operand, instructions::machine_instruction::max_operand_count> final_operands;

    const auto* t_end = apply_transforms(final_operands.data(), evaluated_operands, formats.size(), transforms);
    const std::span transformed_operands(final_operands.data(), t_end - final_operands.data());

    check_operands(transformed_operands, formats, mi_name, ops, diags);
}

} // namespace hlasm_plugin::parser_library::checking
