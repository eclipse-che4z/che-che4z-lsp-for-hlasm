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

#include <variant>

#include "context/common_types.h"

// the file contains policy classes for retrieving properties
// of built-in functions and function operators

namespace hlasm_plugin::parser_library::expressions {

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

struct ca_expr_op
{
    ca_expr_ops op;
    int priority;
    bool binary;
    bool right_assoc;
};

struct invalid_by_policy
{};

enum class ca_expr_funcs
{
    // arithmetic
    B2A,
    C2A,
    D2A,
    DCLEN,
    FIND,
    INDEX,
    X2A,

    // binary
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

// policy class for arithmetic functions and operators
class ca_arithmetic_policy
{
public:
    static constexpr context::SET_t_enum set_type = context::SET_t_enum::A_TYPE;

    // is unary arithmetic operation
    static bool is_unary(ca_expr_ops op);

    // is binary arithmetic operation
    static bool is_binary(ca_expr_ops op);

    // get priority relative to rest of arithmetic operators
    static int get_priority(ca_expr_ops op);

    // is arithmetic operator
    static bool is_operator(ca_expr_ops op);

    // is arithmetic function
    static bool is_function(ca_expr_funcs func);

    // transforms string operator to enum
    static ca_expr_ops get_operator(std::string_view symbol);

    static std::variant<std::monostate, invalid_by_policy, ca_expr_op> get_operator_properties(std::string_view symbol);

    // transforms string function to enum
    static ca_expr_funcs get_function(std::string_view symbol);

    // return number of required parameters and return type
    static std::pair<size_t, context::SET_t_enum> get_function_param_info(ca_expr_funcs func);

    // get operand types of operator op
    static context::SET_t_enum get_operands_type(ca_expr_ops op);
};

// policy class for binary functions and operators
class ca_binary_policy
{
public:
    static constexpr context::SET_t_enum set_type = context::SET_t_enum::B_TYPE;

    // is unary binary operation
    static bool is_unary(ca_expr_ops op);

    // is binary binary operation
    static bool is_binary(ca_expr_ops op);

    // get priority relative to rest of binary operators
    static int get_priority(ca_expr_ops op);

    // is binary operator
    static bool is_operator(ca_expr_ops op);

    // is binary function
    static bool is_function(ca_expr_funcs func);

    // transforms string operator to enum
    static ca_expr_ops get_operator(std::string_view symbol);

    static std::variant<std::monostate, invalid_by_policy, ca_expr_op> get_operator_properties(std::string_view symbol);

    // transforms string function to enum
    static ca_expr_funcs get_function(std::string_view symbol);

    // return number of required parameters and return type
    static std::pair<size_t, context::SET_t_enum> get_function_param_info(ca_expr_funcs func);

    // get operand types of operator op
    static context::SET_t_enum get_operands_type(ca_expr_ops op);
};

// policy class for character functions and operators
class ca_character_policy
{
public:
    static constexpr context::SET_t_enum set_type = context::SET_t_enum::C_TYPE;

    // is unary character operation
    static bool is_unary(ca_expr_ops op);

    // is binary character operation
    static bool is_binary(ca_expr_ops op);

    // get priority relative to rest of character operators
    static int get_priority(ca_expr_ops op);

    // is character operator
    static bool is_operator(ca_expr_ops op);

    // is character function
    static bool is_function(ca_expr_funcs func);

    // transforms string operator to enum
    static ca_expr_ops get_operator(std::string_view symbol);

    static std::variant<std::monostate, invalid_by_policy, ca_expr_op> get_operator_properties(std::string_view symbol);

    // transforms string function to enum
    static ca_expr_funcs get_function(std::string_view symbol);

    // return number of required parameters and return type
    static std::pair<size_t, context::SET_t_enum> get_function_param_info(ca_expr_funcs func);

    // get operand types of operator op
    static context::SET_t_enum get_operands_type(ca_expr_ops op);
};

// policy class that aggregates some methods of specific policy classes above
class ca_common_expr_policy
{
public:
    static std::pair<size_t, context::SET_t_enum> get_function_param_info(
        ca_expr_funcs func, context::SET_t_enum expr_kind);

    static context::SET_t_enum get_function_type(ca_expr_funcs func);

    static context::SET_t_enum get_operands_type(ca_expr_ops op, context::SET_t_enum expr_kind);

    static ca_expr_funcs get_function(std::string_view symbol);

    static constexpr size_t max_function_name_length = 8;
};

template<typename T>
struct ca_expr_traits
{};

template<>
struct ca_expr_traits<context::A_t>
{
    using policy_t = ca_arithmetic_policy;
};

template<>
struct ca_expr_traits<context::B_t>
{
    using policy_t = ca_binary_policy;
};

template<>
struct ca_expr_traits<context::C_t>
{
    using policy_t = ca_character_policy;
};

} // namespace hlasm_plugin::parser_library::expressions


#endif
