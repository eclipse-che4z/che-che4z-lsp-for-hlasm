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

#include <cstdint>
#include <string>

namespace hlasm_plugin::parser_library::context {

// type for SETA symbol
using A_t = int32_t;
// type for SETB symbol
using B_t = bool;
// type for SETC symbol
using C_t = std::string;

// enum of SET symbols
enum class SET_t_enum : unsigned char
{
    A_TYPE,
    B_TYPE,
    C_TYPE,
    UNDEF_TYPE
};

// enum of variable symbols
enum class variable_kind : unsigned char
{
    SET_VAR_KIND,
    MACRO_VAR_KIND
};

// enum of macro symbolic parameters
enum class macro_param_type : unsigned char
{
    POS_PAR_TYPE,
    KEY_PAR_TYPE,
    SYSTEM_TYPE
};

// helper traits structure for SET types
template<typename T>
struct object_traits
{
    static constexpr SET_t_enum type_enum = SET_t_enum::UNDEF_TYPE;
};

template<>
struct object_traits<A_t>
{
    static constexpr SET_t_enum type_enum = SET_t_enum::A_TYPE;
    static constexpr A_t default_v() noexcept { return 0; }
};

template<>
struct object_traits<B_t>
{
    static constexpr SET_t_enum type_enum = SET_t_enum::B_TYPE;
    static constexpr B_t default_v() noexcept { return false; }
};

template<>
struct object_traits<C_t>
{
    static constexpr SET_t_enum type_enum = SET_t_enum::C_TYPE;
    static C_t default_v() noexcept { return C_t(); } // llvm-14 - constexpr
};

// struct aggregating SET types for easier usage
struct SET_t
{
private:
    C_t c_value = {};
    A_t a_value = 0;
    SET_t_enum value_type;

public:
    SET_t(A_t value) noexcept
        : a_value(value)
        , value_type(SET_t_enum::A_TYPE)
    {}

    SET_t(B_t value) noexcept
        : a_value(static_cast<A_t>(value))
        , value_type(SET_t_enum::B_TYPE)
    {}

    SET_t(C_t value) noexcept
        : c_value(std::move(value))
        , value_type(SET_t_enum::C_TYPE)
    {}

    SET_t(const char* value)
        : c_value(value)
        , value_type(SET_t_enum::C_TYPE)
    {}

    SET_t(SET_t_enum type = SET_t_enum::UNDEF_TYPE) noexcept
        : value_type(type)
    {}

    SET_t_enum type() const noexcept { return value_type; }

    A_t access_a() const noexcept { return a_value; }
    B_t access_b() const noexcept { return !!a_value; }
    C_t& access_c() noexcept { return c_value; }
    const C_t& access_c() const noexcept { return c_value; }

    bool operator==(const SET_t& r) const noexcept;
};

} // namespace hlasm_plugin::parser_library::context
#endif
