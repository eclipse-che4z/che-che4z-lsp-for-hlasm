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

#ifndef CONTEXT_COMMON_TYPES_H
#define CONTEXT_COMMON_TYPES_H

#include <string>

namespace hlasm_plugin {
namespace parser_library {
namespace context {

// type for SETA symbol
using A_t = int32_t;
// type for SETB symbol
using B_t = bool;
// type for SETC symbol
using C_t = std::string;

// enum of SET symbols
enum class SET_t_enum
{
    A_TYPE,
    B_TYPE,
    C_TYPE,
    UNDEF_TYPE
};

// enum of variable symbols
enum class variable_kind
{
    SET_VAR_KIND,
    MACRO_VAR_KIND
};

// enum of macro symbolic parameters
enum class macro_param_type
{
    POS_PAR_TYPE,
    KEY_PAR_TYPE,
    SYSTEM_TYPE
};

// helper traits structure for SET types
template<typename T> struct object_traits
{
    static constexpr SET_t_enum type_enum = SET_t_enum::UNDEF_TYPE;
};

template<> struct object_traits<A_t>
{
    static constexpr SET_t_enum type_enum = SET_t_enum::A_TYPE;
    static const A_t& default_v()
    {
        static A_t def = 0;
        return def;
    }
};

template<> struct object_traits<B_t>
{
    static constexpr SET_t_enum type_enum = SET_t_enum::B_TYPE;
    static const B_t& default_v()
    {
        static B_t def = false;
        return def;
    }
};

template<> struct object_traits<C_t>
{
    static constexpr SET_t_enum type_enum = SET_t_enum::C_TYPE;
    static const C_t& default_v()
    {
        static C_t def("");
        return def;
    }
};

// struct agregating SET types for easier usage
struct SET_t
{
private:
    A_t a_value;
    B_t b_value;
    C_t c_value;

public:
    SET_t(A_t value);
    SET_t(B_t value);
    SET_t(C_t value);
    SET_t();

    const SET_t_enum type;

    A_t& access_a();
    B_t& access_b();
    C_t& access_c();
};

// just mock method for now, will be implemented later with respect to UTF/EBCDIC
std::string& to_upper(std::string& s);
std::string to_upper_copy(std::string s);

} // namespace context
} // namespace parser_library
} // namespace hlasm_plugin
#endif
