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

namespace hlasm_plugin {
namespace parser_library {
namespace context {

// enumeration of all data attributes
enum class data_attr_kind
{
    T,
    L,
    S,
    I,
    K,
    N,
    D,
    O,
    UNKNOWN
};

// tells how symbol is created
// whether it is section definition, machine label, equated or data definition symbol
enum class symbol_origin
{
    SECT,
    MACH,
    EQU,
    DAT
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
    static symbol_attributes make_org_attrs();

    // helper function to transform char to enum
    static data_attr_kind transform_attr(char c);

    static bool needs_ordinary(data_attr_kind attribute);
    static bool ordinary_allowed(data_attr_kind attribute);
    static value_t default_value(data_attr_kind attribute);

    symbol_attributes(symbol_origin origin,
        type_attr type,
        len_attr length = undef_length,
        scale_attr scale = undef_scale,
        len_attr integer = undef_length);
    symbol_attributes(symbol_origin origin);

    type_attr type() const;
    len_attr length() const;
    scale_attr scale() const;
    len_attr integer() const;

    bool is_defined(data_attr_kind attribute) const;

    bool can_have_SI_attr() const;

    value_t get_attribute_value(data_attr_kind attribute) const;

    // sets length if undefined
    void length(len_attr value);
    // sets scale if undefined
    void scale(scale_attr value);

    const symbol_origin origin;

private:
    type_attr type_;
    len_attr length_;
    scale_attr scale_;
    len_attr integer_;
};

} // namespace context
} // namespace parser_library
} // namespace hlasm_plugin
#endif
