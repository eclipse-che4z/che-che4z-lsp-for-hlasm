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

#ifndef CONTEXT_SYMBOL_ATTRIBUTES_H
#define CONTEXT_SYMBOL_ATTRIBUTES_H

#include <cstdint>

#include "context/common_types.h"

namespace hlasm_plugin::parser_library::context {

// enumeration of all data attributes
enum class data_attr_kind : unsigned char
{
    UNKNOWN,
    T,
    L,
    S,
    I,
    K,
    N,
    D,
    O,

    max = O,
};

// tells how symbol is created
// whether it is section definition, machine label, equated, data definition symbol, or created by some other ASM
// instruction
enum class symbol_origin : unsigned char
{
    SECT,
    MACH,
    EQU,
    DAT,
    ASM,
    UNKNOWN
};

// structure wrapping attributes of the symbol
// the structure fields are to be constant except undefined fields, their value can be defined later
struct symbol_attributes
{
    using value_t = int32_t;
    using type_attr = uint16_t;
    using len_attr = uint32_t;
    using scale_attr = int16_t;

    // static field describing undefined states of attributes
    static const type_attr undef_type;
    static const len_attr undef_length;
    static const scale_attr undef_scale;

    // predefined symbol_attributes classes
    static symbol_attributes make_section_attrs();
    static symbol_attributes make_machine_attrs(len_attr);
    static symbol_attributes make_extrn_attrs();
    static symbol_attributes make_wxtrn_attrs();
    static symbol_attributes make_org_attrs();
    static symbol_attributes make_ccw_attrs();
    static symbol_attributes make_cnop_attrs();

    // helper function to transform char to enum
    static data_attr_kind transform_attr(unsigned char c);

    static bool requires_ordinary_symbol(data_attr_kind attribute);
    static bool is_ordinary_attribute(data_attr_kind attribute);
    static value_t default_value(data_attr_kind attribute);
    static SET_t default_ca_value(data_attr_kind attribute);

    symbol_attributes(symbol_origin origin,
        type_attr type,
        len_attr length = undef_length,
        scale_attr scale = undef_scale,
        len_attr integer = undef_length);
    symbol_attributes(symbol_origin origin);

    symbol_origin origin() const { return origin_; }
    type_attr type() const { return type_; }
    len_attr length() const { return length_; }
    scale_attr scale() const { return scale_; }
    len_attr integer() const { return integer_; }

    bool is_defined(data_attr_kind attribute) const;

    bool can_have_SI_attr() const;

    value_t get_attribute_value(data_attr_kind attribute) const;

    // sets length if undefined
    void length(len_attr value);
    // sets scale if undefined
    void scale(scale_attr value);


private:
    len_attr length_;
    len_attr integer_;
    type_attr type_;
    scale_attr scale_;
    symbol_origin origin_;
};

} // namespace hlasm_plugin::parser_library::context
#endif
