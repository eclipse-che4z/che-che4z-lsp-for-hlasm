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

// This file contains implementation of the data_def_type for
// these types: A, Y, S, R, V, Q, J

#include "checking/checker_helper.h"
#include "checking/diagnostic_collector.h"
#include "context/ordinary_assembly/symbol_attributes.h"
#include "data_def_types.h"

using namespace hlasm_plugin::parser_library::checking;
using namespace hlasm_plugin::parser_library::context;
using namespace hlasm_plugin::parser_library;

//***************************   types A, Y   *****************************//

data_def_type_A_AD_Y::data_def_type_A_AD_Y(char type, char extension, alignment align, uint64_t implicit_length)
    : data_def_type(type,
          extension,
          no_check(),
          no_check(),
          n_a(),
          n_a(),
          nominal_value_type::EXPRESSIONS,
          align,
          implicit_length,
          integer_type::undefined)
{}

data_def_type_A::data_def_type_A()
    : data_def_type_A_AD_Y('A', '\0', fullword, 4)
{}

bool check_A_AD_Y_length(std::string_view type,
    const data_definition_operand& op,
    const diagnostic_collector& add_diagnostic,
    int min_byte_abs,
    int max_byte_abs,
    int min_byte_sym,
    int max_byte_sym,
    int min_bit,
    int max_bit)
{
    bool all_absolute = true;
    if (!op.length.present)
        return true;

    for (auto e : std::get<nominal_value_expressions>(op.nominal_value.value))
    {
        const data_def_expr& expr = std::get<data_def_expr>(e);
        if (!expr.ignored && expr.ex_kind != expr_type::ABS)
        {
            all_absolute = false;
            break;
        }
    }
    // For absolute expressions, it is possible to specify bit or byte length modifier with bounds specified in
    // parameters.
    if (all_absolute)
    {
        if (op.length.len_type == data_def_length_t::length_type::BIT
            && (op.length.value < min_bit || op.length.value > max_bit))
        {
            add_diagnostic(diagnostic_op::error_D008(op.length.rng, type, "bit length", min_bit, max_bit));
            return false;
        }
        else if (op.length.len_type == data_def_length_t::BYTE
            && (op.length.value < min_byte_abs || op.length.value > max_byte_abs))
        {
            add_diagnostic(diagnostic_op::error_D008(op.length.rng, type, "length", min_byte_abs, max_byte_abs));
            return false;
        }
    }
    else
    { // For relocatable expressions, bit length is not allowed and byte has specific bounds.
        if (op.length.len_type == data_def_length_t::BIT)
        {
            add_diagnostic(diagnostic_op::error_D007(op.length.rng, type, " with relocatable symbols"));
            return false;
        }
        else if (op.length.len_type == data_def_length_t::BYTE
            && (op.length.value < min_byte_sym || op.length.value > max_byte_sym))
        {
            add_diagnostic(diagnostic_op::error_D008(
                op.length.rng, type, "length", min_byte_sym, max_byte_sym, " with relocatable symbols"));
            return false;
        }
    }
    return true;
}

bool data_def_type_A::check_impl(
    const data_definition_operand& op, const diagnostic_collector& add_diagnostic, bool check_nominal) const
{
    if (!check_nominal)
        return true;
    return check_A_AD_Y_length("A", op, add_diagnostic, 1, 8, 2, 4, 1, 128);
}

data_def_type_AD::data_def_type_AD()
    : data_def_type_A_AD_Y('A', 'D', doubleword, 8)
{}

bool data_def_type_AD::check_impl(
    const data_definition_operand& op, const diagnostic_collector& add_diagnostic, bool check_nominal) const
{
    if (!check_nominal)
        return true;
    return check_A_AD_Y_length("AD", op, add_diagnostic, 1, 8, 2, 8, 1, 128);
}

data_def_type_Y::data_def_type_Y()
    : data_def_type_A_AD_Y('Y', '\0', halfword, 2)
{}

bool data_def_type_Y::check_impl(
    const data_definition_operand& op, const diagnostic_collector& add_diagnostic, bool check_nominal) const
{
    if (!check_nominal)
        return true;
    return check_A_AD_Y_length("Y", op, add_diagnostic, 1, 2, 2, 2, 1, 16);
}

//***************************   types S, SY   *****************************//

data_def_type_S_SY::data_def_type_S_SY(char extension, int size)
    : data_def_type('S',
          extension,
          n_a(),
          modifier_bound { size, size },
          n_a(),
          n_a(),
          nominal_value_type::ADDRESS_OR_EXPRESSION,
          halfword,
          (unsigned long long)size,
          integer_type::undefined)
{}

data_def_type_S::data_def_type_S()
    : data_def_type_S_SY('\0', 2)
{}
// Checks S and SY operand, size specifies size of displacement in bits,
// is_signed specifies whether first bit is sign bit
template<size_t size, bool is_signed>
bool check_S_SY_operand(const data_definition_operand& op, const diagnostic_collector& add_diagnostic)
{
    bool ret = true;
    for (auto& e : std::get<nominal_value_expressions>(op.nominal_value.value))
    {
        if (std::holds_alternative<data_def_address>(e))
        {
            const auto& adr = std::get<data_def_address>(e);
            if (adr.ignored)
                continue;
            if (is_signed)
            {
                if (!is_size_corresponding_signed(adr.displacement.value, size))
                {
                    add_diagnostic(diagnostic_op::error_D022(adr.displacement.rng));
                    ret = false;
                }
            }
            else
            {
                if (!is_size_corresponding_unsigned(adr.displacement.value, size))
                {
                    add_diagnostic(diagnostic_op::error_D022(adr.displacement.rng));
                    ret = false;
                }
            }
            if (!is_size_corresponding_unsigned(adr.base.value, 4))
            {
                add_diagnostic(diagnostic_op::error_D023(adr.base.rng));
                ret = false;
            }
        }
        else if (std::holds_alternative<data_def_expr>(e))
        {
            // The expression specifies address displacement, base is implicit.
            const auto& expr = std::get<data_def_expr>(e);
            if (expr.ignored)
                continue;
            if (is_signed)
            {
                if (!is_size_corresponding_signed(expr.value, size))
                {
                    add_diagnostic(diagnostic_op::error_D022(expr.rng));
                    ret = false;
                }
            }
            else
            {
                if (!is_size_corresponding_unsigned(expr.value, size))
                {
                    add_diagnostic(diagnostic_op::error_D022(expr.rng));
                    ret = false;
                }
            }
        }
    }
    return ret;
}

bool data_def_type_S::check_impl(
    const data_definition_operand& op, const diagnostic_collector& add_diagnostic, bool check_nominal) const
{
    if (!check_nominal)
        return true;
    return check_S_SY_operand<12, false>(op, add_diagnostic);
}

data_def_type_SY::data_def_type_SY()
    : data_def_type_S_SY('Y', 3)
{}

bool data_def_type_SY::check_impl(
    const data_definition_operand& op, const diagnostic_collector& add_diagnostic, bool check_nominal) const
{
    if (!check_nominal)
        return true;
    return check_S_SY_operand<20, true>(op, add_diagnostic);
}

//***************************   types R, RD   *****************************//

data_def_type_single_symbol::data_def_type_single_symbol(
    char type, char extension, modifier_spec length_bound, alignment align, uint64_t implicit_length)
    : data_def_type(type,
          extension,
          n_a(),
          length_bound,
          n_a(),
          n_a(),
          nominal_value_type::EXPRESSIONS,
          align,
          implicit_length,
          integer_type::undefined,
          expects_single_symbol_t::yes)
{}

data_def_type_R::data_def_type_R()
    : data_def_type_single_symbol('R', '\0', modifier_bound { 3, 4 }, fullword, 4)
{}

data_def_type_RD::data_def_type_RD()
    : data_def_type_single_symbol('R', 'D', no_check(), doubleword, 8)
{}

bool data_def_type_RD::check_impl(
    const data_definition_operand& op, const diagnostic_collector& add_diagnostic, bool) const
{
    if (op.length.present && op.length.value != 3 && op.length.value != 4 && op.length.value != 8)
    {
        add_diagnostic(diagnostic_op::error_D021(op.length.rng, type_str));
        return false;
    }
    return true;
}

//***************************   types V, VD   *****************************//

data_def_type_V::data_def_type_V()
    : data_def_type_single_symbol('V', '\0', modifier_bound { 3, 4 }, fullword, 4)
{}

data_def_type_VD::data_def_type_VD()
    : data_def_type_single_symbol('V', 'D', no_check(), doubleword, 8)
{}

bool data_def_type_VD::check_impl(
    const data_definition_operand& op, const diagnostic_collector& add_diagnostic, bool) const
{
    if (op.length.present && op.length.value != 3 && op.length.value != 4 && op.length.value != 8)
    {
        add_diagnostic(diagnostic_op::error_D021(op.length.rng, type_str));
        return false;
    }
    return true;
}

//***************************   types Q, QD, QY   *****************************//

data_def_type_Q::data_def_type_Q()
    : data_def_type_single_symbol('Q', '\0', modifier_bound { 1, 4 }, fullword, 4)
{}

data_def_type_QD::data_def_type_QD()
    : data_def_type_single_symbol('Q', 'D', modifier_bound { 1, 8 }, quadword, 8)
{}

data_def_type_QY::data_def_type_QY()
    : data_def_type_single_symbol('Q', 'Y', modifier_bound { 3, 3 }, halfword, 3)
{}


//***************************   types J, JD   *****************************//

data_def_type_J::data_def_type_J()
    : data_def_type_single_symbol('J', '\0', no_check(), fullword, 4)
{}

bool data_def_type_J::check_impl(
    const data_definition_operand& op, const diagnostic_collector& add_diagnostic, bool) const
{
    if (op.length.present && op.length.value != 2 && op.length.value != 3 && op.length.value != 4
        && op.length.value != 8)
    {
        add_diagnostic(diagnostic_op::error_D024(op.length.rng, type_str));
        return false;
    }
    return true;
}


data_def_type_JD::data_def_type_JD()
    : data_def_type_single_symbol('J', 'D', no_check(), doubleword, 8)
{}

bool data_def_type_JD::check_impl(
    const data_definition_operand& op, const diagnostic_collector& add_diagnostic, bool) const
{
    if (op.length.present && op.length.value != 2 && op.length.value != 3 && op.length.value != 4
        && op.length.value != 8)
    {
        add_diagnostic(diagnostic_op::error_D024(op.length.rng, type_str));
        return false;
    }
    return true;
}
