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
#include "checking/checker_helper.h"
#include "checking/diagnostic_collector.h"
#include "checking/using_label_checker.h"
#include "compiler_options.h"
#include "context/ordinary_assembly/section.h"
#include "context/ordinary_assembly/symbol.h"
#include "context/ordinary_assembly/symbol_value.h"
#include "context/using.h"
#include "data_definition_operand.h"
#include "expressions/nominal_value.h"
#include "instructions/instruction.h"
#include "semantics/operand_impls.h"

namespace hlasm_plugin::parser_library::checking {

namespace {
std::pair<const data_def_type*, bool> check_type_and_extension(
    const expressions::data_definition& dd, const diagnostic_collector& add_diagnostic)
{
    if (const auto found = data_def_type::access_data_def_type(dd.type, dd.extension))
        return { found, true };

    if (dd.extension)
    {
        if (const auto found = data_def_type::access_data_def_type(dd.type, '\0'))
        {
            add_diagnostic(diagnostic_op::error_D013(dd.extension_range, std::string_view(&dd.type, 1)));
            return { found, false };
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
    if (!e)
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
    const data_def_type& dd;
    std::string_view modifier_name;
    const diagnostic_collector& add_diagnostic;

    bool operator()(const ignored&) const
    {
        const bool tolerate = value == 0;
        if (tolerate)
            add_diagnostic(diagnostic_op::warn_D025(expr, dd.type_str(), modifier_name));
        else
            add_diagnostic(diagnostic_op::error_D009(expr, dd.type_str(), modifier_name));
        return tolerate;
    }
    bool operator()(const n_a&) const
    {
        // modifier not allowed with this type
        add_diagnostic(diagnostic_op::error_D009(expr, dd.type_str(), modifier_name));
        return false;
    }
    bool operator()(const bound_list& list) const
    {
        if (!list.allowed(value))
        {
            add_diagnostic(diagnostic_op::error_D021(expr, modifier_name, list.to_diag_list(), dd.type_str()));
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
            add_diagnostic(diagnostic_op::error_D008(expr, dd.type_str(), modifier_name, min, max));
            return false;
        }
        else if (even && value % 2 == 1)
        {
            add_diagnostic(diagnostic_op::error_D014(expr, modifier_name, dd.type_str()));
            return false;
        }
        return true;
    }
};

bool check_base(const data_def_type& type,
    const data_definition_common& op,
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
        if (std::holds_alternative<n_a>(type.bit_length_spec_) && op.length_in_bits)
        {
            // bit length not allowed with this type
            add_diagnostic(diagnostic_op::error_D007(*op.rng_length, type.type_str()));
            ret = false;
        }
        else if (op.length_in_bits)
            ret &= std::visit(check_modifier { op.length, *op.rng_length, type, "bit length", add_diagnostic },
                type.get_bit_length_spec(instr_type));
        else
            ret &= std::visit(check_modifier { op.length, *op.rng_length, type, "length", add_diagnostic },
                type.get_length_spec(instr_type));
    }

    if (op.has_scale())
        ret &= std::visit(check_modifier { op.scale, *op.rng_scale, type, "scale", add_diagnostic }, type.scale_spec_);
    if (op.has_exponent())
        ret &= std::visit(
            check_modifier { op.exponent, *op.rng_exponent, type, "exponent", add_diagnostic }, type.exponent_spec_);

    return ret;
}

bool all_values_are_absolute(const std::vector<expressions::address_nominal>& exprs,
    context::dependency_solver& dep_solver,
    const diagnostic_collector& add_diagnostic) noexcept
{
    diagnostic_consumer_transform diags([&add_diagnostic](diagnostic_op d) { add_diagnostic(std::move(d)); });
    bool result = true;
    for (const auto& addr : exprs)
    {
        assert(!addr.base);
        const auto value = addr.displacement->evaluate(dep_solver, diags);
        result &= value.value_kind() != context::symbol_value_kind::RELOC; // UNDEF should produce its own diags
    }
    return result;
}

template<std::predicate<char> F>
bool check_comma_separated(std::string_view nom, F is_valid_digit)
{
    bool last_valid = false;
    for (char c : nom)
    {
        if (c == ' ')
            continue;
        if (c == ',')
        {
            if (!last_valid)
                return false;
            last_valid = false;
        }
        else if (is_valid_digit(c))
            last_valid = true;
        else
            return false;
    }
    if (!last_valid)
        return false;
    return true;
}

const expressions::nominal_value_exprs* has_only_simple_expressions(
    const data_def_type& dd, const expressions::nominal_value_t& nominal, const diagnostic_collector& add_diagnostic)
{
    const auto* exprs = nominal.access_exprs();
    if (!exprs)
    {
        add_diagnostic(diagnostic_op::error_D017(nominal.value_range, dd.type_str()));
        return nullptr;
    }
    for (auto& p : exprs->exprs)
    {
        if (p.base)
        {
            add_diagnostic(diagnostic_op::error_D020(p.total, dd.type_str()));
            exprs = nullptr;
        }
    }
    return exprs;
}

constexpr bool is_valid_external_symbol(const context::section& s) noexcept
{
    using enum context::section_kind;
    return s.kind == DUMMY || s.kind == EXTERNAL_DSECT;
}

bool has_single_symbol_only(const data_def_type& dd,
    const std::vector<expressions::address_nominal>& exprs,
    const diagnostic_collector& add_diagnostic)
{
    bool result = true;
    for (const auto& addr : exprs)
    {
        if (addr.base)
        {
            add_diagnostic(diagnostic_op::error_D030(addr.base->get_range(), dd.type_str()));
            result = false;
            continue;
        }
        const auto* expr = addr.displacement.get();
        const auto* symbol = dynamic_cast<const expressions::mach_expr_symbol*>(expr);
        if (!symbol)
        {
            add_diagnostic(diagnostic_op::error_D030(expr->get_range(), dd.type_str()));
            result = false;
        }
    }
    return result;
}

bool check_q_nominal(const std::vector<expressions::address_nominal>& exprs,
    context::dependency_solver& info,
    const diagnostic_collector& add_diagnostic)
{
    static constexpr std::string_view type = "Q";

    const auto goff = info.get_options().sysopt_xobject;

    bool result = true;
    for (const auto& addr : exprs)
    {
        if (addr.base)
        {
            add_diagnostic(diagnostic_op::error_D030(addr.base->get_range(), type));
            result = false;
            continue;
        }
        const auto* expr = addr.displacement.get();
        const auto* symbol_expr = dynamic_cast<const expressions::mach_expr_symbol*>(expr);
        if (!symbol_expr)
        {
            add_diagnostic(diagnostic_op::error_D030(expr->get_range(), type));
            result = false;
            continue;
        }
        if (goff)
        {
            if (const auto* s = info.get_symbol(symbol_expr->value);
                s && s->attributes().origin() == context::symbol_origin::SECT)
                continue;
        }
        else if (const auto* s = info.get_section(symbol_expr->value); s && is_valid_external_symbol(*s))
            continue;

        add_diagnostic(diagnostic_op::error_D035(symbol_expr->get_range(), goff));
        result = false;
    }

    return result;
}

std::pair<int32_t, int32_t> transate_address_via_using(const context::address& addr,
    context::dependency_solver& dep_solver,
    diagnostic_op_consumer& diags,
    const range& r,
    bool extended)
{
    const auto& base = addr.bases().front();
    const auto translated_addr = dep_solver.using_evaluate(base.qualifier, base.owner, addr.offset(), extended);

    if (translated_addr.reg == context::using_collection::invalid_register)
    {
        if (translated_addr.reg_offset)
            diags.add_diagnostic(diagnostic_op::error_ME008(translated_addr.reg_offset, r));
        else
            diags.add_diagnostic(diagnostic_op::error_ME007(r));
        return {};
    }
    return { translated_addr.reg_offset, translated_addr.reg };
}

bool check_S_SY_operand(const expressions::address_nominal& addr,
    context::dependency_solver& dep_solver,
    const diagnostic_collector& add_diagnostic,
    bool extended)
{
    diagnostic_consumer_transform diags([&add_diagnostic](diagnostic_op d) { add_diagnostic(std::move(d)); });

    int32_t d = 0;
    int32_t b = 0;
    if (addr.base)
    {
        const auto d_value = addr.displacement->evaluate(dep_solver, diags);
        const auto b_value = addr.base->evaluate(dep_solver, diags);
        const auto d_abs = d_value.value_kind() == context::symbol_value_kind::ABS;
        const auto b_abs = b_value.value_kind() == context::symbol_value_kind::ABS;
        if (!d_abs)
            add_diagnostic(diagnostic_op::error_D034(addr.displacement->get_range()));
        if (!b_abs)
            add_diagnostic(diagnostic_op::error_D034(addr.base->get_range()));
        if (!d_abs || !b_abs)
            return false;
        d = d_value.get_abs();
        b = b_value.get_abs();
    }
    else
    {
        switch (const auto d_value = addr.displacement->evaluate(dep_solver, diags); d_value.value_kind())
        {
            case context::symbol_value_kind::UNDEF:
                add_diagnostic(diagnostic_op::error_D034(addr.displacement->get_range()));
                return false;

            case context::symbol_value_kind::ABS:
                d = d_value.get_abs();
                b = 0;
                break;

            case context::symbol_value_kind::RELOC:
                if (const auto& a = d_value.get_reloc(); a.is_simple())
                    std::tie(d, b) = transate_address_via_using(a, dep_solver, diags, addr.total, extended);
                else
                {
                    add_diagnostic(diagnostic_op::error_D033(addr.total));
                    return false;
                }
                break;
        }
    }

    bool result = true;
    if (extended)
    {
        if (!is_size_corresponding_signed(d, 20))
        {
            add_diagnostic(diagnostic_op::error_D022(addr.displacement->get_range()));
            result = false;
        }
    }
    else
    {
        if (!is_size_corresponding_unsigned(d, 12))
        {
            add_diagnostic(diagnostic_op::error_D022(addr.displacement->get_range()));
            result = false;
        }
    }
    if (addr.base)
    {
        if (!is_size_corresponding_unsigned(b, 4))
        {
            add_diagnostic(diagnostic_op::error_D023(addr.base->get_range()));
            result = true;
        }
    }
    return result;
}

bool check_nominal(const data_def_type& dd,
    const data_definition_common& common,
    const semantics::data_def_operand& op,
    data_instr_type subtype,
    context::dependency_solver& dep_solver,
    const diagnostic_collector& add_diagnostic)
{
    switch (subtype)
    {
        case data_instr_type::DS:
            if (!op.value->nominal_value)
                return true;
            break;

        case data_instr_type::DC:
            // nominal value can be omitted with DC when duplication factor is 0.
            if (common.dupl_factor == 0 && common.has_dupl_factor() && !op.value->nominal_value)
                return true;
            if (!op.value->nominal_value)
            {
                add_diagnostic(diagnostic_op::error_D016(op.operand_range));
                return false;
            }
            break;
    }

    const auto& nominal = *op.value->nominal_value;

    nominal_diag_func diag_func = nullptr;

    // TODO: we are still missing length checks on nominal length for C, G, or X
    switch (dd.type())
    {
        case data_definition_type::A:
        case data_definition_type::Y: {
            const auto* exprs = has_only_simple_expressions(dd, nominal, add_diagnostic);
            if (!exprs)
                return false;

            // TODO: overflow checks are missing
            if (!all_values_are_absolute(exprs->exprs, dep_solver, add_diagnostic) && common.length_in_bits)
            {
                static constexpr std::string_view reloc_suffix = " with relocatable symbols";
                add_diagnostic(diagnostic_op::error_D007(nominal.value_range, dd.type_str(), reloc_suffix));
                return false;
            }
            return true;
        }

        case data_definition_type::Q:
            if (const auto* exprs = nominal.access_exprs(); exprs)
                return check_q_nominal(exprs->exprs, dep_solver, add_diagnostic);
            diag_func = diagnostic_op::error_D017;
            break;

        case data_definition_type::J:
        case data_definition_type::R:
        case data_definition_type::V:
            if (const auto* exprs = nominal.access_exprs(); exprs)
                return has_single_symbol_only(dd, exprs->exprs, add_diagnostic);
            diag_func = diagnostic_op::error_D017;
            break;

        case data_definition_type::S: { // special case for now
            if (const auto* exprs = nominal.access_exprs(); exprs)
            {
                bool ret = true;
                const auto ext = dd.extension() == 'Y';
                for (const auto& addr : exprs->exprs)
                {
                    ret &= check_S_SY_operand(addr, dep_solver, add_diagnostic, ext);
                }
                return ret;
            }
            diag_func = diagnostic_op::error_D017;
            break;
        }

        case data_definition_type::H:
        case data_definition_type::F:
            if (const auto* str = nominal.access_string())
                diag_func = check_nominal_H_F_FD(str->value);
            else
                diag_func = diagnostic_op::error_D018;
            break;

        case data_definition_type::P:
        case data_definition_type::Z:
            if (const auto* str = nominal.access_string())
                diag_func = check_nominal_P_Z(str->value);
            else
                diag_func = diagnostic_op::error_D018;
            break;

        case data_definition_type::E:
        case data_definition_type::D:
        case data_definition_type::L:
            if (const auto* str = nominal.access_string())
                diag_func = check_nominal_E_D_L(str->value, dd.extension());
            else
                diag_func = diagnostic_op::error_D018;
            break;

        case data_definition_type::B:
            if (const auto* str = nominal.access_string(); !str)
                diag_func = diagnostic_op::error_D018;
            else if (!check_comma_separated(str->value, [](char c) { return c == '0' || c == '1'; }))
                diag_func = diagnostic_op::error_D010;
            break;

        case data_definition_type::X:
            if (const auto* str = nominal.access_string(); !str)
                diag_func = diagnostic_op::error_D018;
            else if (!check_comma_separated(str->value, &is_hexadecimal_digit))
                diag_func = diagnostic_op::error_D010;
            break;

        case data_definition_type::C:
        case data_definition_type::G:
            if (const auto* str = nominal.access_string(); !str)
                diag_func = diagnostic_op::error_D018;
            break;
    }
    if (diag_func)
    {
        add_diagnostic(diag_func(nominal.value_range, dd.type_str()));
        return false;
    }
    return true;
}

reduced_nominal_value_t reduce_nominal_value(const expressions::nominal_value_t* n)
{
    if (!n)
        return std::monostate();
    else if (const auto* str = n->access_string())
        return str->value;
    else if (const auto* expr = n->access_exprs())
        return expr->exprs.size();

    assert(false);
    return {};
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

        const auto common = evaluate_common(*op->value, dep_solver, diags);

        const auto base_passed = check_base(*def_type, common, subtype, add_diagnostic);

        const auto nom_passed = check_nominal(*def_type, common, *op, subtype, dep_solver, add_diagnostic);

        if (!base_passed || !nom_passed)
        {
            continue;
        }

        const auto bit_length = def_type->get_length(common.has_dupl_factor() ? common.dupl_factor : -1,
            common.has_length() ? common.length : -1,
            common.length_in_bits,
            reduce_nominal_value(op->value->nominal_value.get()));

        if (bit_length >= ((1ll << 31) - 1) * 8)
        {
            add_diagnostic(diagnostic_op::error_D028(op->operand_range));
            continue;
        }

        if (!common.length_in_bits)
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
