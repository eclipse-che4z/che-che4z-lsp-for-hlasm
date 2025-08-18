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

#include "context/ordinary_assembly/symbol_attributes.h"
#include "data_def_types.h"
#include "diagnostic_op.h"

namespace hlasm_plugin::parser_library::checking {

//***************************   types A, Y   *****************************//

data_def_type_A_AD_Y::data_def_type_A_AD_Y(
    data_definition_type type, char extension, context::alignment align, uint64_t implicit_length)
    : data_def_type(type,
          extension,
          no_check(),
          no_check(),
          n_a(),
          n_a(),
          align,
          implicit_length,
          context::integer_type::undefined)
{}

data_def_type_A::data_def_type_A()
    : data_def_type_A_AD_Y(data_definition_type::A, '\0', context::fullword, 4)
{}

template<int min_byte_abs, int max_byte_abs, int min_byte_sym, int max_byte_sym, int min_bit, int max_bit>
nominal_diag_func check_A_AD_Y_length(const data_definition_common& common, bool all_absolute) noexcept
{
    // For absolute expressions, it is possible to specify bit or byte length modifier with bounds specified in
    // parameters.
    if (all_absolute)
    {
        if (common.length_in_bits && (common.length < min_bit || common.length > max_bit))
        {
            return [](const range& r, std::string_view type) {
                return diagnostic_op::error_D008(r, type, "bit length", min_bit, max_bit);
            };
        }
        else if (!common.length_in_bits && (common.length < min_byte_abs || common.length > max_byte_abs))
        {
            return [](const range& r, std::string_view type) {
                return diagnostic_op::error_D008(r, type, "length", min_byte_abs, max_byte_abs);
            };
        }
    }
    else
    { // For relocatable expressions, bit length is not allowed and byte has specific bounds.
        if (common.length_in_bits)
        {
            return [](const range& r, std::string_view type) {
                return diagnostic_op::error_D007(r, type, " with relocatable symbols");
            };
        }
        else if (!common.length_in_bits && (common.length < min_byte_sym || common.length > max_byte_sym))
        {
            return [](const range& r, std::string_view type) {
                return diagnostic_op::error_D008(
                    r, type, "length", min_byte_sym, max_byte_sym, " with relocatable symbols");
            };
        }
    }
    return nullptr;
}

nominal_diag_func check_A_length(const data_definition_common& common, bool all_absolute) noexcept
{
    return check_A_AD_Y_length<1, 8, 2, 4, 1, 128>(common, all_absolute);
}

nominal_diag_func check_AD_length(const data_definition_common& common, bool all_absolute) noexcept
{
    return check_A_AD_Y_length<1, 8, 2, 8, 1, 128>(common, all_absolute);
}

nominal_diag_func check_Y_length(const data_definition_common& common, bool all_absolute) noexcept
{
    return check_A_AD_Y_length<1, 2, 2, 2, 1, 16>(common, all_absolute);
}

data_def_type_AD::data_def_type_AD()
    : data_def_type_A_AD_Y(data_definition_type::A, 'D', context::doubleword, 8)
{}

data_def_type_Y::data_def_type_Y()
    : data_def_type_A_AD_Y(data_definition_type::Y, '\0', context::halfword, 2)
{}

//***************************   types S, SY   *****************************//

data_def_type_S_SY::data_def_type_S_SY(char extension, int size)
    : data_def_type(data_definition_type::S,
          extension,
          n_a(),
          modifier_bound { size, size },
          n_a(),
          n_a(),
          context::halfword,
          (unsigned long long)size,
          context::integer_type::undefined)
{}

data_def_type_S::data_def_type_S()
    : data_def_type_S_SY('\0', 2)
{}
// Checks S and SY operand, size specifies size of displacement in bits,
// is_signed specifies whether first bit is sign bit

data_def_type_SY::data_def_type_SY()
    : data_def_type_S_SY('Y', 3)
{}

//***************************   types R, RD   *****************************//

data_def_type_single_symbol::data_def_type_single_symbol(data_definition_type type,
    char extension,
    modifier_spec length_bound,
    context::alignment align,
    uint64_t implicit_length)
    : data_def_type(
          type, extension, n_a(), length_bound, n_a(), n_a(), align, implicit_length, context::integer_type::undefined)
{}

data_def_type_R::data_def_type_R()
    : data_def_type_single_symbol(data_definition_type::R, '\0', modifier_bound { 3, 4 }, context::fullword, 4)
{}

data_def_type_RD::data_def_type_RD()
    : data_def_type_single_symbol(data_definition_type::R, 'D', bound_list { 3, 4, 8 }, context::doubleword, 8)
{}

//***************************   types V, VD   *****************************//

data_def_type_V::data_def_type_V()
    : data_def_type_single_symbol(data_definition_type::V, '\0', modifier_bound { 3, 4 }, context::fullword, 4)
{}

data_def_type_VD::data_def_type_VD()
    : data_def_type_single_symbol(data_definition_type::V, 'D', bound_list { 3, 4, 8 }, context::doubleword, 8)
{}

//***************************   types Q, QD, QY   *****************************//

data_def_type_Q::data_def_type_Q()
    : data_def_type_single_symbol(data_definition_type::Q, '\0', modifier_bound { 1, 4 }, context::fullword, 4)
{}

data_def_type_QD::data_def_type_QD()
    : data_def_type_single_symbol(data_definition_type::Q, 'D', modifier_bound { 1, 8 }, context::quadword, 8)
{}

data_def_type_QY::data_def_type_QY()
    : data_def_type_single_symbol(data_definition_type::Q, 'Y', modifier_bound { 3, 3 }, context::halfword, 3)
{}


//***************************   types J, JD   *****************************//

data_def_type_J::data_def_type_J()
    : data_def_type_single_symbol(data_definition_type::J, '\0', bound_list { 2, 3, 4 }, context::fullword, 4)
{}

data_def_type_JD::data_def_type_JD()
    : data_def_type_single_symbol(data_definition_type::J, 'D', bound_list { 2, 3, 4, 8 }, context::doubleword, 8)
{}
} // namespace hlasm_plugin::parser_library::checking
