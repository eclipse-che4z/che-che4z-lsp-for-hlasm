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

bool ca_arithmetic_policy::is_unary(const std::string& op) { return op == "NOT"; }
bool ca_binary_policy::is_unary(const std::string& op) { return op == "NOT"; }
bool ca_character_policy::is_unary(const std::string& op)
{
    return op == "BYTE" || op == "DOUBLE" || op == "LOWER" || op == "SIGNED" || op == "UPPER";
}

bool ca_arithmetic_policy::multiple_words(const std::string&) { return false; }
bool ca_binary_policy::multiple_words(const std::string& op) { return op == "AND" || op == "OR" || op == "XOR"; }
bool ca_character_policy::multiple_words(const std::string&) { return false; }

bool ca_arithmetic_policy::is_binary(const std::string& op)
{
    return op == "SLA" || op == "SLL" || op == "SRA" || op == "SRL" || op == "FIND" || op == "INDEX" || op == "AND"
        || op == "OR" || op == "XOR";
}

bool ca_binary_policy::is_binary(const std::string& op)
{
    return op == "EQ" || op == "NE" || op == "LE" || op == "LT" || op == "GE" || op == "GT" || op == "AND" || op == "OR"
        || op == "XOR" || op == "AND NOT" || op == "OR NOT" || op == "XOR NOT";
}

bool ca_character_policy::is_binary(const std::string&) { return false; }

bool ca_arithmetic_policy::is_operator(const std::string& op) { return is_unary(op) || is_binary(op) };
bool ca_binary_policy::is_operator(const std::string& op) { return is_unary(op) || is_binary(op) };
bool ca_character_policy::is_operator(const std::string& op) { return is_unary(op) || is_binary(op) };

int ca_arithmetic_policy::get_priority(const std::string& op)
{
    if (op == "FIND" || op == "INDEX")
        return 0;
    else if (op == "NOT")
        return 1;
    else if (op == "AND" || op == "OR" || op == "XOR")
        return 2;
    else if (op == "SLA" || op == "SLL" || op == "SRA" || op == "SRL")
        return 3;
    return 0;
}

int ca_binary_policy::get_priority(const std::string& op)
{
    if (op == "EQ" || op == "NE" || op == "LE" || op == "LT" || op == "GE" || op == "GT")
        return 0;
    else if (op == "NOT")
        return 1;
    else if (op == "AND" || op == "AND NOT")
        return 2;
    else if (op == "OR" || op == "OR NOT")
        return 3;
    else if (op == "XOR" || op == "XOR NOT")
        return 4;
    return 0;
}

int ca_character_policy::get_priority(const std::string& op) { return 0; }

// string to enum
#define S2E(X)                                                                                                         \
    if (op == "X")                                                                                                     \
    return ca_expr_ops::X

ca_expr_ops get_ca_operator(const std::string& symbol)
{
    S2E(SLA);
    S2E(SLL);
    S2E(SRA);
    S2E(SRL);
    S2E(FIND);
    S2E(INDEX);
    if (op == "AND NOT")
        return ca_expr_ops::AND_NOT;
    if (op == "OR NOT")
        return ca_expr_ops::OR_NOT;
    if (op == "XOR NOT")
        return ca_expr_ops::XOR_NOT;
    S2E(EQ);
    S2E(NE);
    S2E(LE);
    S2E(LT);
    S2E(GE);
    S2E(GT);
    S2E(AND);
    S2E(OR);
    S2E(XOR);
    S2E(NOT);
    S2E(BYTE);
    S2E(DOUBLE);
    S2E(LOWER);
    S2E(SIGNED);
    S2E(UPPER);
}

ca_expr_ops ca_arithmetic_policy::get_operator(const std::string& op) { return get_ca_operator(op); }
ca_expr_ops ca_binary_policy::get_operator(const std::string& op) { return get_ca_operator(op); }
ca_expr_ops ca_character_policy::get_operator(const std::string& op) { return get_ca_operator(op); }

} // namespace expressions
} // namespace parser_library
} // namespace hlasm_plugin


#endif
