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
// floating point types: E, D, L

#include "checking/checker_helper.h"
#include "data_def_types.h"

using namespace hlasm_plugin::parser_library::checking;
using namespace hlasm_plugin::parser_library::context;
using namespace hlasm_plugin::parser_library;

data_def_type_E_D_L::data_def_type_E_D_L(char type,
    char extension,
    modifier_spec bit_length_spec,
    modifier_spec length_spec,
    modifier_spec scale_spec,
    alignment align,
    uint64_t implicit_length)
    : data_def_type(type,
        extension,
        bit_length_spec,
        length_spec,
        scale_spec,
        modifier_bound { -85, 75 },
        nominal_value_type::STRING,
        align,
        implicit_length)
{}

std::map<char, std::set<std::string>> allowed_round_modes = {
    { 'B', { "1", "4", "5", "6", "7" } },
    { 'H', { "1", "4", "5", "6", "7" } },
    { 'D', { "8", "9", "10", "11", "12", "13", "14", "15" } },
};

class E_D_L_number_spec
{
public:
    static bool is_end_char(char c) { return c == ',' || c == 'E' || c == 'R'; }

    static bool is_sign_char(char c) { return c == '+' || c == '-'; }
};

bool data_def_type_E_D_L::check(
    const data_definition_operand& op, const diagnostic_collector& add_diagnostic, bool check_nominal) const
{
    if (!check_nominal)
        return true;

    size_t i = 0;
    const std::string& nom = std::get<std::string>(op.nominal_value.value);
    if (nom.size() == 0)
    {
        add_diagnostic(diagnostic_op::error_D010(op.nominal_value.rng, type_str));
        return false;
    }
    else if (nom.back() == ',')
    {
        add_diagnostic(diagnostic_op::error_D010(op.nominal_value.rng, type_str));
        return false;
    }
    while (i < nom.size())
    {
        // the number may end with E, R or ',' and begin with + or -.
        if (!check_number<E_D_L_number_spec>(nom, i))
        {
            add_diagnostic(diagnostic_op::error_D010(op.nominal_value.rng, type_str));
            return false;
        }

        // After E comes exponent
        if (nom[i] == 'E')
        {
            if (!check_exponent(nom, i))
            {
                add_diagnostic(diagnostic_op::error_D010(op.nominal_value.rng, type_str));
                return false;
            }
            if (i >= nom.size())
                return true;
        }

        // After R comes rounding mode
        if (nom[i] == 'R')
        {
            ++i;
            std::string round_mode_s;
            while (i < nom.size() && is_digit(nom[i]))
            {
                round_mode_s += nom[i];
                ++i;
            }

            if (allowed_round_modes[extension].find(round_mode_s) == allowed_round_modes[extension].end())
            {
                add_diagnostic(diagnostic_op::error_D026(op.nominal_value.rng));
                return false;
            }
        }
        if (i < nom.size() && nom[i] != ',')
        {
            add_diagnostic(diagnostic_op::error_D010(op.nominal_value.rng, type_str));
            return false;
        }
        ++i;
    }

    return true;
}

int32_t data_def_type_E_D_L::get_integer_attribute_impl(uint32_t length, int32_t scale) const
{
    // Decimal and binary floating point constants have integer attribute 0
    if (extension == 'D' || extension == 'B')
        return 0;
    else if (length > 8)
        return 2 * (length - 1) - scale - 2;
    else
        return 2 * (length - 1) - scale;
}

data_def_type_E::data_def_type_E()
    : data_def_type_E_D_L(
        'E', '\0', modifier_bound { 1, 64 }, modifier_bound { 1, 8 }, modifier_bound { 0, 5 }, fullword, 4)
{}

data_def_type_EH::data_def_type_EH()
    : data_def_type_E_D_L(
        'E', 'H', modifier_bound { 12, 64 }, modifier_bound { 1, 8 }, modifier_bound { 0, 5 }, fullword, 4)
{}

data_def_type_ED::data_def_type_ED()
    : data_def_type_E_D_L('E', 'D', modifier_bound { 32, 32 }, modifier_bound { 4, 4 }, ignored(), fullword, 4)
{}

data_def_type_EB::data_def_type_EB()
    : data_def_type_E_D_L('E', 'B', modifier_bound { 32, 32 }, modifier_bound { 4, 4 }, ignored(), fullword, 4)
{}

data_def_type_D::data_def_type_D()
    : data_def_type_E_D_L(
        'D', '\0', modifier_bound { 1, 64 }, modifier_bound { 1, 8 }, modifier_bound { 0, 13 }, doubleword, 8)
{}

data_def_type_DH::data_def_type_DH()
    : data_def_type_E_D_L(
        'D', 'H', modifier_bound { 12, 64 }, modifier_bound { 1, 8 }, modifier_bound { 0, 13 }, doubleword, 8)
{}

data_def_type_DB::data_def_type_DB()
    : data_def_type_E_D_L('D', 'B', modifier_bound { 64, 64 }, modifier_bound { 8, 8 }, ignored(), doubleword, 8)
{}

data_def_type_DD::data_def_type_DD()
    : data_def_type_E_D_L('D', 'D', modifier_bound { 64, 64 }, modifier_bound { 8, 8 }, ignored(), doubleword, 8)
{}

data_def_type_L::data_def_type_L()
    : data_def_type_E_D_L(
        'L', '\0', modifier_bound { 1, 128 }, modifier_bound { 1, 16 }, modifier_bound { 0, 27 }, doubleword, 16)
{}

data_def_type_LH::data_def_type_LH()
    : data_def_type_E_D_L(
        'L', 'H', modifier_bound { 12, 128 }, modifier_bound { 1, 16 }, modifier_bound { 0, 27 }, doubleword, 16)
{}

data_def_type_LQ::data_def_type_LQ()
    : data_def_type_E_D_L(
        'L', 'Q', modifier_bound { 12, 128 }, modifier_bound { 1, 16 }, modifier_bound { 0, 27 }, quadword, 16)
{}

data_def_type_LD::data_def_type_LD()
    : data_def_type_E_D_L('L', 'D', modifier_bound { 128, 128 }, modifier_bound { 16, 16 }, ignored(), doubleword, 16)
{}

data_def_type_LB::data_def_type_LB()
    : data_def_type_E_D_L('L', 'B', modifier_bound { 128, 128 }, modifier_bound { 16, 16 }, ignored(), doubleword, 16)
{}