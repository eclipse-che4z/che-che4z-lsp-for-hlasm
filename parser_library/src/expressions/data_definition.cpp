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

#include "data_definition.h"

#include <assert.h>

#include "checking/data_definition/data_def_fields.h"
#include "checking/data_definition/data_def_type_base.h"
#include "checking/diagnostic_collector.h"
#include "compiler_options.h"
#include "context/ordinary_assembly/section.h"
#include "context/ordinary_assembly/symbol.h"
#include "context/using.h"
#include "ebcdic_encoding.h"
#include "mach_expr_term.h"
#include "mach_expr_visitor.h"
#include "semantics/collector.h"
#include "utils/general_hashers.h"
#include "utils/similar.h"

namespace hlasm_plugin::parser_library::expressions {
using utils::hashers::hash_combine;

constexpr char V_type = 'V';
constexpr char R_type = 'R';
constexpr char R_type_suppressed = 'r';

context::dependency_collector data_definition::get_dependencies(context::dependency_solver& solver) const
{
    context::dependency_collector deps = get_length_dependencies(solver);

    if (scale)
        deps.merge(scale->get_dependencies(solver));
    if (exponent)
        deps.merge(exponent->get_dependencies(solver));

    // In V type, the symbols are external, it is not defined in current program and does not
    // have any dependencies.
    if (type != V_type && type != R_type && type != R_type_suppressed && nominal_value)
        deps.merge(nominal_value->get_dependencies(solver));

    return deps;
}

context::dependency_collector data_definition::get_length_dependencies(context::dependency_solver& solver) const
{
    auto result = dupl_factor ? dupl_factor->get_dependencies(solver) : context::dependency_collector();
    result *= length ? length->get_dependencies(solver) : context::dependency_collector();
    return result;
}

const checking::data_def_type* data_definition::access_data_def_type() const
{
    return checking::data_def_type::access_data_def_type(type, extension);
}

context::alignment data_definition::get_alignment() const
{
    if (auto def_type = access_data_def_type(); def_type && !length)
        return def_type->alignment_;
    else
        return context::no_align;
}

char data_definition::get_type_attribute() const
{
    switch (type)
    {
        case 'B':
        case 'C':
        case 'P':
        case 'X':
        case 'Z':
            return type;
        case 'G':
            return '@';
        default:
            break;
    }
    if (length == nullptr)
    {
        switch (type)
        {
            case 'A':
            case 'J':
                return 'A';
            case 'D':
            case 'E':
            case 'F':
            case 'H':
            case 'L':
            case 'Q':
            case 'S':
            case 'V':
            case 'Y':
                return type;
            case 'R':
                return 'V';
            default:
                break;
        }
    }
    else
    {
        switch (type)
        {
            case 'F':
            case 'H':
                return 'G';
            case 'E':
            case 'D':
            case 'L':
                return 'K';
            case 'A':
            case 'S':
            case 'Q':
            case 'J':
            case 'R':
            case 'V':
            case 'Y':
                return 'R';
            default:
                break;
        }
    }
    return 'U';
}

int32_t data_definition::get_scale_attribute(context::dependency_solver& info, diagnostic_op_consumer& diags) const
{
    auto def_type = access_data_def_type();
    if (def_type)
        return def_type->get_scale_attribute(evaluate_scale(info, diags), evaluate_reduced_nominal_value());
    else
        return 0;
}

uint32_t data_definition::get_length_attribute(context::dependency_solver& info, diagnostic_op_consumer& diags) const
{
    auto def_type = access_data_def_type();
    if (def_type)
        return def_type->get_length_attribute(evaluate_length(info, diags), evaluate_reduced_nominal_value());
    else
        return 0;
}

context::integer_type data_definition::get_integer_attribute() const noexcept
{
    auto def_type = access_data_def_type();
    if (def_type)
        return def_type->get_int_type();
    else
        return context::integer_type::undefined;
}

context::symbol_attributes::program_type data_definition::get_program_attribute(
    context::dependency_solver& info, diagnostic_op_consumer& diags) const
{
    if (!program_type)
        return {};
    const auto p = program_type->evaluate(info, diags);
    if (p.value_kind() != context::symbol_value_kind::ABS)
        return {};
    return context::symbol_attributes::program_type((std::uint32_t)p.get_abs());
}

context::symbol_attributes data_definition::get_symbol_attributes(
    context::dependency_solver& solver, diagnostic_op_consumer& diags) const
{
    return context::symbol_attributes(context::symbol_origin::DAT,
        ebcdic_encoding::to_ebcdic((unsigned char)get_type_attribute()),
        get_length_attribute(solver, diags),
        get_scale_attribute(solver, diags),
        get_integer_attribute(),
        get_program_attribute(solver, diags));
}

checking::data_def_field<int32_t> set_data_def_field(
    const expressions::mach_expression* e, context::dependency_solver& info, diagnostic_op_consumer& diags)
{
    using namespace checking;
    data_def_field<int32_t> field;
    // if the expression cannot be evaluated, we return field as if it was not there
    if (e)
    {
        field.rng = e->get_range();

        if (field.present = !e->get_dependencies(info).contains_dependencies())
        {
            auto ret = e->evaluate(info, diags);

            if (ret.value_kind() == context::symbol_value_kind::ABS)
                field.value = ret.get_abs();
            else
            {
                field.present = false;
                diags.add_diagnostic(diagnostic_op::error_D034(e->get_range()));
            }
        }
    }
    return field;
}

checking::dupl_factor_modifier_t data_definition::evaluate_dupl_factor(
    context::dependency_solver& info, diagnostic_op_consumer& diags) const
{
    return dupl_factor ? set_data_def_field(dupl_factor.get(), info, diags) : checking::data_def_field<int32_t>(1);
}

checking::data_def_length_t data_definition::evaluate_length(
    context::dependency_solver& info, diagnostic_op_consumer& diags) const
{
    checking::data_def_length_t len(set_data_def_field(length.get(), info, diags));
    len.len_type = length_type == expressions::data_definition::length_type::BIT ? checking::data_def_length_t::BIT
                                                                                 : checking::data_def_length_t::BYTE;
    return len;
}

checking::scale_modifier_t data_definition::evaluate_scale(
    context::dependency_solver& info, diagnostic_op_consumer& diags) const
{
    auto common = set_data_def_field(scale.get(), info, diags);
    return checking::scale_modifier_t(common.present, (int16_t)common.value, common.rng);
}

checking::reduced_nominal_value_t data_definition::evaluate_reduced_nominal_value() const
{
    if (!nominal_value)
        return std::monostate();

    if (const auto* str = nominal_value->access_string())
        return str->value;
    else if (const auto* exprs = nominal_value->access_exprs())
        return exprs->exprs.size();
    else
        assert(false);
}

long long data_definition::evaluate_total_length(
    context::dependency_solver& info, checking::data_instr_type instr_type, diagnostic_op_consumer& diags) const
{
    auto dd_type = checking::data_def_type::access_data_def_type(type, extension);
    if (!dd_type)
        return -1;
    auto dupl = evaluate_dupl_factor(info, diags);
    auto len = evaluate_length(info, diags);

    if (dupl.present && dupl.value < 0)
        return -1;

    const auto bit_len = len.len_type == checking::data_def_length_t::length_type::BIT;
    if (len.present)
    {
        const auto spec = bit_len ? dd_type->get_bit_length_spec(instr_type) : dd_type->get_length_spec(instr_type);

        if (std::holds_alternative<checking::modifier_bound>(spec))
        {
            if (const auto [min, max, _] = std::get<checking::modifier_bound>(spec); len.value < min || len.value > max)
                return -1;
        }
        else if (std::holds_alternative<checking::n_a>(spec))
            return -1;
        // else if (std::holds_alternative<checking::no_check>(spec));
        // TODO: the following were previously ignored
        // else if (std::holds_alternative<checking::bound_list>(spec));
        else if (len.value < 0) // at least take care of the obvious error
            return -1;
        assert(!std::holds_alternative<checking::ignored>(spec));
    }

    auto result = dd_type->get_length(
        dupl.present ? dupl.value : -1, len.present ? len.value : -1, bit_len, evaluate_reduced_nominal_value());
    return result >= ((1ll << 31) - 1) * 8 ? -1 : (long long)result;
}

void data_definition::apply(mach_expr_visitor& visitor) const
{
    if (dupl_factor)
        dupl_factor->apply(visitor);
    if (program_type)
        program_type->apply(visitor);
    if (length)
        length->apply(visitor);
    if (scale)
        scale->apply(visitor);
    if (exponent)
        exponent->apply(visitor);

    if (const auto* exprs = nominal_value ? nominal_value->access_exprs() : nullptr)
    {
        for (const auto& val : exprs->exprs)
        {
            if (val.base)
                val.base->apply(visitor);
            if (val.displacement)
                val.displacement->apply(visitor);
        }
    }
}

size_t data_definition::hash() const
{
    auto ret = (size_t)0x65b40f329f97f6c9;
    ret = hash_combine(ret, type);
    ret = hash_combine(ret, extension);
    if (length)
        ret = hash_combine(ret, length->hash());

    ret = hash_combine(ret, (size_t)length_type);
    if (dupl_factor)
        ret = hash_combine(ret, dupl_factor->hash());
    if (program_type)
        ret = hash_combine(ret, program_type->hash());
    if (scale)
        ret = hash_combine(ret, scale->hash());
    if (exponent)
        ret = hash_combine(ret, exponent->hash());
    if (nominal_value)
        ret = hash_combine(ret, nominal_value->hash());

    return ret;
}

bool is_similar(const data_definition& l, const data_definition& r) noexcept
{
    return utils::is_similar(l,
        r,
        &data_definition::type,
        &data_definition::extension,
        &data_definition::length,
        &data_definition::length_type,
        &data_definition::dupl_factor,
        &data_definition::program_type,
        &data_definition::scale,
        &data_definition::exponent,
        &data_definition::nominal_value);
}

} // namespace hlasm_plugin::parser_library::expressions
