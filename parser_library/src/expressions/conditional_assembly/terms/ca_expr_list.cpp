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
#include <stack>

#include "../ca_operator_binary.h"
#include "../ca_operator_unary.h"
#include "ca_function.h"
#include "ca_symbol.h"
#include "expressions/conditional_assembly/ca_expr_visitor.h"

namespace hlasm_plugin::parser_library::expressions {

ca_expr_list::ca_expr_list(std::vector<ca_expr_ptr> expr_list, range expr_range, bool parenthesized)
    : ca_expression(context::SET_t_enum::UNDEF_TYPE, std::move(expr_range))
    , expr_list(std::move(expr_list))
    , parenthesized(parenthesized)
{}

bool ca_expr_list::get_undefined_attributed_symbols(
    std::vector<context::id_index>& symbols, const evaluation_context& eval_ctx) const
{
    bool result = false;
    for (auto&& expr : expr_list)
        result |= expr->get_undefined_attributed_symbols(symbols, eval_ctx);
    return result;
}

bool is_symbol(const ca_expr_ptr& expr) { return dynamic_cast<const ca_symbol*>(expr.get()) != nullptr; }

std::string_view get_symbol(const ca_expr_ptr& expr)
{
    return dynamic_cast<const ca_symbol&>(*expr).symbol.to_string_view();
}

void ca_expr_list::resolve_expression_tree(ca_expression_ctx expr_ctx, diagnostic_op_consumer& diags)
{
    expr_kind = expr_ctx.kind;

    if (expr_kind == context::SET_t_enum::A_TYPE || expr_kind == context::SET_t_enum::B_TYPE)
        unknown_functions_to_operators();

    if (expr_kind == context::SET_t_enum::A_TYPE)
        resolve<context::A_t>(expr_ctx, diags);
    else if (expr_kind == context::SET_t_enum::B_TYPE)
        resolve<context::B_t>(expr_ctx, diags);
    else if (expr_kind == context::SET_t_enum::C_TYPE)
        resolve<context::C_t>(expr_ctx, diags);
    else
        assert(false);
}

bool ca_expr_list::is_character_expression(character_expression_purpose purpose) const
{
    return purpose == character_expression_purpose::assignment && expr_list.size() == 1
        && expr_list.front()->is_character_expression(purpose);
}

void ca_expr_list::apply(ca_expr_visitor& visitor) const { visitor.visit(*this); }

context::SET_t ca_expr_list::evaluate(const evaluation_context& eval_ctx) const
{
    assert(expr_list.size() <= 1);
    return expr_list.size() == 1 ? expr_list.front()->evaluate(eval_ctx) : context::SET_t(expr_kind);
}

void ca_expr_list::unknown_functions_to_operators()
{
    for (int idx = (int)expr_list.size() - 1; idx >= 0; --idx)
    {
        if (auto expr_func = dynamic_cast<ca_function*>(expr_list[idx].get());
            expr_func && expr_func->function == ca_expr_funcs::UNKNOWN && expr_func->parameters.size() == 1)
        {
            auto holder = std::move(expr_list[idx]);
            auto& true_func = dynamic_cast<ca_function&>(*holder);
            if (true_func.duplication_factor)
            {
                auto expr_r = true_func.duplication_factor->expr_range;
                expr_list[idx] = std::make_unique<ca_par_operator>(std::move(true_func.duplication_factor), expr_r);

                expr_r = true_func.parameters.front()->expr_range;
                expr_list.insert(expr_list.begin() + idx + 1,
                    std::make_unique<ca_par_operator>(std::move(true_func.parameters.front()), expr_r));

                expr_r = true_func.expr_range;
                expr_list.insert(
                    expr_list.begin() + idx + 1, std::make_unique<ca_symbol>(true_func.function_name, expr_r));
            }
            else
            {
                auto expr_r = true_func.expr_range;
                expr_list[idx] = std::make_unique<ca_symbol>(true_func.function_name, expr_r);

                expr_r = true_func.parameters.front()->expr_range;
                expr_list.insert(expr_list.begin() + idx + 1,
                    std::make_unique<ca_par_operator>(std::move(true_func.parameters.front()), expr_r));
            }
        }
    }
}

std::span<const ca_expr_ptr> ca_expr_list::expression_list() const { return expr_list; }

namespace {

struct term_entry
{
    ca_expr_ptr term;
    size_t i;
};
struct op_entry
{
    size_t i;
    ca_expr_ops op_type;
    int priority;
    bool binary;
    bool right_assoc;
    range r;
    ca_expr_funcs function;
};

template<typename T>
struct resolve_stacks
{
    using expr_policy = typename ca_expr_traits<T>::policy_t;

    std::stack<term_entry> terms;
    std::stack<op_entry> op_stack;

    void push_term(term_entry t) { terms.push(std::move(t)); }
    void push_op(op_entry op) { op_stack.push(std::move(op)); }

    bool reduce_binary(const op_entry& op, diagnostic_op_consumer& diags)
    {
        auto right = std::move(terms.top());
        terms.pop();
        auto left = std::move(terms.top());
        terms.pop();

        if (left.i > op.i)
        {
            diags.add_diagnostic(diagnostic_op::error_CE003(range(op.r.start)));
            return false;
        }
        if (right.i < op.i)
        {
            diags.add_diagnostic(diagnostic_op::error_CE003(range(op.r.end)));
            return false;
        }

        if (op.function == ca_expr_funcs::UNKNOWN)
            terms.push({ std::make_unique<ca_function_binary_operator>(std::move(left.term),
                             std::move(right.term),
                             op.op_type,
                             context::object_traits<T>::type_enum,
                             op.r),
                left.i });
        else
        {
            std::vector<ca_expr_ptr> function_params;
            function_params.push_back(std::move(left.term));
            function_params.push_back(std::move(right.term));
            terms.push({ std::make_unique<ca_function>(
                             context::id_index(), op.function, std::move(function_params), nullptr, op.r),
                left.i });
        }

        return true;
    }

    bool reduce_unary(const op_entry& op, diagnostic_op_consumer& diags)
    {
        auto right = std::move(terms.top());
        terms.pop();

        if (right.i < op.i)
        {
            diags.add_diagnostic(diagnostic_op::error_CE003(range(op.r.end)));
            return false;
        }

        terms.push({ std::make_unique<ca_function_unary_operator>(
                         std::move(right.term), op.op_type, expr_policy::set_type, op.r),
            op.i });
        return true;
    }

    bool reduce_stack_entry(diagnostic_op_consumer& diags)
    {
        auto op = std::move(op_stack.top());
        op_stack.pop();
        if (terms.size() < 1 + op.binary)
        {
            diags.add_diagnostic(diagnostic_op::error_CE003(
                range(terms.size() < static_cast<size_t>(op.binary) ? op.r.start : op.r.end)));
            return false;
        }
        return op.binary ? reduce_binary(op, diags) : reduce_unary(op, diags);
    }

    bool reduce_stack(diagnostic_op_consumer& diags, int prio_limit, bool right_assoc)
    {
        while (!op_stack.empty())
        {
            if (op_stack.top().priority + right_assoc > prio_limit)
                break;
            if (!reduce_stack_entry(diags))
                return false;
        }
        return true;
    }

    bool reduce_stack_all(diagnostic_op_consumer& diags)
    {
        while (!op_stack.empty())
        {
            if (!reduce_stack_entry(diags))
                return false;
        }
        return true;
    }
};

} // namespace

template<typename T>
void ca_expr_list::resolve(ca_expression_ctx expr_ctx, diagnostic_op_consumer& diags)
{
    using expr_policy = typename ca_expr_traits<T>::policy_t;

    resolve_stacks<T> stacks;

    auto i = (size_t)-1;
    bool prefer_next_term = true;
    for (auto& curr_expr : expr_list)
    {
        ++i;
        if (is_symbol(curr_expr))
        {
            const auto symbol = get_symbol(curr_expr);
            auto op_type_var = expr_policy::get_operator_properties(symbol);
            if (std::holds_alternative<std::monostate>(op_type_var))
            {
                // fallback to term
            }
            else if (std::holds_alternative<invalid_by_policy>(op_type_var))
            {
                if (!prefer_next_term)
                {
                    diags.add_diagnostic(diagnostic_op::error_CE002(symbol, curr_expr->expr_range));
                    expr_list.clear();
                    return;
                }
                // fallback to term
            }
            else if (const auto& op_type = std::get<ca_expr_op>(op_type_var);
                     !(prefer_next_term && op_type.binary)) // ... AND AND is interpreted as AND term,
                                                            // ... AND NOT ... is apparently not
            {
                if (op_type.binary && !stacks.reduce_stack(diags, op_type.priority, op_type.right_assoc))
                {
                    expr_list.clear();
                    return;
                }
                stacks.push_op({ i,
                    op_type.op,
                    op_type.priority,
                    op_type.binary,
                    op_type.right_assoc,
                    curr_expr->expr_range,
                    ca_common_expr_policy::get_function(symbol) });
                prefer_next_term = op_type.binary;
                continue;
            }
        }
        stacks.push_term({ std::move(curr_expr), i });
        prefer_next_term = false;
    }

    if (!stacks.reduce_stack_all(diags))
    {
        expr_list.clear();
        return;
    }
    if (stacks.terms.empty())
    {
        diags.add_diagnostic(diagnostic_op::error_CE003(expr_range));
        expr_list.clear();
        return;
    }
    if (stacks.terms.size() > 1)
    {
        diags.add_diagnostic(diagnostic_op::error_CE001(range(stacks.terms.top().term->expr_range)));
        expr_list.clear();
        return;
    }

    ca_expr_ptr final_expr = std::move(stacks.terms.top().term);

    // resolve created tree
    expr_ctx.kind = context::object_traits<T>::type_enum;
    final_expr->resolve_expression_tree(expr_ctx, diags);

    // move resolved tree to the front of the array
    expr_list.clear();
    expr_list.emplace_back(std::move(final_expr));
}

} // namespace hlasm_plugin::parser_library::expressions
