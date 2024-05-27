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

#include "ca_function.h"

#include <array>
#include <cassert>
#include <charconv>
#include <iomanip>
#include <sstream>

#include "ca_string.h"
#include "ebcdic_encoding.h"
#include "expressions/conditional_assembly/ca_expr_visitor.h"
#include "expressions/evaluation_context.h"
#include "lexing/lexer.h"
#include "semantics/variable_symbol.h"

#define RET_ERRPARM                                                                                                    \
    do                                                                                                                 \
    {                                                                                                                  \
        add_diagnostic(diagnostic_op::error_CE007);                                                                    \
        return context::SET_t();                                                                                       \
    } while (0)

namespace hlasm_plugin::parser_library::expressions {

ca_function::ca_function(context::id_index function_name,
    ca_expr_funcs function,
    std::vector<ca_expr_ptr> parameters,
    ca_expr_ptr duplication_factor,
    range expr_range)
    : ca_expression(ca_common_expr_policy::get_function_type(function), std::move(expr_range))
    , function_name(function_name)
    , function(function)
    , parameters(std::move(parameters))
    , duplication_factor(std::move(duplication_factor))
{}

bool ca_function::get_undefined_attributed_symbols(
    std::vector<context::id_index>& symbols, const evaluation_context& eval_ctx) const
{
    bool result = false;
    for (auto&& expr : parameters)
        result |= expr->get_undefined_attributed_symbols(symbols, eval_ctx);
    if (duplication_factor)
        result |= duplication_factor->get_undefined_attributed_symbols(symbols, eval_ctx);
    return result;
}

void ca_function::resolve_expression_tree(ca_expression_ctx expr_ctx, diagnostic_op_consumer& diags)
{
    // No diag when kind == expr_kind or when there is a combination of A_TYPE and B_TYPE
    static constexpr bool allowed_combinations[4][4] = {
        { 1, 1, 0, 0 },
        { 1, 1, 0, 0 },
        { 0, 0, 1, 0 },
        { 0, 0, 0, 1 },
    };

    if (!allowed_combinations[static_cast<int>(expr_ctx.kind)][static_cast<int>(expr_kind)])
        diags.add_diagnostic(diagnostic_op::error_CE004(expr_range));
    else if (duplication_factor && expr_kind != context::SET_t_enum::C_TYPE)
        diags.add_diagnostic(diagnostic_op::error_CE005(duplication_factor->expr_range));
    else if (auto [param_size, param_kind] = ca_common_expr_policy::get_function_param_info(function, expr_kind);
             parameters.size() != param_size)
        diags.add_diagnostic(diagnostic_op::error_CE006(expr_range));
    else
    {
        expr_ctx.kind = param_kind;
        for (auto&& expr : parameters)
        {
            expr->resolve_expression_tree(expr_ctx, diags);
        }
    }
}

bool ca_function::is_character_expression(character_expression_purpose purpose) const
{
    return purpose == character_expression_purpose::assignment && ca_character_policy::is_function(function);
}

void ca_function::apply(ca_expr_visitor& visitor) const { visitor.visit(*this); }

context::SET_t ca_function::evaluate(const evaluation_context& eval_ctx) const
{
    context::SET_t str_ret;
    diagnostic_adder add_diagnostic(eval_ctx.diags, expr_range);

    switch (function)
    {
        case ca_expr_funcs::B2A:
            return B2A(get_ith_param(0, eval_ctx).access_c(), add_diagnostic);
        case ca_expr_funcs::C2A:
            return C2A(get_ith_param(0, eval_ctx).access_c(), add_diagnostic);
        case ca_expr_funcs::D2A:
            return D2A(get_ith_param(0, eval_ctx).access_c(), add_diagnostic);
        case ca_expr_funcs::DCLEN:
            return DCLEN(get_ith_param(0, eval_ctx).access_c());
        case ca_expr_funcs::FIND:
            return FIND(get_ith_param(0, eval_ctx).access_c(), get_ith_param(1, eval_ctx).access_c());
        case ca_expr_funcs::INDEX:
            return INDEX(get_ith_param(0, eval_ctx).access_c(), get_ith_param(1, eval_ctx).access_c());
        case ca_expr_funcs::ISBIN:
            return ISBIN(get_ith_param(0, eval_ctx).access_c(), add_diagnostic);
        case ca_expr_funcs::ISDEC:
            return ISDEC(get_ith_param(0, eval_ctx).access_c(), add_diagnostic);
        case ca_expr_funcs::ISHEX:
            return ISHEX(get_ith_param(0, eval_ctx).access_c(), add_diagnostic);
        case ca_expr_funcs::ISSYM:
            return ISSYM(get_ith_param(0, eval_ctx).access_c(), add_diagnostic);
        case ca_expr_funcs::X2A:
            return X2A(get_ith_param(0, eval_ctx).access_c(), add_diagnostic);

        case ca_expr_funcs::A2B:
            str_ret = A2B(get_ith_param(0, eval_ctx).access_a());
            break;
        case ca_expr_funcs::A2C:
            str_ret = A2C(get_ith_param(0, eval_ctx).access_a());
            break;
        case ca_expr_funcs::A2D:
            str_ret = A2D(get_ith_param(0, eval_ctx).access_a());
            break;
        case ca_expr_funcs::A2X:
            str_ret = A2X(get_ith_param(0, eval_ctx).access_a());
            break;
        case ca_expr_funcs::B2C:
            str_ret = B2C(get_ith_param(0, eval_ctx).access_c(), add_diagnostic);
            break;
        case ca_expr_funcs::B2D:
            str_ret = B2D(get_ith_param(0, eval_ctx).access_c(), add_diagnostic);
            break;
        case ca_expr_funcs::B2X:
            str_ret = B2X(get_ith_param(0, eval_ctx).access_c(), add_diagnostic);
            break;
        case ca_expr_funcs::BYTE:
            str_ret = BYTE(get_ith_param(0, eval_ctx).access_a(), add_diagnostic);
            break;
        case ca_expr_funcs::C2B:
            str_ret = C2B(get_ith_param(0, eval_ctx).access_c(), add_diagnostic);
            break;
        case ca_expr_funcs::C2D:
            str_ret = C2D(get_ith_param(0, eval_ctx).access_c(), add_diagnostic);
            break;
        case ca_expr_funcs::C2X:
            str_ret = C2X(get_ith_param(0, eval_ctx).access_c(), add_diagnostic);
            break;
        case ca_expr_funcs::D2B:
            str_ret = D2B(get_ith_param(0, eval_ctx).access_c(), add_diagnostic);
            break;
        case ca_expr_funcs::D2C:
            str_ret = D2C(get_ith_param(0, eval_ctx).access_c(), add_diagnostic);
            break;
        case ca_expr_funcs::D2X:
            str_ret = D2X(get_ith_param(0, eval_ctx).access_c(), add_diagnostic);
            break;
        case ca_expr_funcs::DCVAL:
            str_ret = DCVAL(get_ith_param(0, eval_ctx).access_c());
            break;
        case ca_expr_funcs::DEQUOTE:
            str_ret = DEQUOTE(get_ith_param(0, eval_ctx).access_c());
            break;
        case ca_expr_funcs::DOUBLE:
            str_ret = DOUBLE(get_ith_param(0, eval_ctx).access_c(), add_diagnostic);
            break;
        case ca_expr_funcs::ESYM:
            str_ret = ESYM(get_ith_param(0, eval_ctx).access_c());
            break;
        case ca_expr_funcs::LOWER:
            str_ret = LOWER(get_ith_param(0, eval_ctx).access_c());
            break;
        case ca_expr_funcs::SIGNED:
            str_ret = SIGNED(get_ith_param(0, eval_ctx).access_a());
            break;
        case ca_expr_funcs::SYSATTRA:
            str_ret = SYSATTRA(get_ith_param(0, eval_ctx).access_c());
            break;
        case ca_expr_funcs::SYSATTRP:
            str_ret = SYSATTRP(get_ith_param(0, eval_ctx).access_c());
            break;
        case ca_expr_funcs::UPPER:
            str_ret = UPPER(get_ith_param(0, eval_ctx).access_c());
            break;
        case ca_expr_funcs::X2B:
            str_ret = X2B(get_ith_param(0, eval_ctx).access_c(), add_diagnostic);
            break;
        case ca_expr_funcs::X2C:
            str_ret = X2C(get_ith_param(0, eval_ctx).access_c(), add_diagnostic);
            break;
        case ca_expr_funcs::X2D:
            str_ret = X2D(get_ith_param(0, eval_ctx).access_c(), add_diagnostic);
            break;
        default:
            return context::SET_t();
    }

    return ca_string::duplicate(duplication_factor, std::move(str_ret.access_c()), expr_range, eval_ctx);
}

context::SET_t ca_function::B2A(std::string_view param, diagnostic_adder& add_diagnostic)
{
    if (param.empty())
        return 0;

    if (param.size() > 32)
        RET_ERRPARM;

    uint32_t res;
    auto conv = std::from_chars(param.data(), param.data() + param.size(), res, 2);

    if (conv.ec != std::errc() || conv.ptr != param.data() + param.size())
        RET_ERRPARM;

    return static_cast<int32_t>(res);
}

context::SET_t ca_function::C2A(std::string_view param, diagnostic_adder& add_diagnostic)
{
    if (param.empty())
        return 0;

    const char* c = std::to_address(param.cbegin());
    const auto ce = std::to_address(param.cend());

    context::A_t ret = 0;
    for (auto n = 0; n < 4 && c != ce; ++n)
    {
        ret <<= 8;
        const auto [ch, newc] = ebcdic_encoding::to_ebcdic(c, ce);
        ret += ch;
        c = newc;
    }

    if (c != ce)
        RET_ERRPARM;

    return ret;
}

context::SET_t ca_function::D2A(std::string_view param, diagnostic_adder& add_diagnostic)
{
    if (param.empty())
        return 0;

    auto it = std::ranges::find_if(param, [](int c) { return c != '-' && c != '+'; });

    if (it - param.begin() > 1)
        RET_ERRPARM;

    if (param.end() - it > 10)
        RET_ERRPARM;

    size_t start = param.front() == '+' ? 1 : 0;

    int res = 0;
    auto conv = std::from_chars(param.data() + start, param.data() + param.size(), res, 10);

    if (conv.ec != std::errc() || conv.ptr != param.data() + param.size())
        RET_ERRPARM;

    return res;
}

context::SET_t ca_function::DCLEN(const context::C_t& param)
{
    context::A_t ret = 0;
    const char* c = param.c_str();
    while (c < param.c_str() + param.size())
    {
        if ((*c == '\'' && *(c + 1) == '\'') || (*c == '&' && *(c + 1) == '&'))
            ++c;
        ++ret;
        ++c;
    }
    return ret;
}

context::SET_t ca_function::FIND(const context::C_t& lhs, const context::C_t& rhs)
{
    auto idx = lhs.find_first_of(rhs);
    return idx == std::string::npos ? 0 : (context::A_t)idx + 1;
}

context::SET_t ca_function::INDEX(const context::C_t& lhs, const context::C_t& rhs)
{
    auto idx = lhs.find(rhs);
    return idx == std::string::npos ? 0 : (context::A_t)idx + 1;
}

context::SET_t ca_function::ISBIN(const context::C_t& param, diagnostic_adder& add_diagnostic)
{
    if (param.empty())
        RET_ERRPARM;

    if (param.size() <= 32 && std::ranges::all_of(param, [](char c) { return c == '0' || c == '1'; }))
        return true;
    return false;
}

context::SET_t ca_function::ISDEC(const context::C_t& param, diagnostic_adder& add_diagnostic)
{
    if (param.empty())
        RET_ERRPARM;

    context::B_t ret;

    if (param.size() > 10 || param.front() == '-')
        ret = false;
    else
    {
        context::A_t tmp;
        auto conv = std::from_chars(param.c_str(), param.c_str() + param.size(), tmp, 10);

        if (conv.ec != std::errc() || conv.ptr != param.c_str() + param.size())
            ret = false;
        else
            ret = true;
    }
    return ret;
}

context::SET_t ca_function::ISHEX(const context::C_t& param, diagnostic_adder& add_diagnostic)
{
    if (param.empty())
        RET_ERRPARM;

    if (param.size() <= 8 && std::ranges::all_of(param, [](unsigned char c) { return std::isxdigit(c); }))
        return true;
    return false;
}

context::SET_t ca_function::ISSYM(const context::C_t& param, diagnostic_adder& add_diagnostic)
{
    if (param.empty())
        RET_ERRPARM;

    return !std::isdigit((unsigned char)param.front()) && param.size() < 64
        && std::ranges::all_of(param, lexing::lexer::ord_char);
}

context::SET_t ca_function::X2A(std::string_view param, diagnostic_adder& add_diagnostic)
{
    if (param.empty())
        return 0;

    if (param.size() > 8)
        RET_ERRPARM;

    unsigned int res;

    auto conv = std::from_chars(param.data(), param.data() + param.size(), res, 16);

    if (conv.ec != std::errc() || conv.ptr != param.data() + param.size())
        RET_ERRPARM;

    return *reinterpret_cast<int*>(&res);
}

context::SET_t ca_function::A2B(context::A_t param) { return std::bitset<32>(param).to_string(); }

context::SET_t ca_function::A2C(context::A_t param)
{
    std::uint32_t uparam = param;

    return ebcdic_encoding::to_ascii(std::string {
        static_cast<char>(uparam >> 24 & 0xff),
        static_cast<char>(uparam >> 16 & 0xff),
        static_cast<char>(uparam >> 8 & 0xff),
        static_cast<char>(uparam >> 0 & 0xff),
    });
}

context::SET_t ca_function::A2D(context::A_t param)
{
    auto ret = std::to_string(param);
    if (ret.front() == '-')
        return ret;
    else
        return '+' + ret;
}

context::SET_t ca_function::A2X(context::A_t param)
{
    unsigned long uparam = param;
    std::string result;

    for (int shift = 28; shift >= 0; shift -= 4)
        result.push_back("0123456789ABCDEF"[(uparam >> shift) & 0xf]);

    return result;
}

context::SET_t ca_function::B2C(const context::C_t& param, diagnostic_adder& add_diagnostic)
{
    if (param.empty())
        return "";

    size_t algn = 0;
    if (param.size() % 8 != 0)
        algn = 8 - param.size() % 8;

    std::string new_str;

    for (size_t i = 0; i < algn; ++i)
        new_str.push_back('0');

    new_str += param;

    std::string ret;
    ret.reserve(new_str.size() / 8);

    for (size_t i = 0; i < new_str.size() / 8; ++i)
    {
        unsigned char c = 0;
        for (size_t j = 0; j < 8; ++j)
        {
            unsigned char bit = new_str[i * 8 + j] - '0';
            if (bit != 0 && bit != 1)
                RET_ERRPARM;
            c = (c << 1) + bit;
        }
        ret.append(ebcdic_encoding::to_ascii(c));
    }

    return ret;
}

context::SET_t ca_function::B2D(const context::C_t& param, diagnostic_adder& add_diagnostic)
{
    auto tmp = B2A(param, add_diagnostic);
    if (tmp.type() == context::SET_t_enum::UNDEF_TYPE)
        return tmp;
    return A2D(tmp.access_a());
}

context::SET_t ca_function::B2X(const context::C_t& param, diagnostic_adder& add_diagnostic)
{
    if (param.empty())
        return "";

    size_t algn = 0;
    if (param.size() % 4 != 0)
        algn = 4 - param.size() % 4;

    std::string new_str;

    for (size_t i = 0; i < algn; ++i)
        new_str.push_back('0');

    new_str += param;

    std::string ret;
    ret.resize(new_str.size() / 4);

    for (size_t i = 0; i < new_str.size() / 4; ++i)
    {
        unsigned char c = 0;
        for (size_t j = 0; j < 4; ++j)
        {
            unsigned char bit = new_str[i * 4 + j] - '0';
            if (bit != 0 && bit != 1)
                RET_ERRPARM;
            c = (c << 1) + bit;
        }
        ret[i] = "0123456789ABCDEF"[c];
    }

    return ret;
}

context::SET_t ca_function::BYTE(context::A_t param, diagnostic_adder& add_diagnostic)
{
    if (param > 255 || param < 0)
        RET_ERRPARM;
    else
        return ebcdic_encoding::to_ascii((unsigned char)param);
}

context::SET_t ca_function::C2B(const context::C_t& param, diagnostic_adder& add_diagnostic)
{
    if (param.empty())
        return "";

    const char* c = std::to_address(param.cbegin());
    const auto ce = std::to_address(param.cend());

    std::string ret;
    ret.reserve(std::min(param.size() * 8, ca_string::MAX_STR_SIZE));
    while (c != ce && ret.size() + 8 <= ca_string::MAX_STR_SIZE)
    {
        const auto [value, newc] = ebcdic_encoding::to_ebcdic(c, ce);
        ret.append(std::bitset<8>(value).to_string());
        c = newc;
    }

    if (c != ce)
        RET_ERRPARM;

    return ret;
}

context::SET_t ca_function::C2D(const context::C_t& param, diagnostic_adder& add_diagnostic)
{
    auto tmp = C2A(param, add_diagnostic);
    if (tmp.type() == context::SET_t_enum::UNDEF_TYPE)
        return tmp;
    return A2D(tmp.access_a());
}

context::SET_t ca_function::C2X(const context::C_t& param, diagnostic_adder& add_diagnostic)
{
    if (param.empty())
        return "";

    const char* c = std::to_address(param.cbegin());
    const auto ce = std::to_address(param.cend());

    std::string ret;
    ret.reserve(std::min(param.size() * 2, ca_string::MAX_STR_SIZE));
    while (c != ce && ret.size() + 2 <= ca_string::MAX_STR_SIZE)
    {
        const auto [value, newc] = ebcdic_encoding::to_ebcdic(c, ce);

        ret.push_back("0123456789ABCDEF"[value >> 4]);
        ret.push_back("0123456789ABCDEF"[value & 0xf]);

        c = newc;
    }

    if (c != ce)
        RET_ERRPARM;

    return ret;
}

context::SET_t ca_function::D2B(const context::C_t& param, diagnostic_adder& add_diagnostic)
{
    if (param.empty())
        return "";

    auto tmp = D2A(param, add_diagnostic);
    if (tmp.type() == context::SET_t_enum::UNDEF_TYPE)
        return tmp;
    return A2B(tmp.access_a());
}

context::SET_t ca_function::D2C(const context::C_t& param, diagnostic_adder& add_diagnostic)
{
    if (param.empty())
        RET_ERRPARM;

    auto tmp = D2A(param, add_diagnostic);
    if (tmp.type() == context::SET_t_enum::UNDEF_TYPE)
        return tmp;
    return A2C(tmp.access_a());
}

context::SET_t ca_function::D2X(const context::C_t& param, diagnostic_adder& add_diagnostic)
{
    if (param.empty())
        RET_ERRPARM;

    auto tmp = D2A(param, add_diagnostic);
    if (tmp.type() == context::SET_t_enum::UNDEF_TYPE)
        return tmp;
    return A2X(tmp.access_a());
}

context::SET_t ca_function::DCVAL(const context::C_t& param)
{
    std::string ret;
    const char* c = param.c_str();
    while (c < param.c_str() + param.size())
    {
        if ((*c == '\'' && *(c + 1) == '\'') || (*c == '&' && *(c + 1) == '&'))
            ++c;
        ret.push_back(*c);
        ++c;
    }
    return ret;
}

context::SET_t ca_function::DEQUOTE(context::C_t param)
{
    if (param.empty())
        return "";

    if (param.front() == '\'')
        param.erase(param.begin());

    if (param.size() && param.back() == '\'')
        param.pop_back();

    return param;
}

context::SET_t ca_function::DOUBLE(const context::C_t& param, diagnostic_adder& add_diagnostic)
{
    std::string ret;
    ret.reserve(param.size());
    for (char c : param)
    {
        ret.push_back(c);
        if (c == '\'' || c == '&')
            ret.push_back(c);
    }

    if (ret.size() > ca_string::MAX_STR_SIZE)
        RET_ERRPARM;

    return ret;
}

context::SET_t ca_function::ESYM(const context::C_t&) { return context::SET_t(); }

context::SET_t ca_function::LOWER(context::C_t param)
{
    std::ranges::transform(param, param.begin(), [](unsigned char c) { return (char)tolower(c); });
    return param;
}

context::SET_t ca_function::SIGNED(context::A_t param) { return std::to_string(param); }

context::SET_t ca_function::SYSATTRA(const context::C_t&) { return context::SET_t(); }

context::SET_t ca_function::SYSATTRP(const context::C_t&) { return context::SET_t(); }

context::SET_t ca_function::UPPER(context::C_t param)
{
    std::ranges::transform(param, param.begin(), [](unsigned char c) { return (char)toupper(c); });
    return param;
}

context::SET_t ca_function::X2B(const context::C_t& param, diagnostic_adder& add_diagnostic)
{
    if (param.empty())
        return "";

    if (param.size() * 4 > ca_string::MAX_STR_SIZE)
        RET_ERRPARM;

    std::string ret;
    ret.reserve(param.size() * 4);
    for (auto c = param.c_str(); c != param.c_str() + param.size(); ++c)
    {
        unsigned char value = 0;
        if (std::isxdigit((unsigned char)*c))
            std::from_chars(c, c + 1, value, 16);
        else
            RET_ERRPARM;

        ret.append(std::bitset<4>(value).to_string());
    }
    return ret;
}

context::SET_t ca_function::X2C(const context::C_t& param, diagnostic_adder& add_diagnostic)
{
    if (param.empty())
        return "";

    std::string new_string;
    new_string.reserve(param.size());

    if (param.size() % 2 == 1)
        new_string.push_back('0');
    new_string += param;

    std::string ret;
    for (auto c = new_string.c_str(); c != new_string.c_str() + new_string.size(); c += 2)
    {
        unsigned char value = 0;
        if (std::isxdigit((unsigned char)*c))
            std::from_chars(c, c + 2, value, 16);
        else
            RET_ERRPARM;

        ret.append(ebcdic_encoding::to_ascii(value));
    }
    return ret;
}

context::SET_t ca_function::X2D(const context::C_t& param, diagnostic_adder& add_diagnostic)
{
    auto tmp = X2A(param, add_diagnostic);
    if (tmp.type() == context::SET_t_enum::UNDEF_TYPE)
        return tmp;
    return A2D(tmp.access_a());
}

context::SET_t ca_function::get_ith_param(size_t idx, const evaluation_context& eval_ctx) const
{
    if (idx < parameters.size())
        return parameters[idx]->evaluate(eval_ctx);
    else
        return context::SET_t();
}

} // namespace hlasm_plugin::parser_library::expressions
