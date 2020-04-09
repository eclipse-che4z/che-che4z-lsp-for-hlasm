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

#include "expression.h"

#include <algorithm>
#include <assert.h>
#include <exception>
#include <stack>

#include "arithmetic_expression.h"
#include "character_expression.h"
#include "diagnosable.h"
#include "keyword_expression.h"
#include "logic_expression.h"
#include "visitors/expression_evaluator.h"

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::expressions;

void expression::copy_diag(expr_ref o) { diag = std::make_unique<diagnostic_op>(*o->diag); }

void expression::copy_diag(const expression* o) { diag = std::make_unique<diagnostic_op>(*o->diag); }

void expression::copy_diag(const expression& o) { diag = std::make_unique<diagnostic_op>(*o.diag); }

expr_ptr expression::binary_operation(str_ref, expr_ref) const
{
    return default_expr_with_error<arithmetic_expression>(error_messages::e001());
}

expr_ptr expression::unary_operation(str_ref) const
{
    return default_expr_with_error<arithmetic_expression>(error_messages::e001());
}

expr_ptr expression::resolve_ord_symbol(str_ref symbol)
{
    if (keyword_expression::is_keyword(symbol))
        return std::make_unique<keyword_expression>(symbol);

    // TODO: resolve identifier
    if (std::all_of(symbol.cbegin(), symbol.cend(), [](char c) { return isdigit(c); }))
        return arithmetic_expression::from_string(symbol, 10);



    return default_expr_with_error<arithmetic_expression>(error_messages::not_implemented());
}

#define front_keyword(exprs) dynamic_cast<keyword_expression*>(exprs.front().get())

expr_ptr expression::evaluate(std::deque<expr_ptr> exprs)
{
    size_t operator_count = 0;
    auto e = evaluate_term(exprs, 5, operator_count);

    if (exprs.size() > 0)
        return default_expr_with_error<arithmetic_expression>(error_messages::e001());

    return e;
}

expr_ptr expression::evaluate_term(std::deque<expr_ptr>& exprs, uint8_t priority, size_t& operator_count)
{
    expr_ptr a1;
    if (priority == 1)
        a1 = evaluate_factor(exprs, operator_count);
    else
        a1 = evaluate_term(exprs, priority - 1, operator_count);

    if (exprs.size() > 0 && exprs.front()->is_keyword() && front_keyword(exprs)->priority() == priority)
    {
        auto o = std::move(exprs.front());
        exprs.pop_front();

        std::string keyword(o->get_str_val());
        if (dynamic_cast<logic_expression*>(a1.get()) && o->is_complex_keyword() && exprs.front()->is_keyword()
            && exprs.front()->get_str_val() == "NOT")
        {
            exprs.pop_front();
            keyword.append(" NOT");
        }
        /*
        ++operator_count;
        //maximum operator count, see reference
        if (operator_count > 24)
                return default_expr_with_error<arithmetic_expression>
                (error_messages::e001());
        */
        auto a2 = evaluate_term(exprs, priority, operator_count);
        return a1->binary_operation(keyword, a2);
    }
    return a1;
}

expr_ptr expression::evaluate_factor(std::deque<expr_ptr>& exprs, size_t& operator_count)
{
    if (exprs.size() == 0)
        return default_expr_with_error<arithmetic_expression>(error_messages::e001());

    // value
    if (!exprs.front()->is_keyword() || (exprs.front()->is_keyword() && exprs.size() == 1))
    {
        auto e = std::move(exprs.front());
        exprs.pop_front();
        if (!e->is_keyword())
            return e;
        return dynamic_cast<keyword_expression*>(e.get())->to_expression();
    }

    // unary operator factor
    if (exprs.front()->is_keyword() && dynamic_cast<keyword_expression*>(exprs.front().get())->is_unary())
    {
        ++operator_count;
        if (operator_count > 24)
            return default_expr_with_error<arithmetic_expression>(error_messages::e002());
        auto op = std::move(exprs.front());
        exprs.pop_front();
        auto e = evaluate_factor(exprs, operator_count);
        return e->unary_operation(op->get_str_val());
    }

    return default_expr_with_error<arithmetic_expression>(error_messages::e001());
}

expr_ptr expression::operator+(expression_ref) const
{
    return default_expr_with_error<arithmetic_expression>(error_messages::e001());
}

expr_ptr expression::operator-(expression_ref) const
{
    return default_expr_with_error<arithmetic_expression>(error_messages::e001());
}

expr_ptr expression::operator*(expression_ref) const
{
    return default_expr_with_error<arithmetic_expression>(error_messages::e001());
}

expr_ptr expression::operator/(expression_ref) const
{
    return default_expr_with_error<arithmetic_expression>(error_messages::e001());
}

expr_ptr expression::operator|(expression_ref) const
{
    return default_expr_with_error<arithmetic_expression>(error_messages::e001());
}

expr_ptr expression::operator&(expression_ref) const
{
    return default_expr_with_error<arithmetic_expression>(error_messages::e001());
}

expr_ptr expression::operator+() const
{
    return default_expr_with_error<arithmetic_expression>(error_messages::e001());
}

expr_ptr expression::operator-() const
{
    return default_expr_with_error<arithmetic_expression>(error_messages::e001());
}

context::SET_t expression::get_set_value() const { return context::SET_t(); }

int32_t expression::get_numeric_value() const
{
    auto t = retype<arithmetic_expression>();
    if (t == NULL)
    {
        auto tl = retype<logic_expression>();
        if (tl == NULL)
            return 0;
        return static_cast<int32_t>(tl->get_value());
    }
    else
        return t->get_value();
}

expr_ptr expression::self_defining_term(str_ref type, str_ref val, bool dbcs)
{
    assert(type.length() > 0);
    char t = static_cast<char>(toupper(type[0]));

    if (t == 'D' || t == 'B' || t == 'X' || t == 'C' || t == 'D' || t == 'G')
        return arithmetic_expression::from_string(type, val, dbcs);

    return default_expr_with_error<arithmetic_expression>(error_messages::e003());
}
