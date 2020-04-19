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

#ifndef HLASMPLUGIN_PARSER_HLASMKEXPRESSION_H
#define HLASMPLUGIN_PARSER_HLASMKEXPRESSION_H
#include <algorithm>
#include <map>
#include <string>

#include "expression.h"

namespace hlasm_plugin {
namespace parser_library {
namespace expressions {
/**
 * helper type, not an actual expression
 * represents unary and binary operators
 * */
class keyword_expression : public expression
{
public:
    keyword_expression(str_ref);
    keyword_expression(const keyword_expression& expr);
    keyword_expression(keyword_expression&&) = default;
    keyword_expression& operator=(keyword_expression&&) = default;
    expr_ptr to_expression() const;
    static bool is_keyword(str_ref kw);


#define KEYWORDS                                                                                                       \
    X(AND)                                                                                                             \
    X(AND_NOT)                                                                                                         \
    X(OR)                                                                                                              \
    X(OR_NOT)                                                                                                          \
    X(XOR)                                                                                                             \
    X(XOR_NOT)                                                                                                         \
    X(NOT)                                                                                                             \
    X(EQ)                                                                                                              \
    X(NE)                                                                                                              \
    X(LE)                                                                                                              \
    X(LT)                                                                                                              \
    X(GE)                                                                                                              \
    X(GT)                                                                                                              \
    X(ASTERISK)                                                                                                        \
    X(FIND)                                                                                                            \
    X(INDEX)                                                                                                           \
    X(SLA)                                                                                                             \
    X(SLL)                                                                                                             \
    X(SRA)                                                                                                             \
    X(SRL)                                                                                                             \
    X(BYTE)                                                                                                            \
    X(LOWER)                                                                                                           \
    X(SIGNED)                                                                                                          \
    X(UPPER)                                                                                                           \
    X(DOUBLE)

#define X(k) k,
    enum class keyword_type
    {
        KEYWORDS
    };
#undef X

    bool is_unary() const;
    uint8_t priority() const;
    bool is_keyword() const override;
    bool is_complex_keyword() const override;
    std::string get_str_val() const override;

private:
    /**
     * HLASM is case insensitive
     * */
    struct upper_equal
    {
        bool operator()(const char l, const char r) const { return toupper(l) < toupper(r); }
        bool operator()(const std::string& l, const std::string& r) const
        {
            return std::lexicographical_compare(l.cbegin(), l.cend(), r.cbegin(), r.cend(), upper_equal());
        }
    };
    /**
     * mapping of string to keyword_type
     * */
    static std::map<std::string, keyword_type, upper_equal> keywords_;
    std::string s_val_;
    keyword_type value_;
};
} // namespace expressions
} // namespace parser_library
} // namespace hlasm_plugin

#endif
