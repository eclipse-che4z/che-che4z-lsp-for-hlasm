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

#include <cassert>
#include <charconv>
#include <iomanip>
#include <sstream>

#include "ca_string.h"
#include "ebcdic_encoding.h"
#include "expressions/evaluation_context.h"
#include "lexing/lexer.h"

#define RET_ERRPARM                                                                                                    \
    do                                                                                                                 \
    {                                                                                                                  \
        eval_ctx.add_diagnostic(diagnostic_op::error_CE007(expr_range));                                               \
        return context::SET_t();                                                                                       \
    } while (0)

namespace hlasm_plugin {
namespace parser_library {
namespace expressions {

ca_function::ca_function(
    ca_expr_funcs function, std::vector<ca_expr_ptr> parameters, ca_expr_ptr duplication_factor, range expr_range)
    : ca_expression(ca_common_expr_policy::get_function_type(function), std::move(expr_range))
    , function(function)
    , parameters(std::move(parameters))
    , duplication_factor(std::move(duplication_factor))
{ }

undef_sym_set ca_function::get_undefined_attributed_symbols(const context::dependency_solver& solver) const
{
    undef_sym_set ret;
    for (auto&& expr : parameters)
        ret.merge(expr->get_undefined_attributed_symbols(solver));
    if (duplication_factor)
        ret.merge(duplication_factor->get_undefined_attributed_symbols(solver));
    return ret;
}

void ca_function::resolve_expression_tree(context::SET_t_enum kind)
{
    if (kind != expr_kind)
        add_diagnostic(diagnostic_op::error_CE004(expr_range));
    else if (duplication_factor && expr_kind != context::SET_t_enum::C_TYPE)
        add_diagnostic(diagnostic_op::error_CE005(duplication_factor->expr_range));
    else
    {
        auto [param_size, param_kind] = ca_common_expr_policy::get_function_param_info(function, expr_kind);
        if (parameters.size() != param_size)
            add_diagnostic(diagnostic_op::error_CE006(expr_range));
        else
        {
            for (auto&& expr : parameters)
                expr->resolve_expression_tree(param_kind);
        }
    }
}

void ca_function::collect_diags() const { }

bool ca_function::is_character_expression() const { return false; }

context::SET_t ca_function::evaluate(evaluation_context& eval_ctx) const
{
    switch (function)
    {
        case ca_expr_funcs::B2A:
            return B2A(get_ith_param(0, eval_ctx), expr_range, eval_ctx);
        case ca_expr_funcs::C2A:
            return C2A(get_ith_param(0, eval_ctx), expr_range, eval_ctx);
        case ca_expr_funcs::D2A:
            return D2A(get_ith_param(0, eval_ctx), expr_range, eval_ctx);
        case ca_expr_funcs::DCLEN:
            return DCLEN(get_ith_param(0, eval_ctx));
        case ca_expr_funcs::FIND:
            return FIND(get_ith_param(0, eval_ctx), get_ith_param(1, eval_ctx));
        case ca_expr_funcs::INDEX:
            return INDEX(get_ith_param(0, eval_ctx), get_ith_param(1, eval_ctx));
        case ca_expr_funcs::ISBIN:
            return ISBIN(get_ith_param(0, eval_ctx), expr_range, eval_ctx);
        case ca_expr_funcs::ISDEC:
            return ISDEC(get_ith_param(0, eval_ctx), expr_range, eval_ctx);
        case ca_expr_funcs::ISHEX:
            return ISHEX(get_ith_param(0, eval_ctx), expr_range, eval_ctx);
        case ca_expr_funcs::ISSYM:
            return ISSYM(get_ith_param(0, eval_ctx), expr_range, eval_ctx);
        case ca_expr_funcs::X2A:
            return X2A(get_ith_param(0, eval_ctx), expr_range, eval_ctx);

        case ca_expr_funcs::A2B:
            return A2B(get_ith_param(0, eval_ctx));
        case ca_expr_funcs::A2C:
            return A2C(get_ith_param(0, eval_ctx));
        case ca_expr_funcs::A2D:
            return A2D(get_ith_param(0, eval_ctx));
        case ca_expr_funcs::A2X:
            return A2X(get_ith_param(0, eval_ctx));
        case ca_expr_funcs::B2C:
            return B2C(get_ith_param(0, eval_ctx), expr_range, eval_ctx);
        case ca_expr_funcs::B2D:
            return B2D(get_ith_param(0, eval_ctx), expr_range, eval_ctx);
        case ca_expr_funcs::B2X:
            return B2X(get_ith_param(0, eval_ctx), expr_range, eval_ctx);
        case ca_expr_funcs::BYTE:
            return BYTE(get_ith_param(0, eval_ctx), expr_range, eval_ctx);
        case ca_expr_funcs::C2B:
            return C2B(get_ith_param(0, eval_ctx), expr_range, eval_ctx);
        case ca_expr_funcs::C2D:
            return C2D(get_ith_param(0, eval_ctx), expr_range, eval_ctx);
        case ca_expr_funcs::C2X:
            return C2X(get_ith_param(0, eval_ctx), expr_range, eval_ctx);
        case ca_expr_funcs::D2B:
            return D2B(get_ith_param(0, eval_ctx), expr_range, eval_ctx);
        case ca_expr_funcs::D2C:
            return D2C(get_ith_param(0, eval_ctx), expr_range, eval_ctx);
        case ca_expr_funcs::D2X:
            return D2X(get_ith_param(0, eval_ctx), expr_range, eval_ctx);
        case ca_expr_funcs::DCVAL:
            return DCVAL(get_ith_param(0, eval_ctx));
        case ca_expr_funcs::DEQUOTE:
            return DEQUOTE(get_ith_param(0, eval_ctx));
        case ca_expr_funcs::DOUBLE:
            return DOUBLE(get_ith_param(0, eval_ctx), expr_range, eval_ctx);
        case ca_expr_funcs::ESYM:
            return ESYM(get_ith_param(0, eval_ctx));
        case ca_expr_funcs::LOWER:
            return LOWER(get_ith_param(0, eval_ctx));
        case ca_expr_funcs::SIGNED:
            return SIGNED(get_ith_param(0, eval_ctx));
        case ca_expr_funcs::SYSATTRA:
            return SYSATTRA(get_ith_param(0, eval_ctx));
        case ca_expr_funcs::SYSATTRP:
            return SYSATTRP(get_ith_param(0, eval_ctx));
        case ca_expr_funcs::UPPER:
            return UPPER(get_ith_param(0, eval_ctx));
        case ca_expr_funcs::X2B:
            return X2B(get_ith_param(0, eval_ctx), expr_range, eval_ctx);
        case ca_expr_funcs::X2C:
            return X2C(get_ith_param(0, eval_ctx), expr_range, eval_ctx);
        case ca_expr_funcs::X2D:
            return X2D(get_ith_param(0, eval_ctx), expr_range, eval_ctx);
        default:
            assert(false);
            return context::SET_t();
    }
}

context::SET_t ca_function::B2A(context::SET_t param, range expr_range, evaluation_context& eval_ctx)
{
    const auto& str = param.access_c();

    if (str.empty())
        return 0;

    if (str.size() > 32)
        RET_ERRPARM;

    context::A_t res;
    auto conv = std::from_chars(str.c_str(), str.c_str() + str.size(), res, 2);

    if (conv.ec == std::errc() || conv.ptr != str.c_str() + str.size())
        RET_ERRPARM;

    return res;
}

context::SET_t ca_function::C2A(context::SET_t param, range expr_range, evaluation_context& eval_ctx)
{
    if (param.access_c().empty())
        return 0;

    const auto& str = param.access_c();

    if (str.size() > 4)
        RET_ERRPARM;

    context::A_t ret = 0;
    for (const char* c = str.c_str(); *c != 0; ++c)
    {
        ret <<= 8;
        ret += ebcdic_encoding::to_ebcdic(ebcdic_encoding::to_pseudoascii(c));
    }

    return ret;
}

context::SET_t ca_function::D2A(context::SET_t param, range expr_range, evaluation_context& eval_ctx)
{
    const auto& str = param.access_c();

    if (str.empty())
        return 0;

    if (str.size() > 11)
        RET_ERRPARM;

    int res;

    auto it = std::find_if(str.begin(), str.end(), [](int c) { return c != '-' && c != '+'; });

    if (it - str.begin() > 1)
        RET_ERRPARM;

    size_t start = str.front() == '+' ? 1 : 0;

    auto conv = std::from_chars(str.c_str() + start, str.c_str() + str.size(), res, 10);

    if (conv.ec == std::errc() || conv.ptr != str.c_str() + str.size())
        RET_ERRPARM;

    return res;
}

context::SET_t ca_function::DCLEN(context::SET_t param)
{
    context::A_t ret = 0;
    for (const char* c = param.access_c().c_str(); *c != 0; ++c)
    {
        if ((*c == '\'' && *(c + 1) == '\'') || (*c == '&' && *(c + 1) == '&'))
            ++c;
        ++ret;
    }
    return ret;
}

context::SET_t ca_function::FIND(context::SET_t lhs, context::SET_t rhs)
{
    auto idx = lhs.access_c().find_first_of(rhs.access_c());
    return idx == std::string::npos ? 0 : (context::A_t)idx + 1;
}

context::SET_t ca_function::INDEX(context::SET_t lhs, context::SET_t rhs)
{
    auto idx = lhs.access_c().find(rhs.access_c());
    return idx == std::string::npos ? 0 : (context::A_t)idx + 1;
}

context::SET_t ca_function::ISBIN(context::SET_t param, range expr_range, evaluation_context& eval_ctx)
{
    const auto& str = param.access_c();
    if (str.empty())
        RET_ERRPARM;

    if (str.size() <= 32 && std ::all_of(str.cbegin(), str.cend(), [](char c) { return c == '0' || c == '1'; }))
        return (context::A_t)1;
    return (context::A_t)0;
}

context::SET_t ca_function::ISDEC(context::SET_t param, range expr_range, evaluation_context& eval_ctx)
{
    const auto& str = param.access_c();
    if (str.empty())
        RET_ERRPARM;

    context::A_t ret;

    if (str.size() > 10 || str.front() == '-')
        ret = 0;
    else
    {
        context::A_t tmp;
        auto conv = std::from_chars(str.c_str(), str.c_str() + str.size(), tmp, 10);

        if (conv.ec == std::errc() || conv.ptr != str.c_str() + str.size())
            ret = 0;
        else
            ret = 1;
    }
    return ret;
}

context::SET_t ca_function::ISHEX(context::SET_t param, range expr_range, evaluation_context& eval_ctx)
{
    const auto& str = param.access_c();
    if (str.empty())
        RET_ERRPARM;

    if (str.size() <= 8 && std::all_of(str.cbegin(), str.cend(), [](char c) { return std::isxdigit(c); }))
        return (context::A_t)1;
    return (context::A_t)0;
}

context::SET_t ca_function::ISSYM(context::SET_t param, range expr_range, evaluation_context& eval_ctx)
{
    const auto& str = param.access_c();
    if (str.empty())
        RET_ERRPARM;

    if (!std::isdigit(str.front()) && str.size() < 64 && std::all_of(str.cbegin(), str.cend(), lexing::lexer::ord_char))
        return 1;
    return 0;
}

context::SET_t ca_function::X2A(context::SET_t param, range expr_range, evaluation_context& eval_ctx)
{
    const auto& str = param.access_c();

    if (str.empty())
        return 0;

    if (str.size() > 8)
        RET_ERRPARM;

    int res;

    auto conv = std::from_chars(str.c_str(), str.c_str() + str.size(), res, 16);

    if (conv.ec == std::errc() || conv.ptr != str.c_str() + str.size())
        RET_ERRPARM;

    return res;
}

context::SET_t ca_function::A2B(context::SET_t param) { return std::bitset<32>(param.access_a()).to_string(); }

context::SET_t ca_function::A2C(context::SET_t param)
{
    auto value = param.access_a();
    std::uint32_t sign_mask = 1U << 31;
    std::uint32_t char_mask = 0xffU << 3 * 8;

    std::string ret;
    ret.reserve(4);

    for (size_t i = 0; i < 4; ++i)
    {
        auto sign = value & sign_mask;
        auto rest = value & char_mask;
        auto c = (unsigned char)(rest >> (3 - i) * 8);
        c |= sign >> (3 - i) * 8;

        ret.append(ebcdic_encoding::to_ascii(c));

        sign_mask >>= 8;
        char_mask >>= 8;
    }
    return ret;
}

context::SET_t ca_function::A2D(context::SET_t param)
{
    auto ret = std::to_string(param.access_a());
    if (ret.front() == '-')
        return ret;
    else
        return '+' + ret;
}

context::SET_t ca_function::A2X(context::SET_t param)
{
    std::stringstream stream;
    stream << std::setfill('0') << std::setw(8) << std::uppercase << std::hex << param.access_a();
    return stream.str();
}

context::SET_t ca_function::B2C(context::SET_t param, range expr_range, evaluation_context& eval_ctx)
{
    const auto& str = param.access_c();
    if (str.empty())
        return "";

    size_t algn = 0;
    if (str.size() % 8 != 0)
        algn = 8 - str.size() % 8;

    std::string new_str;

    for (size_t i = 0; i < algn; ++i)
        new_str.push_back('0');

    new_str += str;

    std::string ret;
    ret.reserve(new_str.size() / 8);

    for (size_t i = 0; i < new_str.size() / 8; ++i)
    {
        unsigned char c = 0;
        for (size_t j = 0; j < 8; ++j)
        {
            unsigned char bit = new_str[i * 8 + 7 - j] - '0';
            if (bit != 0 && bit != 1)
                RET_ERRPARM;
            c = (c << 1) + bit;
        }
        ret.append(ebcdic_encoding::to_ascii(c));
    }

    return ret;
}

context::SET_t ca_function::B2D(context::SET_t param, range expr_range, evaluation_context& eval_ctx)
{
    auto tmp = B2A(std::move(param), expr_range, eval_ctx);
    if (tmp.type == context::SET_t_enum::UNDEF_TYPE)
        return tmp;
    return A2D(tmp);
}

context::SET_t ca_function::B2X(context::SET_t param, range expr_range, evaluation_context& eval_ctx)
{
    const auto& str = param.access_c();
    if (str.empty())
        return "";

    size_t algn = 0;
    if (str.size() % 4 != 0)
        algn = 4 - str.size() % 4;

    std::string new_str;

    for (size_t i = 0; i < algn; ++i)
        new_str.push_back('0');

    new_str += str;

    std::string ret;
    ret.resize(new_str.size() / 4);

    for (size_t i = 0; i < new_str.size() / 4; ++i)
    {
        unsigned char c = 0;
        for (size_t j = 0; j < 4; ++j)
        {
            unsigned char bit = new_str[i * 4 + 3 - j] - '0';
            if (bit != 0 && bit != 1)
                RET_ERRPARM;
            c = (c << 1) + bit;
        }
        ret[i] = "0123456789ABCDEF"[c];
    }

    return ret;
}

context::SET_t ca_function::BYTE(context::SET_t param, range expr_range, evaluation_context& eval_ctx)
{
    auto value = param.access_a();
    if (value > 255 || value < 0)
        RET_ERRPARM;
    else
        return ebcdic_encoding::to_ascii((unsigned char)value);
}

context::SET_t ca_function::C2B(context::SET_t param, range expr_range, evaluation_context& eval_ctx)
{
    const auto& str = param.access_c();
    if (str.empty())
        return "";

    if (str.size() * 8 > ca_string::MAX_STR_SIZE)
        RET_ERRPARM;

    std::string ret;
    ret.reserve(str.size() * 8);
    for (const char* c = str.c_str(); *c != 0; ++c)
    {
        auto value = ebcdic_encoding::to_ebcdic(ebcdic_encoding::to_pseudoascii(c));
        ret.append(std::bitset<8>(value).to_string());
    }
    return ret;
}

context::SET_t ca_function::C2D(context::SET_t param, range expr_range, evaluation_context& eval_ctx)
{
    auto tmp = C2A(std::move(param), expr_range, eval_ctx);
    if (tmp.type == context::SET_t_enum::UNDEF_TYPE)
        return tmp;
    return A2D(tmp);
}

context::SET_t ca_function::C2X(context::SET_t param, range expr_range, evaluation_context& eval_ctx)
{
    const auto& str = param.access_c();
    if (str.empty())
        return "";

    if (str.size() * 2 > ca_string::MAX_STR_SIZE)
        RET_ERRPARM;

    std::string ret;
    ret.reserve(str.size() * 2);
    for (const char* c = str.c_str(); *c != 0; ++c)
    {
        auto value = ebcdic_encoding::to_ebcdic(ebcdic_encoding::to_pseudoascii(c));

        std::stringstream stream;
        stream << std::setfill('0') << std::setw(2) << std::uppercase << std::hex << value;
        ret.append(stream.str());
    }
    return ret;
}

context::SET_t ca_function::D2B(context::SET_t param, range expr_range, evaluation_context& eval_ctx)
{
    if (param.access_c().empty())
        return "";

    auto tmp = C2A(std::move(param), expr_range, eval_ctx);
    if (tmp.type == context::SET_t_enum::UNDEF_TYPE)
        return tmp;
    return A2B(tmp);
}

context::SET_t ca_function::D2C(context::SET_t param, range expr_range, evaluation_context& eval_ctx)
{
    if (param.access_c().empty())
        RET_ERRPARM;

    auto tmp = D2A(std::move(param), expr_range, eval_ctx);
    if (tmp.type == context::SET_t_enum::UNDEF_TYPE)
        return tmp;
    return A2C(tmp);
}

context::SET_t ca_function::D2X(context::SET_t param, range expr_range, evaluation_context& eval_ctx)
{
    if (param.access_c().empty())
        RET_ERRPARM;

    auto tmp = D2A(std::move(param), expr_range, eval_ctx);
    if (tmp.type == context::SET_t_enum::UNDEF_TYPE)
        return tmp;
    return A2X(tmp);
}

context::SET_t ca_function::DCVAL(context::SET_t param)
{
    std::string ret;

    for (const char* c = param.access_c().c_str(); *c != 0; ++c)
    {
        if ((*c == '\'' && *(c + 1) == '\'') || (*c == '&' && *(c + 1) == '&'))
            ++c;
        ret.push_back(*c);
    }
    return ret;
}

context::SET_t ca_function::DEQUOTE(context::SET_t param)
{
    std::string& str = param.access_c();
    if (str.empty())
        return "";

    if (str.front() == '\'')
        str.erase(str.begin());

    if (str.size() && str.back() == '\'')
        str.erase(str.end() - 1);

    return param;
}

context::SET_t ca_function::DOUBLE(context::SET_t param, range expr_range, evaluation_context& eval_ctx)
{
    std::string ret;
    ret.reserve(param.access_c().size());
    for (char c : param.access_c())
    {
        ret.push_back(c);
        if (c == '\'' || c == '&')
            ret.push_back(c);
    }

    if (ret.size() > ca_string::MAX_STR_SIZE)
        RET_ERRPARM;

    return ret;
}

context::SET_t ca_function::ESYM(context::SET_t ) { return context::SET_t(); }

context::SET_t ca_function::LOWER(context::SET_t param)
{
    std::string& value = param.access_c();
    std::transform(value.begin(), value.end(), value.begin(), [](char c) { return (char)tolower(c); });
    return value;
}

context::SET_t ca_function::SIGNED(context::SET_t param) { return std::to_string(param.access_a()); }

context::SET_t ca_function::SYSATTRA(context::SET_t ) { return context::SET_t(); }

context::SET_t ca_function::SYSATTRP(context::SET_t ) { return context::SET_t(); }

context::SET_t ca_function::UPPER(context::SET_t param)
{
    std::string& value = param.access_c();
    std::transform(value.begin(), value.end(), value.begin(), [](char c) { return (char)toupper(c); });
    return value;
}

context::SET_t ca_function::X2B(context::SET_t param, range expr_range, evaluation_context& eval_ctx)
{
    const auto& str = param.access_c();
    if (str.empty())
        return "";

    if (str.size() * 4 > ca_string::MAX_STR_SIZE)
        RET_ERRPARM;

    std::string ret;
    ret.reserve(str.size() * 4);
    for (auto c = str.c_str(); *c != '\0'; ++c)
    {
        unsigned char value = 0;
        if (std::isxdigit(*c))
            std::from_chars(c, c + 1, value, 16);
        else
            RET_ERRPARM;

        ret.append(std::bitset<4>(value).to_string());
    }
    return ret;
}

context::SET_t ca_function::X2C(context::SET_t param, range expr_range, evaluation_context& eval_ctx)
{
    const auto& str = param.access_c();
    if (str.empty())
        return "";

    std::string ret;
    for (auto c = str.c_str(); *c != '\0'; ++c)
    {
        unsigned char value = 0;
        if (std::isxdigit(*c))
            std::from_chars(c, c + 1, value, 16);
        else
            RET_ERRPARM;

        ret.append(ebcdic_encoding::to_ascii(value));
    }
    return ret;
}

context::SET_t ca_function::X2D(context::SET_t param, range expr_range, evaluation_context& eval_ctx)
{
    auto tmp = X2A(std::move(param), expr_range, eval_ctx);
    if (tmp.type == context::SET_t_enum::UNDEF_TYPE)
        return tmp;
    return A2D(tmp);
}

context::SET_t ca_function::get_ith_param(size_t idx, evaluation_context& eval_ctx) const
{
    if (idx < parameters.size())
        return parameters[idx]->evaluate(eval_ctx);
    else
        return context::SET_t();
}

} // namespace expressions
} // namespace parser_library
} // namespace hlasm_plugin
