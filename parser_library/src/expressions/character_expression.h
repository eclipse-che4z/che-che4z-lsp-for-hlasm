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

#ifndef HLASMPLUGIN_PARSER_HLASMCEXPRESSION_H
#define HLASMPLUGIN_PARSER_HLASMCEXPRESSION_H

#include <string>

#include "expression.h"
#include "logic_expression.h"

namespace hlasm_plugin::parser_library::expressions {

class expression;
class character_expression;
using char_ptr = std::shared_ptr<character_expression>;
/**
 * logic for character expression and operations on this type
 * */
class character_expression : public expression
{
public:
    character_expression() = default;
    character_expression(character_expression&&) = default;
    character_expression& operator=(character_expression&&) = default;
    character_expression(const character_expression&);
    character_expression(std::string);

    /**
     * all operations involving arguments check for errors
     * in all arguments immediately before accessing their values
     *
     * if any argument contains an error, it is copied
     * and an erroneous expression (meaning with en error)
     * is returned
     *
     * see: copy_return_on_error and copy_return_on_error_binary
     * */

    void append(std::string);
    char_ptr append(const char_ptr& arg) const;
    char_ptr append(const character_expression* arg) const;

    virtual expr_ptr binary_operation(str_ref operation_name, expr_ref arg2) const override;
    virtual expr_ptr unary_operation(str_ref operation_name) const override;

    context::SET_t get_set_value() const override;

    /**
     * special HLASM CA substring
     * */
    template<typename T>
    char_ptr substring(int32_t dupl, const T& s, const T& e) const
    {
        if (dupl < 0)
            return default_expr_with_error<character_expression>(error_messages::ec01());

        int32_t start = 0;
        int32_t len = static_cast<int32_t>(value_.length());

        if (s != nullptr)
            start = s->get_numeric_value() - 1;


        if (e != nullptr)
            len = e->get_numeric_value();

        if (start < 0 || len < 0)
            return default_expr_with_error<character_expression>(error_messages::ec02());

        if ((size_t)start > value_.size())
            return default_expr_with_error<character_expression>(error_messages::ec02());

        auto value = value_.substr(start, len);

        if (dupl > 1)
        {
            value.reserve(dupl * value.length());
            auto val = value;
            for (int32_t i = 1; i < dupl; ++i)
                value.append(val);
        }

        return std::make_shared<character_expression>(std::move(value));
    }

    std::string get_str_val() const override;

    static std::string num_to_hex(int32_t val);
    static char num_to_hex_char(int32_t val);
    /**
     * number as EBCDIC string
     * */
    static std::string num_to_ebcdic(int32_t val);
    static bool isalpha_hlasm(char c);
    static char hex_to_num(char c, size_t*);

private:
    // str len
    expr_ptr dclen() const;
    expr_ptr isbin() const;
    expr_ptr isdec() const;
    expr_ptr ishex() const;
    expr_ptr issym() const;
    // interpret binary string as EBCDIC string
    expr_ptr b2c() const;
    /**
     * convert binary string to decimal
     * convert to string with sign
     * */
    expr_ptr b2d() const;
    /**
     * convert binary string to  hexadecimal
     * convert to string with sign
     * */
    expr_ptr b2x() const;
    /**
     * EBCDIC characters as binary
     * */
    expr_ptr c2b() const;
    /**
     * EBCDIC characters as decimal string with sign
     * */
    expr_ptr c2d() const;
    /**
     * EBCDIC characters as hexadecimal string with sign
     * */
    expr_ptr c2x() const;
    /**
     * decimal string to binary string
     * */
    expr_ptr d2b() const;
    /**
     * decimal string to EBCDIC string
     * */
    expr_ptr d2c() const;
    /**
     * decimal string to headecimal string
     * */
    expr_ptr d2x() const;
    /**
     * dequote double_quote() string
     * */
    expr_ptr dcval() const;
    /**
     * remove quotation of ' from ends
     * */
    expr_ptr dequote() const;
    /**
     * double ' and & characters in string
     * */
    expr_ptr double_quote() const;
    /**
     * hexadecimal string as binary string
     * */
    expr_ptr x2b() const;
    /**
     * hexadecimal string to number
     * number interpreted as EBCDIC string
     * */
    expr_ptr x2c() const;
    /**
     * hexadecimal string as decimal string with sign
     * */
    expr_ptr x2d() const;

    std::string value_ = "";
};

} // namespace hlasm_plugin::parser_library::expressions

#endif
