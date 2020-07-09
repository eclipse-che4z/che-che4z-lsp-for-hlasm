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
        add_diagnostic(diagnostic_op::error_CE007);                                                                    \
        return context::SET_t();                                                                                       \
    } while (0)

namespace hlasm_plugin {
namespace parser_library {
namespace expressions {

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
    context::SET_t str_ret;
    ranged_diagnostic_collector add_diagnostic(&eval_ctx, expr_range);

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

context::SET_t ca_function::B2A(std::string_view param, const ranged_diagnostic_collector& add_diagnostic)
{
    if (param.empty())
        return 0;

    if (param.size() > 32)
        RET_ERRPARM;

    unsigned int res;
    auto conv = std::from_chars(param.data(), param.data() + param.size(), res, 2);

    if (conv.ec != std::errc() || conv.ptr != param.data() + param.size())
        RET_ERRPARM;

    return *reinterpret_cast<int*>(&res);
}

context::SET_t ca_function::C2A(std::string_view param, const ranged_diagnostic_collector& add_diagnostic)
{
    if (param.empty())
        return 0;

    if (param.size() > 4)
        RET_ERRPARM;

    context::A_t ret = 0;
    for (const char* c = param.data(); c < param.data() + param.size(); ++c)
    {
        ret <<= 8;
        ret += ebcdic_encoding::to_ebcdic(ebcdic_encoding::to_pseudoascii(c));
    }

    return ret;
}

context::SET_t ca_function::D2A(std::string_view param, const ranged_diagnostic_collector& add_diagnostic)
{
    if (param.empty())
        return 0;

    if (param.size() > 11)
        RET_ERRPARM;

    int res;

    auto it = std::find_if(param.begin(), param.end(), [](int c) { return c != '-' && c != '+'; });

    if (it - param.begin() > 1)
        RET_ERRPARM;

    size_t start = param.front() == '+' ? 1 : 0;

    auto conv = std::from_chars(param.data() + start, param.data() + param.size(), res, 10);

    if (conv.ec != std::errc() || conv.ptr != param.data() + param.size())
        RET_ERRPARM;

    return res;
}

context::SET_t ca_function::DCLEN(const context::C_t& param)
{
    context::A_t ret = 0;
    for (const char* c = param.c_str(); c != param.c_str() + param.size(); ++c)
    {
        if ((*c == '\'' && *(c + 1) == '\'') || (*c == '&' && *(c + 1) == '&'))
            ++c;
        ++ret;
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

context::SET_t ca_function::ISBIN(const context::C_t& param, const ranged_diagnostic_collector& add_diagnostic)
{
    if (param.empty())
        RET_ERRPARM;

    if (param.size() <= 32 && std ::all_of(param.cbegin(), param.cend(), [](char c) { return c == '0' || c == '1'; }))
        return (context::A_t)1;
    return (context::A_t)0;
}

context::SET_t ca_function::ISDEC(const context::C_t& param, const ranged_diagnostic_collector& add_diagnostic)
{
    if (param.empty())
        RET_ERRPARM;

    context::A_t ret;

    if (param.size() > 10 || param.front() == '-')
        ret = 0;
    else
    {
        context::A_t tmp;
        auto conv = std::from_chars(param.c_str(), param.c_str() + param.size(), tmp, 10);

        if (conv.ec != std::errc() || conv.ptr != param.c_str() + param.size())
            ret = 0;
        else
            ret = 1;
    }
    return ret;
}

context::SET_t ca_function::ISHEX(const context::C_t& param, const ranged_diagnostic_collector& add_diagnostic)
{
    if (param.empty())
        RET_ERRPARM;

    if (param.size() <= 8 && std::all_of(param.cbegin(), param.cend(), [](char c) { return std::isxdigit(c); }))
        return (context::A_t)1;
    return (context::A_t)0;
}

context::SET_t ca_function::ISSYM(const context::C_t& param, const ranged_diagnostic_collector& add_diagnostic)
{
    if (param.empty())
        RET_ERRPARM;

    if (!std::isdigit(param.front()) && param.size() < 64
        && std::all_of(param.cbegin(), param.cend(), lexing::lexer::ord_char))
        return 1;
    return 0;
}

context::SET_t ca_function::X2A(std::string_view param, const ranged_diagnostic_collector& add_diagnostic)
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
    std::uint32_t sign_mask = 1U << 31;
    std::uint32_t char_mask = 0xffU << 3 * 8;

    std::string ret;
    ret.reserve(4);

    for (size_t i = 0; i < 4; ++i)
    {
        auto sign = param & sign_mask;
        auto rest = param & char_mask;
        auto c = (unsigned char)(rest >> (3 - i) * 8);
        c |= sign >> (3 - i) * 8;

        ret.append(ebcdic_encoding::to_ascii(c));

        sign_mask >>= 8;
        char_mask >>= 8;
    }
    return ret;
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
    std::stringstream stream;
    stream << std::setfill('0') << std::setw(8) << std::uppercase << std::hex << param;
    return stream.str();
}

context::SET_t ca_function::B2C(const context::C_t& param, const ranged_diagnostic_collector& add_diagnostic)
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

context::SET_t ca_function::B2D(const context::C_t& param, const ranged_diagnostic_collector& add_diagnostic)
{
    auto tmp = B2A(param, add_diagnostic);
    if (tmp.type == context::SET_t_enum::UNDEF_TYPE)
        return tmp;
    return A2D(tmp.access_a());
}

context::SET_t ca_function::B2X(const context::C_t& param, const ranged_diagnostic_collector& add_diagnostic)
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

context::SET_t ca_function::BYTE(context::A_t param, const ranged_diagnostic_collector& add_diagnostic)
{
    if (param > 255 || param < 0)
        RET_ERRPARM;
    else
        return ebcdic_encoding::to_ascii((unsigned char)param);
}

context::SET_t ca_function::C2B(const context::C_t& param, const ranged_diagnostic_collector& add_diagnostic)
{
    if (param.empty())
        return "";

    if (param.size() * 8 > ca_string::MAX_STR_SIZE)
        RET_ERRPARM;

    std::string ret;
    ret.reserve(param.size() * 8);
    for (const char* c = param.c_str(); c != param.c_str() + param.size(); ++c)
    {
        auto value = ebcdic_encoding::to_ebcdic(ebcdic_encoding::to_pseudoascii(c));
        ret.append(std::bitset<8>(value).to_string());
    }
    return ret;
}

context::SET_t ca_function::C2D(const context::C_t& param, const ranged_diagnostic_collector& add_diagnostic)
{
    auto tmp = C2A(param, add_diagnostic);
    if (tmp.type == context::SET_t_enum::UNDEF_TYPE)
        return tmp;
    return A2D(tmp.access_a());
}

context::SET_t ca_function::C2X(const context::C_t& param, const ranged_diagnostic_collector& add_diagnostic)
{
    if (param.empty())
        return "";

    if (param.size() * 2 > ca_string::MAX_STR_SIZE)
        RET_ERRPARM;

    std::string ret;
    ret.reserve(param.size() * 2);
    for (const char* c = param.c_str(); c != param.c_str() + param.size(); ++c)
    {
        int value = ebcdic_encoding::to_ebcdic(ebcdic_encoding::to_pseudoascii(c));

        std::stringstream stream;
        stream << std::setfill('0') << std::setw(2) << std::uppercase << std::hex << value;
        ret.append(stream.str());
    }
    return ret;
}

context::SET_t ca_function::D2B(const context::C_t& param, const ranged_diagnostic_collector& add_diagnostic)
{
    if (param.empty())
        return "";

    auto tmp = D2A(param, add_diagnostic);
    if (tmp.type == context::SET_t_enum::UNDEF_TYPE)
        return tmp;
    return A2B(tmp.access_a());
}

context::SET_t ca_function::D2C(const context::C_t& param, const ranged_diagnostic_collector& add_diagnostic)
{
    if (param.empty())
        RET_ERRPARM;

    auto tmp = D2A(param, add_diagnostic);
    if (tmp.type == context::SET_t_enum::UNDEF_TYPE)
        return tmp;
    return A2C(tmp.access_a());
}

context::SET_t ca_function::D2X(const context::C_t& param, const ranged_diagnostic_collector& add_diagnostic)
{
    if (param.empty())
        RET_ERRPARM;

    auto tmp = D2A(param, add_diagnostic);
    if (tmp.type == context::SET_t_enum::UNDEF_TYPE)
        return tmp;
    return A2X(tmp.access_a());
}

context::SET_t ca_function::DCVAL(const context::C_t& param)
{
    std::string ret;

    for (const char* c = param.c_str(); c != param.c_str() + param.size(); ++c)
    {
        if ((*c == '\'' && *(c + 1) == '\'') || (*c == '&' && *(c + 1) == '&'))
            ++c;
        ret.push_back(*c);
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
        param.erase(param.end() - 1);

    return param;
}

context::SET_t ca_function::DOUBLE(const context::C_t& param, const ranged_diagnostic_collector& add_diagnostic)
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
    std::transform(param.begin(), param.end(), param.begin(), [](char c) { return (char)tolower(c); });
    return param;
}

context::SET_t ca_function::SIGNED(context::A_t param) { return std::to_string(param); }

context::SET_t ca_function::SYSATTRA(const context::C_t&) { return context::SET_t(); }

context::SET_t ca_function::SYSATTRP(const context::C_t&) { return context::SET_t(); }

context::SET_t ca_function::UPPER(context::C_t param)
{
    std::transform(param.begin(), param.end(), param.begin(), [](char c) { return (char)toupper(c); });
    return param;
}

context::SET_t ca_function::X2B(const context::C_t& param, const ranged_diagnostic_collector& add_diagnostic)
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
        if (std::isxdigit(*c))
            std::from_chars(c, c + 1, value, 16);
        else
            RET_ERRPARM;

        ret.append(std::bitset<4>(value).to_string());
    }
    return ret;
}

context::SET_t ca_function::X2C(const context::C_t& param, const ranged_diagnostic_collector& add_diagnostic)
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
        if (std::isxdigit(*c))
            std::from_chars(c, c + 2, value, 16);
        else
            RET_ERRPARM;

        ret.append(ebcdic_encoding::to_ascii(value));
    }
    return ret;
}

context::SET_t ca_function::X2D(const context::C_t& param, const ranged_diagnostic_collector& add_diagnostic)
{
    auto tmp = X2A(param, add_diagnostic);
    if (tmp.type == context::SET_t_enum::UNDEF_TYPE)
        return tmp;
    return A2D(tmp.access_a());
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
