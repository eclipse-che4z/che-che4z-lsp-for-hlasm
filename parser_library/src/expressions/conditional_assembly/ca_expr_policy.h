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

#ifndef HLASMPLUGIN_PARSERLIBRARY_CA_EXPR_POLICY_H
#define HLASMPLUGIN_PARSERLIBRARY_CA_EXPR_POLICY_H

#include "context/common_types.h"
#include "context/id_storage.h"

namespace hlasm_plugin {
namespace parser_library {
namespace expressions {

enum class ca_expr_ops
{
    // arithmetic
    SLA,
    SLL,
    SRA,
    SRL,
    FIND,
    INDEX,

    // logical
    AND_NOT,
    OR_NOT,
    XOR_NOT,
    EQ,
    NE,
    LE,
    LT,
    GE,
    GT,

    // arithmetic & logical
    AND,
    OR,
    XOR,
    NOT,

    // character
    BYTE,
    DOUBLE,
    LOWER,
    SIGNED,
    UPPER,

    UNKNOWN
};

enum class ca_expr_funcs
{
    // arithmetic
    B2A,
    C2A,
    D2A,
    DCLEN,
    FIND,
    INDEX,
    ISBIN,
    ISDEC,
    ISHEX,
    ISSYM,

    // character
    A2B,
    A2C,
    A2D,
    A2X,
    B2C,
    B2D,
    B2X,
    BYTE,
    C2B,
    C2D,
    C2X,
    D2B,
    D2C,
    D2X,
    DCVAL,
    DEQUOTE,
    DOUBLE,
    ESYM,
    LOWER,
    SIGNED,
    SYSATTRA,
    SYSATTRP,
    UPPER,
    X2B,
    X2C,
    X2D,

    UNKNOWN
};

class ca_arithmetic_policy
{
public:
    static constexpr context::SET_t_enum set_type = context::SET_t_enum::A_TYPE;

    static bool is_unary(ca_expr_ops op);

    static bool is_binary(ca_expr_ops op);

    static bool multiple_words(ca_expr_ops op);

    static int get_priority(ca_expr_ops op);

    static bool is_operator(ca_expr_ops op);

    static bool is_function(ca_expr_funcs func);

    static ca_expr_ops get_operator(const std::string& symbol);

    static ca_expr_funcs get_function(const std::string& symbol);

    static std::pair<size_t, context::SET_t> get_function_param_info(ca_expr_funcs func);

    static context::SET_t get_operands_type(ca_expr_ops op);
};

class ca_binary_policy
{
public:
    static constexpr context::SET_t_enum set_type = context::SET_t_enum::B_TYPE;

    static bool is_unary(ca_expr_ops op);

    static bool is_binary(ca_expr_ops op);

    static bool multiple_words(ca_expr_ops op);

    static int get_priority(ca_expr_ops op);

    static bool is_operator(ca_expr_ops op);

    static bool is_function(ca_expr_funcs func);

    static ca_expr_ops get_operator(const std::string& symbol);

    static ca_expr_funcs get_function(const std::string& symbol);

    static std::pair<size_t, context::SET_t> get_function_param_info(ca_expr_funcs func);

    static context::SET_t get_operands_type(ca_expr_ops op);
};

class ca_character_policy
{
public:
    static constexpr context::SET_t_enum set_type = context::SET_t_enum::C_TYPE;

    static bool is_unary(ca_expr_ops op);

    static bool is_binary(ca_expr_ops op);

    static bool multiple_words(ca_expr_ops op);

    static int get_priority(ca_expr_ops op);

    static bool is_operator(ca_expr_ops op);

    static bool is_function(ca_expr_funcs func);

    static ca_expr_ops get_operator(const std::string& symbol);

    static ca_expr_funcs get_function(const std::string& symbol);

    static std::pair<size_t, context::SET_t_enum> get_function_param_info(ca_expr_funcs func);

    static context::SET_t_enum get_operands_type(ca_expr_ops op);
};

class ca_common_expr_policy
{
public:
    static std::pair<size_t, context::SET_t_enum> get_function_param_info(
        ca_expr_funcs func, context::SET_t_enum expr_kind);

    static context::SET_t_enum get_function_type(ca_expr_funcs func);

    static context::SET_t_enum get_operands_type(ca_expr_ops op, context::SET_t_enum expr_kind);
};

template<typename T> struct ca_expr_traits
{};

template<> struct ca_expr_traits<context::A_t>
{
    using policy_t = ca_arithmetic_policy;
};

template<> struct ca_expr_traits<context::B_t>
{
    using policy_t = ca_binary_policy;
};

template<> struct ca_expr_traits<context::C_t>
{
    using policy_t = ca_character_policy;
};

} // namespace expressions
} // namespace parser_library
} // namespace hlasm_plugin


#endif
