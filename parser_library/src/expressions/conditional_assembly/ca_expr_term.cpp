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

#include "ca_expr_term.h"

#include <charconv>
#include <iomanip>
#include <sstream>

#include "ca_expr_policy.h"
#include "ca_operator_binary.h"
#include "ca_operator_unary.h"
#include "ebcdic_encoding.h"
#include "lexing/lexer.h"
#include "processing/context_manager.h"
#include "semantics/concatenation_term.h"

namespace hlasm_plugin {
namespace parser_library {
namespace expressions {

ca_expr_list::ca_expr_list(std::vector<ca_expr_ptr> expr_list, range expr_range)
    : ca_expression(context::SET_t_enum::UNDEF_TYPE, std::move(expr_range))
    , expr_list(std::move(expr_list))
{ }

undef_sym_set ca_expr_list::get_undefined_attributed_symbols(const context::dependency_solver& solver) const
{
    undef_sym_set tmp;
    for (auto&& expr : expr_list)
        tmp.merge(expr->get_undefined_attributed_symbols(solver));
    return tmp;
}

bool is_symbol(const ca_expr_ptr& expr) { return static_cast<const ca_symbol*>(expr.get()) != nullptr; }

const std::string& get_symbol(const ca_expr_ptr& expr) { return *static_cast<const ca_symbol*>(expr.get())->symbol; }

void ca_expr_list::resolve_expression_tree(context::SET_t_enum kind)
{
    expr_kind = kind;

    if (kind == context::SET_t_enum::A_TYPE)
        resolve<context::A_t>();
    else if (kind == context::SET_t_enum::B_TYPE)
        resolve<context::B_t>();
    else if (kind == context::SET_t_enum::C_TYPE)
        resolve<context::C_t>();
    else
        assert(false);
}

void ca_expr_list::collect_diags() const
{
    for (auto&& expr : expr_list)
        collect_diags_from_child(*expr);
}

bool ca_expr_list::is_character_expression() const { return false; }

context::SET_t ca_expr_list::evaluate(evaluation_context&) const
{
    assert(false);
    return context::SET_t();
}

template<typename T> void ca_expr_list::resolve()
{
    if (expr_list.empty())
    {
        add_diagnostic(diagnostic_op::error_CE003(expr_range));
        return;
    }

    size_t it = 0;
    bool err = false;

    ca_expr_ptr final_expr = retrieve_term<ca_expr_traits<T>::policy_t>(it, 0);
    err |= final_expr == nullptr;

    while (it == expr_list.size() && !err)
    {
        auto op_range = expr_list[it]->expr_range;

        auto [prio, op_type] = retrieve_binary_operator<ca_expr_traits<T>::policy_t>(it, err);

        auto r_expr = retrieve_term<ca_expr_traits<T>::policy_t>(++it, prio);
        err |= r_expr == nullptr;

        final_expr = std::make_unique<ca_function_binary_operator>(
            std::move(final_expr), std::move(r_expr), op_type, context::object_traits<T>::type_enum, op_range);
    }

    if (err)
    {
        expr_list.clear();
        return;
    }

    // resolve created tree
    final_expr->resolve_expression_tree(context::object_traits<T>::type_enum);

    // move resolved tree to the front of the array
    expr_list.clear();
    expr_list.emplace_back(std::move(final_expr));
}

template<typename EXPR_POLICY> ca_expr_ptr ca_expr_list::retrieve_term(size_t& it, int priority)
{
    // list is exhausted
    if (it == expr_list.size())
    {
        auto r = expr_list[it - 1]->expr_range;
        r.start = r.end;
        r.end.column++;
        add_diagnostic(diagnostic_op::error_CE003(r));
        return nullptr;
    }

    // first possible term
    auto& curr_expr = expr_list[it];

    // is unary op
    if (is_symbol(curr_expr))
    {
        if (auto op_type = EXPR_POLICY::get_operator(get_symbol(curr_expr)); EXPR_POLICY::is_unary(op_type))
        {
            auto new_expr = retrieve_term<EXPR_POLICY>(++it, EXPR_POLICY::get_priority(op_type));
            return std::make_unique<ca_function_unary_operator>(
                std::move(new_expr), op_type, EXPR_POLICY::set_type, curr_expr->expr_range);
        }
    }

    // is only term
    if (it + 1 == expr_list.size())
        return std::move(expr_list[it++]);

    // tries to get binary operator
    auto op_it = it + 1;
    auto op_range = expr_list[op_it]->expr_range;
    bool err = false;

    auto [op_prio, op_type] = retrieve_binary_operator<EXPR_POLICY>(op_it, err);
    if (err)
        return nullptr;

    // if operator is of lower priority than the calling operator, finish
    if (op_prio >= priority)
        return std::move(curr_expr);
    else
        it = op_it;

    auto right_expr = retrieve_term<EXPR_POLICY>(++it, op_prio);

    return std::make_unique<ca_function_binary_operator>(
        std::move(curr_expr), std::move(right_expr), op_type, EXPR_POLICY::set_type, op_range);
}

template<typename EXPR_POLICY> std::pair<int, ca_expr_ops> ca_expr_list::retrieve_binary_operator(size_t& it, bool& err)
{
    auto& op = expr_list[it];

    if (!is_symbol(op) || !EXPR_POLICY::is_operator(EXPR_POLICY::get_operator(get_symbol(op))))
    {
        add_diagnostic(diagnostic_op::error_CE001(expr_range));
        err = true;
    }

    ca_expr_ops op_type = EXPR_POLICY::get_operator(get_symbol(expr_list[it]));

    if (EXPR_POLICY::multiple_words(op_type))
    {
        if (is_symbol(expr_list[it + 1]) && get_symbol(expr_list[it + 1]) == "NOT")
        {
            op_type = EXPR_POLICY::get_operator(get_symbol(expr_list[it + 1]) + " NOT");
            ++it;
        }
    }

    auto op_prio = EXPR_POLICY::get_priority(op_type);

    return std::make_pair(op_prio, op_type);
}

ca_string::substring_t::substring_t()
    : start(nullptr)
    , count(nullptr)
    , to_end(false)
    , substring_range()
{ }

ca_string::ca_string(
    semantics::concat_chain value, ca_expr_ptr duplication_factor, substring_t substring, range expr_range)
    : ca_expression(context::SET_t_enum::C_TYPE, std::move(expr_range))
    , value(std::move(value))
    , duplication_factor(std::move(duplication_factor))
    , substring(std::move(substring))
{ }

undef_sym_set ca_string::get_undefined_attributed_symbols(const context::dependency_solver& solver) const
{
    undef_sym_set tmp;
    if (duplication_factor)
        tmp = duplication_factor->get_undefined_attributed_symbols(solver);
    if (substring.start)
        tmp.merge(substring.start->get_undefined_attributed_symbols(solver));
    if (substring.count)
        tmp.merge(substring.count->get_undefined_attributed_symbols(solver));
    return tmp;
}

void ca_string::resolve_expression_tree(context::SET_t_enum kind)
{
    if (expr_kind != kind)
        add_diagnostic(diagnostic_op::error_CE004(expr_range));
    else
    {
        if (duplication_factor)
            duplication_factor->resolve_expression_tree(context::SET_t_enum::A_TYPE);
        if (substring.start)
            substring.start->resolve_expression_tree(context::SET_t_enum::A_TYPE);
        if (substring.count)
            substring.count->resolve_expression_tree(context::SET_t_enum::A_TYPE);
    }
}

void ca_string::collect_diags() const
{
    if (duplication_factor)
        collect_diags_from_child(*duplication_factor);
    if (substring.start)
        collect_diags_from_child(*substring.start);
    if (substring.count)
        collect_diags_from_child(*substring.count);
}

bool ca_string::is_character_expression() const { return duplication_factor == nullptr; }

context::SET_t ca_string::evaluate(evaluation_context& eval_ctx) const
{
    context::C_t str = ""; // evaluate concat chain

    if (str.size() > MAX_STR_SIZE)
    {
        eval_ctx.add_diagnostic(diagnostic_op::error_CE011(expr_range));
        return context::SET_t();
    }

    if (substring.start)
    {
        auto start = substring.start->evaluate(eval_ctx).access_a();
        auto count = substring.count ? substring.count->evaluate(eval_ctx).access_a() : str.size();

        if (start < 0 || count < 0 || (start == 0 && count > 0))
        {
            eval_ctx.add_diagnostic(diagnostic_op::error_CE008(substring.substring_range));
            return context::SET_t();
        }
        if (start > str.size())
        {
            eval_ctx.add_diagnostic(diagnostic_op::error_CE009(substring.start->expr_range));
            return context::SET_t();
        }

        str = str.substr(start - 1, count);
    }

    if (duplication_factor)
    {
        auto dupl = duplication_factor->evaluate(eval_ctx).access_a();

        if (dupl < 0)
        {
            eval_ctx.add_diagnostic(diagnostic_op::error_CE010(duplication_factor->expr_range));
            return context::SET_t();
        }

        if (str.size() * dupl > MAX_STR_SIZE)
        {
            eval_ctx.add_diagnostic(diagnostic_op::error_CE011(expr_range));
            return context::SET_t();
        }

        if (dupl == 0 || str.empty())
            str = "";
        else
        {
            str.reserve(str.size() * dupl);
            auto begin = str.begin();
            auto end = str.end();
            for (auto i = 1; i < dupl; ++i)
                str.append(begin, end);
        }
    }

    return str;
}

ca_var_sym::ca_var_sym(semantics::vs_ptr symbol, range expr_range)
    : ca_expression(context::SET_t_enum::A_TYPE, std::move(expr_range))
    , symbol(std::move(symbol))
{ }

undef_sym_set get_undefined_attributed_symbols_vs(
    const semantics::vs_ptr& symbol, const context::dependency_solver& solver)
{
    undef_sym_set tmp;
    // for (auto&& expr : symbol->subscript)
    //    tmp.merge(expr->get_undefined_attributed_symbols_vs(solver));

    if (symbol->created)
    {
        auto created = symbol->access_created();
        for (auto&& point : created->created_name)
            if (point->type == semantics::concat_type::VAR)
                tmp.merge(get_undefined_attributed_symbols_vs(point->access_var()->symbol, solver));
    }
    return tmp;
}

undef_sym_set ca_var_sym::get_undefined_attributed_symbols(const context::dependency_solver& solver) const
{
    return get_undefined_attributed_symbols_vs(symbol, solver);
}

void ca_var_sym::resolve_expression_tree(context::SET_t_enum)
{
    // auto&& sym = std::get<semantics::vs_ptr>(symbol);
    // for (auto&& expr : sym->subscript)
    //    expr->resolve_expression_tree(context::SET_t_enum::SETA_type);
}

void ca_var_sym::collect_diags() const
{
    // for (auto&& expr : symbol->subscript)
    //    collect_diags_from_child(*expr);
}

bool ca_var_sym::is_character_expression() const { return false; }

context::SET_t ca_var_sym::evaluate(evaluation_context& eval_ctx) const
{
    processing::context_manager mngr(eval_ctx.hlasm_ctx);

    context::id_index id;
    if (!symbol->created)
        id = symbol->access_basic()->name;
    else
    {
        auto str_name = ""; // concatenate(symbol->access_created()->created_name)
        auto [valid, name] = mngr.try_get_symbol_name(str_name, symbol->symbol_range);
        if (!valid)
            return context::SET_t();
        id = name;
    }

    std::vector<context::A_t> subscript;

    // for (const auto& expr : symbol->subscript)
    //    subscript.push_back(expr->evaluate(eval_ctx).access_a());

    context::SET_t value; // = mngr.get_var_sym_value(id, subscript, symbol->symbol_range);

    eval_ctx.collect_diags_from_child(mngr);

    // if (value.type == context::SET_t_enum::C_TYPE)
    //     return (expr_ptr)arithmetic_expression::from_string(value.access_c(), false);
    // else
    return value;
}

ca_constant::ca_constant(context::A_t value, range expr_range)
    : ca_expression(context::SET_t_enum::A_TYPE, std::move(expr_range))
    , value(value)
{ }

undef_sym_set ca_constant::get_undefined_attributed_symbols(const context::dependency_solver&) const
{
    return undef_sym_set();
}

void ca_constant::resolve_expression_tree(context::SET_t_enum kind)
{
    if (kind == context::SET_t_enum::C_TYPE)
        add_diagnostic(diagnostic_op::error_CE004(expr_range));
}

void ca_constant::collect_diags() const { }

bool ca_constant::is_character_expression() const { return false; }

context::SET_t ca_constant::evaluate(evaluation_context&) const { return value; }

ca_symbol::ca_symbol(context::id_index symbol, range expr_range)
    : ca_expression(context::SET_t_enum::A_TYPE, std::move(expr_range))
    , symbol(symbol)
{ }

undef_sym_set ca_symbol::get_undefined_attributed_symbols(const context::dependency_solver&) const
{
    return undef_sym_set();
}

void ca_symbol::resolve_expression_tree(context::SET_t_enum kind)
{
    if (kind == context::SET_t_enum::C_TYPE)
        add_diagnostic(diagnostic_op::error_CE004(expr_range));
}

void ca_symbol::collect_diags() const { }

bool ca_symbol::is_character_expression() const { return false; }

context::SET_t ca_symbol::evaluate(evaluation_context& eval_ctx) const
{
    auto tmp_symbol = eval_ctx.hlasm_ctx.ord_ctx.get_symbol(symbol);

    if (tmp_symbol && tmp_symbol->kind() == context::symbol_value_kind::ABS)
        return tmp_symbol->value().get_abs();
    else
    {
        eval_ctx.add_diagnostic(diagnostic_op::error_CE012(expr_range));
        return context::SET_t();
    }
}

ca_symbol_attribute::ca_symbol_attribute(context::id_index symbol, context::data_attr_kind attribute, range expr_range)
    : ca_expression(attribute == context::data_attr_kind::T ? context::SET_t_enum::C_TYPE : context::SET_t_enum::A_TYPE,
        std::move(expr_range))
    , attribute(attribute)
    , symbol(symbol)

{ }

ca_symbol_attribute::ca_symbol_attribute(semantics::vs_ptr symbol, context::data_attr_kind attribute, range expr_range)
    : ca_expression(attribute == context::data_attr_kind::T ? context::SET_t_enum::C_TYPE : context::SET_t_enum::A_TYPE,
        std::move(expr_range))
    , attribute(attribute)
    , symbol(std::move(symbol))
{ }

undef_sym_set ca_symbol_attribute::get_undefined_attributed_symbols(const context::dependency_solver& solver) const
{
    if (!context::symbol_attributes::ordinary_allowed(attribute))
        return undef_sym_set();

    if (std::holds_alternative<context::id_index>(symbol))
        return { std::get<context::id_index>(symbol) };
    else if (std::holds_alternative<semantics::vs_ptr>(symbol))
        return get_undefined_attributed_symbols_vs(std::get<semantics::vs_ptr>(symbol), solver);
    else
    {
        assert(false);
        return undef_sym_set();
    }
}

void ca_symbol_attribute::resolve_expression_tree(context::SET_t_enum kind)
{
    if (kind == context::SET_t_enum::C_TYPE && kind != expr_kind)
        add_diagnostic(diagnostic_op::error_CE004(expr_range));
    else if (std::holds_alternative<semantics::vs_ptr>(symbol))
    {
        // auto&& sym = std::get<semantics::vs_ptr>(symbol);
        // for (auto&& expr : sym->subscript)
        //    expr->resolve_expression_tree(context::SET_t_enum::SETA_type);
    }
}

void ca_symbol_attribute::collect_diags() const
{
    if (std::holds_alternative<semantics::vs_ptr>(symbol))
    {
        // auto&& sym = std::get<semantics::vs_ptr>(symbol);
        // for (auto&& expr : sym->subscript)
        //    collect_diags_from_child(*expr);
    }
}

bool ca_symbol_attribute::is_character_expression() const { return false; }

context::SET_t ca_symbol_attribute::evaluate(evaluation_context& eval_ctx) const { return context::SET_t(); }

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

#define RET_ERRPARM                                                                                                    \
    do                                                                                                                 \
    {                                                                                                                  \
        eval_ctx.add_diagnostic(diagnostic_op::error_CE007(expr_range));                                               \
        return context::SET_t();                                                                                       \
    } while (0)

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

    auto algn = 0;
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

    auto algn = 0;
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

context::SET_t ca_function::ESYM(context::SET_t param) { return context::SET_t(); }

context::SET_t ca_function::LOWER(context::SET_t param)
{
    std::string& value = param.access_c();
    std::transform(value.begin(), value.end(), value.begin(), [](char c) { return (char)tolower(c); });
    return value;
}

context::SET_t ca_function::SIGNED(context::SET_t param) { return std::to_string(param.access_a()); }

context::SET_t ca_function::SYSATTRA(context::SET_t param) { return context::SET_t(); }

context::SET_t ca_function::SYSATTRP(context::SET_t param) { return context::SET_t(); }

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
    for (auto c = str.c_str(); c != '\0'; ++c)
    {
        unsigned char value;
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
    for (auto c = str.c_str(); c != '\0'; ++c)
    {
        unsigned char value;
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
