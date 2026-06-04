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
        , left_(std::move(left))
        , right_(std::move(right))
    {}

    context::dependency_collector get_dependencies(context::dependency_solver& info) const override;

    value_t evaluate(context::dependency_solver& info, diagnostic_op_consumer& diags) const override;

    value_t equ_evaluate(context::dependency_solver& info) const override;

    void apply(mach_expr_visitor& visitor) const override
    {
        left_->apply(visitor);
        right_->apply(visitor);
    }

    const mach_expression* leftmost_term() const override { return left_->leftmost_term(); }

    size_t hash() const override;

    mach_expr_ptr clone() const override
    {
        return std::make_unique<mach_expr_binary<T>>(left_->clone(), right_->clone(), get_range());
    }

    const mach_expression* left_expression() const { return left_.get(); }
    const mach_expression* right_expression() const { return right_.get(); }

    std::int32_t derive_length(std::int32_t mi_length, context::dependency_solver& solver) const override
    {
        return left_->derive_length(mi_length, solver);
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
        , child_(std::move(child))
    {}

    context::dependency_collector get_dependencies(context::dependency_solver& info) const override;

    value_t evaluate(context::dependency_solver& info, diagnostic_op_consumer& diags) const override;

    value_t equ_evaluate(context::dependency_solver& info) const override;

    void apply(mach_expr_visitor& visitor) const override { child_->apply(visitor); }

    const mach_expression* leftmost_term() const override { return child_->leftmost_term(); }

    size_t hash() const override;

    mach_expr_ptr clone() const override { return std::make_unique<mach_expr_unary<T>>(child_->clone(), get_range()); }

    std::int32_t derive_length(std::int32_t mi_length, context::dependency_solver& solver) const override
    {
        return child_->derive_length(mi_length, solver);
    }
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

// TODO: rel_addr needs to go...
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
