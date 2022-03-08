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

// This file contains definitions of binary and unary operators
// used in machine expressions. Implements mach_expression interface

#ifndef HLASMPLUGIN_PARSERLIBRARY_MACH_OPERATOR_H
#define HLASMPLUGIN_PARSERLIBRARY_MACH_OPERATOR_H

#include "mach_expression.h"

namespace hlasm_plugin::parser_library::expressions {

// Represents a binary operator in machine expression. Holds its
// left and right operand. Templated by actual operator, one of:
// add, sub, mul, div
template<typename T>
class mach_expr_binary final : public mach_expression
{
    bool do_is_similar(const mach_expression& expr) const override;

    mach_expr_ptr left_;
    mach_expr_ptr right_;

public:
    mach_expr_binary(mach_expr_ptr left, mach_expr_ptr right, range rng)
        : mach_expression(rng)
        , left_(assign_expr(std::move(left), rng))
        , right_(assign_expr(std::move(right), rng))
    {
        // text = left_->move_text() + T::sign_char() + right_->move_text();
    }

    context::dependency_collector get_dependencies(context::dependency_solver& info) const override;

    value_t evaluate(context::dependency_solver& info) const override;

    void apply(mach_expr_visitor& visitor) const override
    {
        left_->apply(visitor);
        right_->apply(visitor);
    }

    const mach_expression* leftmost_term() const override { return left_->leftmost_term(); }

    void collect_diags() const override
    {
        collect_diags_from_child(*left_);
        collect_diags_from_child(*right_);
    }

    size_t hash() const override;

    mach_expr_ptr clone() const override
    {
        return std::make_unique<mach_expr_binary<T>>(left_->clone(), right_->clone(), get_range());
    }
};

// Represents a unart operator in machine expression. Holds its
// operand. Templated by actual operator, either unary minus or
// parentheses
template<typename T>
class mach_expr_unary final : public mach_expression
{
    bool do_is_similar(const mach_expression& expr) const override;

    mach_expr_ptr child_;

public:
    mach_expr_unary(mach_expr_ptr child, range rng)
        : mach_expression(rng)
        , child_(assign_expr(std::move(child), rng))
    {
        // text = T::sign_char_begin() + child_->move_text() + T::sign_char_end();
    }

    context::dependency_collector get_dependencies(context::dependency_solver& info) const override;

    value_t evaluate(context::dependency_solver& info) const override;

    void apply(mach_expr_visitor& visitor) const override { child_->apply(visitor); }

    const mach_expression* leftmost_term() const override { return child_->leftmost_term(); }

    void collect_diags() const override { collect_diags_from_child(*child_); }

    size_t hash() const override;

    mach_expr_ptr clone() const override { return std::make_unique<mach_expr_unary<T>>(child_->clone(), get_range()); }
};

struct add
{
    static std::string sign_char() { return "+"; }
    static std::string sign_char_begin() { return "+"; }
    static std::string sign_char_end() { return ""; }
};

struct sub
{
    static std::string sign_char() { return "-"; }
    static std::string sign_char_begin() { return "-"; }
    static std::string sign_char_end() { return ""; }
};

struct rel_addr
{
    static std::string sign_char() { return "-"; }
    static std::string sign_char_begin() { return "-"; }
    static std::string sign_char_end() { return ""; }
};

struct mul
{
    static std::string sign_char() { return "*"; }
};

struct div
{
    static std::string sign_char() { return "/"; }
};

struct par
{
    static std::string sign_char_begin() { return "("; }
    static std::string sign_char_end() { return ")"; }
};

} // namespace hlasm_plugin::parser_library::expressions

#endif