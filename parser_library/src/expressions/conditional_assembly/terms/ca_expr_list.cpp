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

#include "ca_expr_list.h"

#include <cassert>

#include "../ca_operator_binary.h"
#include "../ca_operator_unary.h"
#include "ca_function.h"
#include "ca_symbol.h"

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

bool is_symbol(const ca_expr_ptr& expr) { return dynamic_cast<const ca_symbol*>(expr.get()) != nullptr; }

const std::string& get_symbol(const ca_expr_ptr& expr) { return *dynamic_cast<const ca_symbol*>(expr.get())->symbol; }

void tidy_list(std::vector<ca_expr_ptr>& expr_list)
{
    for (int idx = (int)expr_list.size() - 1; idx >= 0; --idx)
    {
        if (!expr_list[idx])
            expr_list.erase(expr_list.begin() + idx);
    }
}

void ca_expr_list::resolve_expression_tree(context::SET_t_enum kind)
{
    expr_kind = kind;

    tidy_list(expr_list);
    if (kind == context::SET_t_enum::B_TYPE)
        unknown_functions_to_operators();

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

context::SET_t ca_expr_list::evaluate(evaluation_context& eval_ctx) const
{
    assert(expr_list.size() <= 1);

    if (expr_list.empty())
        return context::SET_t(expr_kind);
    return expr_list.front()->evaluate(eval_ctx);
}

void ca_expr_list::unknown_functions_to_operators()
{
    for (int idx = (int)expr_list.size() - 1; idx >= 0; --idx)
    {
        if (auto expr_func = dynamic_cast<ca_function*>(expr_list[idx].get());
            expr_func && expr_func->function == ca_expr_funcs::UNKNOWN && expr_func->parameters.size() == 1)
        {
            auto holder = std::move(expr_list[idx]);
            auto true_func = dynamic_cast<ca_function*>(holder.get());
            if (true_func->duplication_factor)
            {
                auto expr_r = true_func->duplication_factor->expr_range;
                expr_list[idx] =
                    std::make_unique<ca_par_operator>(std::move(true_func->duplication_factor), expr_r);

                expr_r = true_func->parameters.front()->expr_range;
                expr_list.insert(expr_list.begin() + idx + 1,
                    std::make_unique<ca_par_operator>(std::move(true_func->parameters.front()), expr_r));

                expr_r = true_func->expr_range;
                expr_list.insert(
                    expr_list.begin() + idx + 1, std::make_unique<ca_symbol>(true_func->function_name, expr_r));
            }
            else
            {
                auto expr_r = true_func->expr_range;
                expr_list[idx]= std::make_unique<ca_symbol>(true_func->function_name, expr_r);

                expr_r = true_func->parameters.front()->expr_range;
                expr_list.insert(expr_list.begin() + idx + 1,
                    std::make_unique<ca_par_operator>(std::move(true_func->parameters.front()), expr_r));
            }
        }
    }
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

    ca_expr_ptr final_expr = retrieve_term<typename ca_expr_traits<T>::policy_t>(it, 0);
    err |= final_expr == nullptr;

    while (it != expr_list.size() && !err)
    {
        auto op_range = expr_list[it]->expr_range;

        auto [prio, op_type] = retrieve_binary_operator<typename ca_expr_traits<T>::policy_t>(it, err);

        auto r_expr = retrieve_term<typename ca_expr_traits<T>::policy_t>(++it, prio);
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
    auto op_it = ++it;
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
        return std::make_pair(0, ca_expr_ops::UNKNOWN);
    }

    ca_expr_ops op_type = EXPR_POLICY::get_operator(get_symbol(expr_list[it]));

    if (EXPR_POLICY::multiple_words(op_type))
    {
        if (is_symbol(expr_list[it + 1]) && get_symbol(expr_list[it + 1]) == "NOT")
        {
            op_type = EXPR_POLICY::get_operator(get_symbol(expr_list[it]) + "_NOT");
            ++it;
        }
    }

    auto op_prio = EXPR_POLICY::get_priority(op_type);

    return std::make_pair(op_prio, op_type);
}

} // namespace expressions
} // namespace parser_library
} // namespace hlasm_plugin
