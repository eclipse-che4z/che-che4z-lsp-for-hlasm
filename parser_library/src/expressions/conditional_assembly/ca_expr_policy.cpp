#include "ca_expr_policy.h"
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


#include "ca_expr_policy.h"

namespace hlasm_plugin {
namespace parser_library {
namespace expressions {

bool ca_arithmetic_policy::is_unary(ca_expr_ops op) { return op == ca_expr_ops::NOT; }
bool ca_binary_policy::is_unary(ca_expr_ops op) { return op == ca_expr_ops::NOT; }
bool ca_character_policy::is_unary(ca_expr_ops op)
{
    return op == ca_expr_ops::BYTE || op == ca_expr_ops::DOUBLE || op == ca_expr_ops::LOWER || op == ca_expr_ops::SIGNED
        || op == ca_expr_ops::UPPER;
}

bool ca_arithmetic_policy::multiple_words(ca_expr_ops) { return false; }
bool ca_binary_policy::multiple_words(ca_expr_ops op)
{
    return op == ca_expr_ops::AND || op == ca_expr_ops::OR || op == ca_expr_ops::XOR;
}
bool ca_character_policy::multiple_words(ca_expr_ops) { return false; }

bool ca_arithmetic_policy::is_binary(ca_expr_ops op)
{
    return op == ca_expr_ops::SLA || op == ca_expr_ops::SLL || op == ca_expr_ops::SRA || op == ca_expr_ops::SRL
        || op == ca_expr_ops::FIND || op == ca_expr_ops::INDEX || op == ca_expr_ops::AND || op == ca_expr_ops::OR
        || op == ca_expr_ops::XOR;
}

bool ca_binary_policy::is_binary(ca_expr_ops op)
{
    return op == ca_expr_ops::EQ || op == ca_expr_ops::NE || op == ca_expr_ops::LE || op == ca_expr_ops::LT
        || op == ca_expr_ops::GE || op == ca_expr_ops::GT || op == ca_expr_ops::AND || op == ca_expr_ops::OR
        || op == ca_expr_ops::XOR || op == ca_expr_ops::AND_NOT || op == ca_expr_ops::OR_NOT
        || op == ca_expr_ops::XOR_NOT;
}

bool ca_character_policy::is_binary(ca_expr_ops) { return false; }

bool ca_arithmetic_policy::is_operator(ca_expr_ops op) { return is_unary(op) || is_binary(op); };
bool ca_binary_policy::is_operator(ca_expr_ops op) { return is_unary(op) || is_binary(op); };
bool ca_character_policy::is_operator(ca_expr_ops op) { return is_unary(op) || is_binary(op); };

bool ca_arithmetic_policy::is_function(ca_expr_funcs func)
{
    switch (func)
    {
        case ca_expr_funcs::B2A:
        case ca_expr_funcs::C2A:
        case ca_expr_funcs::D2A:
        case ca_expr_funcs::DCLEN:
        case ca_expr_funcs::FIND:
        case ca_expr_funcs::INDEX:
        case ca_expr_funcs::ISBIN:
        case ca_expr_funcs::ISDEC:
        case ca_expr_funcs::ISHEX:
        case ca_expr_funcs::ISSYM:
        case ca_expr_funcs::X2A:
            return true;
        default:
            return false;
    }
};
bool ca_binary_policy::is_function(ca_expr_funcs) { return false; };
bool ca_character_policy::is_function(ca_expr_funcs func)
{
    switch (func)
    {
        case ca_expr_funcs::A2B:
        case ca_expr_funcs::A2C:
        case ca_expr_funcs::A2D:
        case ca_expr_funcs::A2X:
        case ca_expr_funcs::B2C:
        case ca_expr_funcs::B2D:
        case ca_expr_funcs::B2X:
        case ca_expr_funcs::BYTE:
        case ca_expr_funcs::C2B:
        case ca_expr_funcs::C2D:
        case ca_expr_funcs::C2X:
        case ca_expr_funcs::D2B:
        case ca_expr_funcs::D2C:
        case ca_expr_funcs::D2X:
        case ca_expr_funcs::DCVAL:
        case ca_expr_funcs::DEQUOTE:
        case ca_expr_funcs::DOUBLE:
        case ca_expr_funcs::ESYM:
        case ca_expr_funcs::LOWER:
        case ca_expr_funcs::SIGNED:
        case ca_expr_funcs::SYSATTRA:
        case ca_expr_funcs::SYSATTRP:
        case ca_expr_funcs::UPPER:
        case ca_expr_funcs::X2B:
        case ca_expr_funcs::X2C:
        case ca_expr_funcs::X2D:
            return true;
        default:
            return false;
    }
};

int ca_arithmetic_policy::get_priority(ca_expr_ops op)
{
    switch (op)
    {
        case ca_expr_ops::FIND:
        case ca_expr_ops::INDEX:
            return 0;
        case ca_expr_ops::NOT:
            return 1;
        case ca_expr_ops::AND:
        case ca_expr_ops::OR:
        case ca_expr_ops::XOR:
            return 2;
        case ca_expr_ops::SLA:
        case ca_expr_ops::SLL:
        case ca_expr_ops::SRA:
        case ca_expr_ops::SRL:
            return 3;
        default:
            return 0;
    }
}

int ca_binary_policy::get_priority(ca_expr_ops op)
{
    switch (op)
    {
        case ca_expr_ops::EQ:
        case ca_expr_ops::NE:
        case ca_expr_ops::LE:
        case ca_expr_ops::LT:
        case ca_expr_ops::GE:
        case ca_expr_ops::GT:
            return 0;
        case ca_expr_ops::NOT:
            return 1;
        case ca_expr_ops::AND:
        case ca_expr_ops::AND_NOT:
            return 2;
        case ca_expr_ops::OR:
        case ca_expr_ops::OR_NOT:
            return 3;
        case ca_expr_ops::XOR:
        case ca_expr_ops::XOR_NOT:
            return 4;
        default:
            return 0;
    }
}

int ca_character_policy::get_priority(ca_expr_ops) { return 0; }

std::pair<size_t, context::SET_t_enum> ca_arithmetic_policy::get_function_param_info(ca_expr_funcs func)
{
    if (func == ca_expr_funcs::FIND || func == ca_expr_funcs::INDEX)
        return std::make_pair(2, context::SET_t_enum::C_TYPE);
    else if (is_function(func))
        return std::make_pair(1, context::SET_t_enum::C_TYPE);
    else
        return std::make_pair(0, context::SET_t_enum::UNDEF_TYPE);
}
std::pair<size_t, context::SET_t_enum> ca_binary_policy::get_function_param_info(ca_expr_funcs)
{
    return std::make_pair(0, context::SET_t_enum::UNDEF_TYPE);
}
std::pair<size_t, context::SET_t_enum> ca_character_policy::get_function_param_info(ca_expr_funcs func)
{
    switch (func)
    {
        case ca_expr_funcs::A2B:
        case ca_expr_funcs::A2C:
        case ca_expr_funcs::A2D:
        case ca_expr_funcs::A2X:
        case ca_expr_funcs::BYTE:
        case ca_expr_funcs::SIGNED:
            return std::make_pair(1, context::SET_t_enum::A_TYPE);
        case ca_expr_funcs::B2C:
        case ca_expr_funcs::B2D:
        case ca_expr_funcs::B2X:
        case ca_expr_funcs::C2B:
        case ca_expr_funcs::C2D:
        case ca_expr_funcs::C2X:
        case ca_expr_funcs::D2B:
        case ca_expr_funcs::D2C:
        case ca_expr_funcs::D2X:
        case ca_expr_funcs::DCVAL:
        case ca_expr_funcs::DEQUOTE:
        case ca_expr_funcs::DOUBLE:
        case ca_expr_funcs::ESYM:
        case ca_expr_funcs::LOWER:
        case ca_expr_funcs::SYSATTRA:
        case ca_expr_funcs::SYSATTRP:
        case ca_expr_funcs::UPPER:
        case ca_expr_funcs::X2A:
        case ca_expr_funcs::X2B:
        case ca_expr_funcs::X2C:
        case ca_expr_funcs::X2D:
            return std::make_pair(1, context::SET_t_enum::C_TYPE);
        default:
            return std::make_pair(0, context::SET_t_enum::UNDEF_TYPE);
    }
}

context::SET_t_enum ca_arithmetic_policy::get_operands_type(ca_expr_ops op)
{
    switch (op)
    {
        case ca_expr_ops::FIND:
        case ca_expr_ops::INDEX:
            return context::SET_t_enum::C_TYPE;
        case ca_expr_ops::NOT:
        case ca_expr_ops::AND:
        case ca_expr_ops::OR:
        case ca_expr_ops::XOR:
        case ca_expr_ops::SLA:
        case ca_expr_ops::SLL:
        case ca_expr_ops::SRA:
        case ca_expr_ops::SRL:
            return context::SET_t_enum::A_TYPE;
        default:
            return context::SET_t_enum::UNDEF_TYPE;
    }
}

context::SET_t_enum ca_binary_policy::get_operands_type(ca_expr_ops op)
{
    if (is_operator(op))
        return context::SET_t_enum::B_TYPE;
    else
        return context::SET_t_enum::UNDEF_TYPE;
}

context::SET_t_enum ca_character_policy::get_operands_type(ca_expr_ops op)
{
    switch (op)
    {
        case ca_expr_ops::BYTE:
        case ca_expr_ops::SIGNED:
            return context::SET_t_enum::A_TYPE;
        case ca_expr_ops::DOUBLE:
        case ca_expr_ops::LOWER:
        case ca_expr_ops::UPPER:
            return context::SET_t_enum::C_TYPE;
        default:
            return context::SET_t_enum::UNDEF_TYPE;
    }
}

// string to op
#define S2O(X)                                                                                                         \
    if (op == #X)                                                                                                      \
    return ca_expr_ops::X

ca_expr_ops get_expr_operator(const std::string& op)
{
    S2O(SLA);
    S2O(SLL);
    S2O(SRA);
    S2O(SRL);
    S2O(FIND);
    S2O(INDEX);
    S2O(AND_NOT);
    S2O(OR_NOT);
    S2O(XOR_NOT);
    S2O(EQ);
    S2O(NE);
    S2O(LE);
    S2O(LT);
    S2O(GE);
    S2O(GT);
    S2O(AND);
    S2O(OR);
    S2O(XOR);
    S2O(NOT);
    S2O(BYTE);
    S2O(DOUBLE);
    S2O(LOWER);
    S2O(SIGNED);
    S2O(UPPER);

    return ca_expr_ops::UNKNOWN;
}

ca_expr_ops ca_arithmetic_policy::get_operator(const std::string& symbol) { return get_expr_operator(symbol); }
ca_expr_ops ca_binary_policy::get_operator(const std::string& symbol) { return get_expr_operator(symbol); }
ca_expr_ops ca_character_policy::get_operator(const std::string& symbol) { return get_expr_operator(symbol); }

ca_expr_funcs ca_arithmetic_policy::get_function(const std::string& symbol)
{
    return ca_common_expr_policy::get_function(symbol);
}
ca_expr_funcs ca_binary_policy::get_function(const std::string& symbol)
{
    return ca_common_expr_policy::get_function(symbol);
}
ca_expr_funcs ca_character_policy::get_function(const std::string& symbol)
{
    return ca_common_expr_policy::get_function(symbol);
}

std::pair<size_t, context::SET_t_enum> ca_common_expr_policy::get_function_param_info(
    ca_expr_funcs func, context::SET_t_enum expr_kind)
{
    switch (expr_kind)
    {
        case context::SET_t_enum::A_TYPE:
            return ca_arithmetic_policy::get_function_param_info(func);
        case context::SET_t_enum::B_TYPE:
            return ca_binary_policy::get_function_param_info(func);
        case context::SET_t_enum::C_TYPE:
            return ca_character_policy::get_function_param_info(func);
        default:
            return std::make_pair(0, context::SET_t_enum::UNDEF_TYPE);
    }
}

context::SET_t_enum ca_common_expr_policy::get_function_type(ca_expr_funcs func)
{
    if (ca_arithmetic_policy::is_function(func))
        return context::SET_t_enum::A_TYPE;
    else if (ca_binary_policy::is_function(func))
        return context::SET_t_enum::B_TYPE;
    else if (ca_character_policy::is_function(func))
        return context::SET_t_enum::C_TYPE;
    return context::SET_t_enum::UNDEF_TYPE;
}

context::SET_t_enum ca_common_expr_policy::get_operands_type(ca_expr_ops op, context::SET_t_enum expr_kind)
{
    switch (expr_kind)
    {
        case context::SET_t_enum::A_TYPE:
            return ca_arithmetic_policy::get_operands_type(op);
        case context::SET_t_enum::B_TYPE:
            return ca_binary_policy::get_operands_type(op);
        case context::SET_t_enum::C_TYPE:
            return ca_character_policy::get_operands_type(op);
        default:
            return context::SET_t_enum::UNDEF_TYPE;
    }
}

// string to func
#define S2F(X)                                                                                                         \
    if (op == #X)                                                                                                      \
    return ca_expr_funcs::X

ca_expr_funcs ca_common_expr_policy::get_function(const std::string& op)
{
    S2F(B2A);
    S2F(C2A);
    S2F(D2A);
    S2F(DCLEN);
    S2F(FIND);
    S2F(INDEX);
    S2F(ISBIN);
    S2F(ISDEC);
    S2F(ISHEX);
    S2F(ISSYM);
    S2F(X2A);
    S2F(A2B);
    S2F(A2C);
    S2F(A2D);
    S2F(A2X);
    S2F(B2C);
    S2F(B2D);
    S2F(B2X);
    S2F(BYTE);
    S2F(C2B);
    S2F(C2D);
    S2F(C2X);
    S2F(D2B);
    S2F(D2C);
    S2F(D2X);
    S2F(DCVAL);
    S2F(DEQUOTE);
    S2F(DOUBLE);
    S2F(ESYM);
    S2F(LOWER);
    S2F(SIGNED);
    S2F(SYSATTRA);
    S2F(SYSATTRP);
    S2F(UPPER);
    S2F(X2B);
    S2F(X2C);
    S2F(X2D);

    return ca_expr_funcs::UNKNOWN;
}


} // namespace expressions
} // namespace parser_library
} // namespace hlasm_plugin
