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

#ifndef HLASMPLUGIN_PARSERLIBRARY_CHECKING_DATA_DEF_TYPES_H
#define HLASMPLUGIN_PARSERLIBRARY_CHECKING_DATA_DEF_TYPES_H

#include "data_def_type_base.h"
#include "data_definition_operand.h"

// This file defines all existing data_definition types, all implement the abstract class data_def_type.

namespace hlasm_plugin::parser_library::checking {
//************* string types *****************
class data_def_type_B final : public data_def_type
{
public:
    data_def_type_B();

    virtual bool check(const data_definition_operand& op,
        const diagnostic_collector& add_diagnostic,
        bool check_nominal) const override;

    virtual uint64_t get_nominal_length(const nominal_value_t& op) const override;
    virtual uint32_t get_nominal_length_attribute(const nominal_value_t& op) const override;
};

class data_def_type_CA_CE : public data_def_type
{
public:
    data_def_type_CA_CE(char extension);

    virtual uint64_t get_nominal_length(const nominal_value_t& op) const override;
    virtual uint32_t get_nominal_length_attribute(const nominal_value_t& op) const override;
};

class data_def_type_C final : public data_def_type_CA_CE
{
public:
    data_def_type_C();
};

class data_def_type_CA final : public data_def_type_CA_CE
{
public:
    data_def_type_CA();
};

class data_def_type_CE final : public data_def_type_CA_CE
{
public:
    data_def_type_CE();
};

class data_def_type_CU final : public data_def_type
{
public:
    data_def_type_CU();

    virtual bool check(const data_definition_operand& op,
        const diagnostic_collector& add_diagnostic,
        bool check_nominal) const override;

    virtual uint64_t get_nominal_length(const nominal_value_t& op) const override;
    virtual uint32_t get_nominal_length_attribute(const nominal_value_t& op) const override;
};

class data_def_type_G final : public data_def_type
{
public:
    data_def_type_G();

    virtual bool check(const data_definition_operand& op,
        const diagnostic_collector& add_diagnostic,
        bool check_nominal) const override;

    virtual uint64_t get_nominal_length(const nominal_value_t& op) const override;
    virtual uint32_t get_nominal_length_attribute(const nominal_value_t& op) const override;
};

class data_def_type_X final : public data_def_type
{
public:
    data_def_type_X();

    virtual bool check(const data_definition_operand& op,
        const diagnostic_collector& add_diagnostic,
        bool check_nominal) const override;

    virtual uint64_t get_nominal_length(const nominal_value_t& op) const override;
    virtual uint32_t get_nominal_length_attribute(const nominal_value_t& op) const override;
};

//************* fixed point types *****************


class data_def_type_H_F_FD : public data_def_type
{
public:
    data_def_type_H_F_FD(char type, char extension, uint8_t word_length);

    virtual bool check(const data_definition_operand& op,
        const diagnostic_collector& add_diagnostic,
        bool check_nominal) const override;

protected:
    virtual int32_t get_integer_attribute_impl(uint32_t length, int32_t scale) const override;
};

class data_def_type_H final : public data_def_type_H_F_FD
{
public:
    data_def_type_H();
};

class data_def_type_F final : public data_def_type_H_F_FD
{
public:
    data_def_type_F();
};

class data_def_type_FD final : public data_def_type_H_F_FD
{
public:
    data_def_type_FD();
};

class data_def_type_P_Z : public data_def_type
{
public:
    data_def_type_P_Z(char type);

    virtual bool check(const data_definition_operand& op,
        const diagnostic_collector& add_diagnostic,
        bool check_nominal) const override;
    virtual int16_t get_implicit_scale(const nominal_value_t& op) const override;
};

class data_def_type_P final : public data_def_type_P_Z
{
public:
    data_def_type_P();

protected:
    virtual uint64_t get_nominal_length(const nominal_value_t& op) const override;
    virtual uint32_t get_nominal_length_attribute(const nominal_value_t& op) const override;


    virtual int32_t get_integer_attribute_impl(uint32_t length, int32_t scale) const override;
};

class data_def_type_Z final : public data_def_type_P_Z
{
public:
    data_def_type_Z();

protected:
    virtual uint64_t get_nominal_length(const nominal_value_t& op) const override;
    virtual uint32_t get_nominal_length_attribute(const nominal_value_t& op) const override;
    virtual int32_t get_integer_attribute_impl(uint32_t length, int32_t scale) const override;
};

//************* address types *****************

class data_def_type_A_AD_Y : public data_def_type
{
public:
    data_def_type_A_AD_Y(char type, char extension, context::alignment implicit_alignment, uint64_t implicit_length);
};

class data_def_type_A final : public data_def_type_A_AD_Y
{
public:
    data_def_type_A();

    virtual bool check(const data_definition_operand& op,
        const diagnostic_collector& add_diagnostic,
        bool check_nominal) const override;
};

class data_def_type_AD final : public data_def_type_A_AD_Y
{
public:
    data_def_type_AD();

    virtual bool check(const data_definition_operand& op,
        const diagnostic_collector& add_diagnostic,
        bool check_nominal) const override;
};

class data_def_type_Y final : public data_def_type_A_AD_Y
{
public:
    data_def_type_Y();

    virtual bool check(const data_definition_operand& op,
        const diagnostic_collector& add_diagnostic,
        bool check_nominal) const override;
};



class data_def_type_S_SY : public data_def_type
{
public:
    data_def_type_S_SY(char extension, int size);
};

class data_def_type_S final : public data_def_type_S_SY
{
public:
    data_def_type_S();

    virtual bool check(const data_definition_operand& op,
        const diagnostic_collector& add_diagnostic,
        bool check_nominal) const override;
};

class data_def_type_SY final : public data_def_type_S_SY
{
public:
    data_def_type_SY();

    virtual bool check(const data_definition_operand& op,
        const diagnostic_collector& add_diagnostic,
        bool check_nominal) const override;
};

//************* single symbol types *****************

class data_def_type_single_symbol : public data_def_type
{
public:
    data_def_type_single_symbol(char type,
        char extension,
        modifier_spec length_bound,
        context::alignment implicit_alignment,
        uint64_t implicit_length);

    virtual bool expects_single_symbol() const override;
};

class data_def_type_R final : public data_def_type_single_symbol
{
public:
    data_def_type_R();
};

class data_def_type_RD final : public data_def_type_single_symbol
{
public:
    data_def_type_RD();

    virtual bool check(const data_definition_operand& op,
        const diagnostic_collector& add_diagnostic,
        bool check_nominal) const override;
};

class data_def_type_V final : public data_def_type_single_symbol
{
public:
    data_def_type_V();
};

class data_def_type_VD final : public data_def_type_single_symbol
{
public:
    data_def_type_VD();

    virtual bool check(const data_definition_operand& op,
        const diagnostic_collector& add_diagnostic,
        bool check_nominal) const override;
};

class data_def_type_Q final : public data_def_type_single_symbol
{
public:
    data_def_type_Q();
};

class data_def_type_QD final : public data_def_type_single_symbol
{
public:
    data_def_type_QD();
};

class data_def_type_QY final : public data_def_type_single_symbol
{
public:
    data_def_type_QY();
};

class data_def_type_J final : public data_def_type_single_symbol
{
public:
    data_def_type_J();

    virtual bool check(const data_definition_operand& op,
        const diagnostic_collector& add_diagnostic,
        bool check_nominal) const override;
};

class data_def_type_JD final : public data_def_type_single_symbol
{
public:
    data_def_type_JD();

    virtual bool check(const data_definition_operand& op,
        const diagnostic_collector& add_diagnostic,
        bool check_nominal) const override;
};

//************* floating point types *****************

class data_def_type_E_D_L : public data_def_type
{
public:
    data_def_type_E_D_L(char type,
        char extension,
        modifier_spec big_length_spec,
        modifier_spec length_spec,
        modifier_spec scale_spec,
        context::alignment align,
        uint64_t implicit_length);

    virtual bool check(const data_definition_operand& op,
        const diagnostic_collector& add_diagnostic,
        bool check_nominal) const override;

protected:
    virtual int32_t get_integer_attribute_impl(uint32_t length, int32_t scale) const override;
};

class data_def_type_E final : public data_def_type_E_D_L
{
public:
    data_def_type_E();
};

class data_def_type_EH final : public data_def_type_E_D_L
{
public:
    data_def_type_EH();
};

class data_def_type_ED final : public data_def_type_E_D_L
{
public:
    data_def_type_ED();
};

class data_def_type_EB final : public data_def_type_E_D_L
{
public:
    data_def_type_EB();
};

class data_def_type_D final : public data_def_type_E_D_L
{
public:
    data_def_type_D();
};

class data_def_type_DH final : public data_def_type_E_D_L
{
public:
    data_def_type_DH();
};

class data_def_type_DB final : public data_def_type_E_D_L
{
public:
    data_def_type_DB();
};

class data_def_type_DD final : public data_def_type_E_D_L
{
public:
    data_def_type_DD();
};

class data_def_type_L final : public data_def_type_E_D_L
{
public:
    data_def_type_L();
};

class data_def_type_LH final : public data_def_type_E_D_L
{
public:
    data_def_type_LH();
};

class data_def_type_LQ final : public data_def_type_E_D_L
{
public:
    data_def_type_LQ();
};

class data_def_type_LD final : public data_def_type_E_D_L
{
public:
    data_def_type_LD();
};

class data_def_type_LB final : public data_def_type_E_D_L
{
public:
    data_def_type_LB();
};

} // namespace hlasm_plugin::parser_library::checking


#endif