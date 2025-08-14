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

// This file implements checking of data definition
// asm instructions from the assembler checker:
// DC, DS, DXD

#include "../data_check.h"
#include "checking/asm_instr_check.h"
#include "checking/diagnostic_collector.h"
#include "checking/using_label_checker.h"
#include "context/ordinary_assembly/symbol_value.h"
#include "data_definition_operand.h"
#include "instructions/instruction.h"
#include "semantics/operand_impls.h"

namespace hlasm_plugin::parser_library::checking {

namespace {
std::pair<const data_def_type*, bool> check_type_and_extension(
    const expressions::data_definition& dd, const diagnostic_collector& add_diagnostic)
{
    auto found = data_def_type::types_and_extensions.find({ dd.type, dd.extension });

    if (found != data_def_type::types_and_extensions.end())
        return { found->second.get(), true };

    if (dd.extension)
    {
        found = data_def_type::types_and_extensions.find({ dd.type, '\0' });
        if (found != data_def_type::types_and_extensions.end())
        {
            add_diagnostic(diagnostic_op::error_D013(dd.extension_range, std::string_view(&dd.type, 1)));
            return { found->second.get(), false };
        }
    }

    add_diagnostic(diagnostic_op::error_D012(dd.type_range));
    return { nullptr, false };
}

data_instr_type translate(instructions::data_def_instruction t) noexcept
{
    switch (t)
    {
        case instructions::data_def_instruction::DC_TYPE:
            return data_instr_type::DC;
        case instructions::data_def_instruction::DS_TYPE:
            return data_instr_type::DS;
        default:
            assert(false);
    }
}

std::optional<int32_t> evaluate_abs_expression(
    const expressions::mach_expression* e, context::dependency_solver& info, diagnostic_op_consumer& diags)
{
    // if the expression cannot be evaluated, we return field as if it was not there
    if (!e || e->get_dependencies(info).contains_dependencies())
        return std::nullopt;

    if (const auto ret = e->evaluate(info, diags); ret.value_kind() == context::symbol_value_kind::ABS)
        return ret.get_abs();

    diags.add_diagnostic(diagnostic_op::error_D034(e->get_range()));
    return std::nullopt;
}

template<typename T>
auto as_ptr(T&& t) noexcept
{
    static_assert(std::is_lvalue_reference_v<T>);
    return &t;
}

data_definition_common evaluate_common(
    const expressions::data_definition& dd, context::dependency_solver& dep_solver, diagnostic_op_consumer& diags)
{
    data_definition_common result;
    result.length_in_bits = dd.length_type == expressions::data_definition::length_type::BIT;

    if (auto r = evaluate_abs_expression(dd.dupl_factor.get(), dep_solver, diags))
    {
        result.dupl_factor = *r;
        result.rng_dupl_factor = as_ptr(dd.dupl_factor->get_range());
    }
    if (auto r = evaluate_abs_expression(dd.length.get(), dep_solver, diags))
    {
        result.length = *r;
        result.rng_length = as_ptr(dd.length->get_range());
    }
    if (auto r = evaluate_abs_expression(dd.scale.get(), dep_solver, diags))
    {
        result.scale = *r;
        result.rng_scale = as_ptr(dd.scale->get_range());
    }
    if (auto r = evaluate_abs_expression(dd.exponent.get(), dep_solver, diags))
    {
        result.exponent = *r;
        result.rng_exponent = as_ptr(dd.exponent->get_range());
    }

    return result;
}

struct check_modifier
{
    int32_t value;
    const range& expr;
    std::string_view type_str;
    std::string_view modifier_name;
    const diagnostic_collector& add_diagnostic;

    bool operator()(const no_check&) const { return true; }
    bool operator()(const ignored&) const
    {
        const bool tolerate = value == 0;
        if (tolerate)
            add_diagnostic(diagnostic_op::warn_D025(expr, type_str, modifier_name));
        else
            add_diagnostic(diagnostic_op::error_D009(expr, type_str, modifier_name));
        return tolerate;
    }
    bool operator()(const n_a&) const
    {
        // modifier not allowed with this type
        add_diagnostic(diagnostic_op::error_D009(expr, type_str, modifier_name));
        return false;
    }
    bool operator()(const bound_list& list) const
    {
        if (!list.allowed(value))
        {
            add_diagnostic(diagnostic_op::error_D021(expr, modifier_name, list.to_diag_list(), type_str));
            return false;
        }
        return true;
    }
    bool operator()(const modifier_bound& mb) const
    {
        const auto [min, max, even] = mb;
        if (value < min || value > max)
        {
            // modifier out of bounds
            add_diagnostic(diagnostic_op::error_D008(expr, type_str, modifier_name, min, max));
            return false;
        }
        else if (even && value % 2 == 1)
        {
            add_diagnostic(diagnostic_op::error_D014(expr, modifier_name, type_str));
            return false;
        }
        return true;
    }
};

std::pair<bool, bool> check_nominal_present(const data_definition_common& common,
    bool has_nominal,
    const range& rng,
    data_instr_type instr_type,
    const diagnostic_collector& add_diagnostic)
{
    // DS does not require nominal value
    // however if nominal value present, it must be valid
    if (instr_type == data_instr_type::DS)
        return { true, has_nominal };

    // nominal value can be omitted with DC when duplication factor is 0.
    bool ret = true;
    bool check_nom = true;
    if (common.dupl_factor == 0 && common.has_dupl_factor())
        check_nom = false;

    if (!has_nominal && check_nom)
    {
        add_diagnostic(diagnostic_op::error_D016(rng));
        ret = false;
        check_nom = false;
    }
    else if (has_nominal) // however if nominal value present, it must be valid
        check_nom = true;

    return { ret, check_nom };
}

bool check_base(const data_def_type& type,
    const data_definition_common& op,
    bool bits,
    data_instr_type instr_type,
    const diagnostic_collector& add_diagnostic)
{
    bool ret = true;
    if (op.has_dupl_factor() && op.dupl_factor < 0)
    {
        // Duplication factor must be non negative
        add_diagnostic(diagnostic_op::error_D011(*op.rng_dupl_factor));
        ret = false;
    }

    if (op.has_length())
    {
        if (std::holds_alternative<n_a>(type.bit_length_spec_) && bits)
        {
            // bit length not allowed with this type
            add_diagnostic(diagnostic_op::error_D007(*op.rng_length, type.type_str));
            ret = false;
        }
        else if (bits)
            ret &= std::visit(check_modifier { op.length, *op.rng_length, type.type_str, "bit length", add_diagnostic },
                type.get_bit_length_spec(instr_type));
        else
            ret &= std::visit(check_modifier { op.length, *op.rng_length, type.type_str, "length", add_diagnostic },
                type.get_length_spec(instr_type));
    }

    if (op.has_scale())
        ret &= std::visit(
            check_modifier { op.scale, *op.rng_scale, type.type_str, "scale", add_diagnostic }, type.scale_spec_);
    if (op.has_exponent())
        ret &= std::visit(check_modifier { op.exponent, *op.rng_exponent, type.type_str, "exponent", add_diagnostic },
            type.exponent_spec_);

    return ret;
}

} // namespace

void check_data_instruction_operands(const instructions::assembler_instruction& ai,
    std::span<const std::unique_ptr<semantics::operand>> ops,
    const range& stmt_range,
    context::dependency_solver& dep_solver,
    diagnostic_collector& add_diagnostic)
{
    if (ops.empty())
    {
        add_diagnostic(diagnostic_op::error_A010_minimum(ai.name(), 1, stmt_range));
        return;
    }

    const auto subtype = translate(ai.data_def_type());

    diagnostic_consumer_transform diags([&add_diagnostic](diagnostic_op d) { add_diagnostic(std::move(d)); });
    checking::using_label_checker lc(dep_solver, diags);
    std::vector<context::id_index> missing_symbols;

    auto operands_bit_length = 0ULL;
    for (const auto& operand : ops)
    {
        const auto* op = operand->access_data_def();
        if (!op)
        {
            add_diagnostic(diagnostic_op::error_A004_data_def_expected(operand->operand_range));
            continue;
        }

        assert(ai.has_ord_symbols());
        assert(!ai.postpone_dependencies());

        if (const auto deps = op->value->get_dependencies(dep_solver); deps.contains_dependencies())
        {
            deps.collect_unique_symbolic_dependencies(missing_symbols);
            for (const auto& symbol : missing_symbols)
                add_diagnostic(
                    diagnostic_op::error_E010("ordinary symbol", symbol.to_string_view(), op->operand_range));
            if (missing_symbols.empty()) // this is a fallback message if somehow non-symbolic deps are not resolved
                add_diagnostic(diagnostic_op::error_E016(op->operand_range));
            else
                missing_symbols.clear();
            continue;
        }

        op->apply_mach_visitor(lc);

        const auto [def_type, exact_match] = check_type_and_extension(*op->value, add_diagnostic);
        if (!exact_match)
        {
            continue;
        }

        const auto bits = op->value->length_type == expressions::data_definition::length_type::BIT;

        const auto common = evaluate_common(*op->value, dep_solver, diags);

        const auto base_passed = check_base(*def_type, common, bits, subtype, add_diagnostic);

        const auto [nom_present_ok, check_nom] =
            check_nominal_present(common, !!op->value->nominal_value, op->operand_range, subtype, add_diagnostic);

        const auto nominal = op->value->evaluate_nominal_value(dep_solver, diags);

        const auto nom_ok = !check_nom || def_type->check_nominal_type(nominal, add_diagnostic, op->operand_range);

        const auto detail_pass = def_type->check_impl(common, nominal, add_diagnostic, check_nom && nom_ok);

        if (!base_passed || !nom_ok || !detail_pass)
        {
            continue;
        }

        const auto bit_length = def_type->get_length(common.has_dupl_factor() ? common.dupl_factor : -1,
            common.has_length() ? common.length : -1,
            bits,
            reduce_nominal_value(nominal));

        if (bit_length >= ((1ll << 31) - 1) * 8)
        {
            add_diagnostic(diagnostic_op::error_D028(op->operand_range));
            continue;
        }

        if (!bits)
        {
            // align to whole byte
            operands_bit_length = round_up(operands_bit_length, 8ULL);

            // enforce data def alignment
            const context::alignment al = op->value->get_alignment();

            operands_bit_length = round_up(operands_bit_length, al.boundary * 8ULL);
        }

        operands_bit_length += bit_length;
    }

    // align to whole byte
    operands_bit_length = round_up(operands_bit_length, 8ULL);

    if (operands_bit_length / 8 > INT32_MAX)
    {
        add_diagnostic(diagnostic_op::error_D029(stmt_range));
    }
}

} // namespace hlasm_plugin::parser_library::checking
