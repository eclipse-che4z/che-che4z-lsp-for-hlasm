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

    context::dependency_collector get_dependencies(mach_evaluate_info info) const override;

    value_t evaluate(mach_evaluate_info info) const override;

    void fill_location_counter(context::address addr) override
    {
        left_->fill_location_counter(addr);
        right_->fill_location_counter(std::move(addr));
    }

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
};

// Represents a unart operator in machine expression. Holds its
// operand. Templated by actual operator, either unary minus or
// parentheses
template<typename T>
class mach_expr_unary final : public mach_expression
{
    mach_expr_ptr child_;

public:
    mach_expr_unary(mach_expr_ptr child, range rng)
        : mach_expression(rng)
        , child_(assign_expr(std::move(child), rng))
    {
        // text = T::sign_char_begin() + child_->move_text() + T::sign_char_end();
    }

    context::dependency_collector get_dependencies(mach_evaluate_info info) const override;

    value_t evaluate(mach_evaluate_info info) const override;

    void fill_location_counter(context::address addr) override { child_->fill_location_counter(std::move(addr)); }

    void apply(mach_expr_visitor& visitor) const override { child_->apply(visitor); }

    const mach_expression* leftmost_term() const override { return child_->leftmost_term(); }

    void collect_diags() const override { collect_diags_from_child(*child_); }
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



template<>
inline mach_expression::value_t mach_expr_binary<add>::evaluate(mach_evaluate_info info) const
{
    return left_->evaluate(info) + right_->evaluate(info);
}

template<>
inline mach_expression::value_t mach_expr_binary<sub>::evaluate(mach_evaluate_info info) const
{
    return left_->evaluate(info) - right_->evaluate(info);
}
template<>
inline mach_expression::value_t mach_expr_binary<rel_addr>::evaluate(mach_evaluate_info info) const
{
    auto lc = left_->evaluate(info);
    auto symb = right_->evaluate(info);
    if (symb.value_kind() == context::symbol_value_kind::ABS)
    {
        add_diagnostic(diagnostic_op::warn_D031(get_range(), std::to_string(right_->evaluate(info).get_abs())));
        return right_->evaluate(info);
    }
    else
    {
        if (lc.get_reloc().offset() > symb.get_reloc().offset())
            return (left_->evaluate(info) - right_->evaluate(info));
        else
            return (right_->evaluate(info) - left_->evaluate(info));
    }
}
template<>
inline mach_expression::value_t mach_expr_binary<mul>::evaluate(mach_evaluate_info info) const
{
    auto left_res = left_->evaluate(info);
    auto right_res = right_->evaluate(info);

    if (!(left_res.value_kind() == context::symbol_value_kind::ABS
            && right_res.value_kind() == context::symbol_value_kind::ABS)
        && left_res.value_kind() != context::symbol_value_kind::UNDEF
        && right_res.value_kind() != context::symbol_value_kind::UNDEF)
        add_diagnostic(diagnostic_op::error_ME002(get_range()));


    return left_res * right_res;
}

template<>
inline mach_expression::value_t mach_expr_binary<div>::evaluate(mach_evaluate_info info) const
{
    auto left_res = left_->evaluate(info);
    auto right_res = right_->evaluate(info);

    if (!(left_res.value_kind() == context::symbol_value_kind::ABS
            && right_res.value_kind() == context::symbol_value_kind::ABS)
        && left_res.value_kind() != context::symbol_value_kind::UNDEF
        && right_res.value_kind() != context::symbol_value_kind::UNDEF)
        add_diagnostic(diagnostic_op::error_ME002(get_range()));

    return left_res / right_res;
}

template<>
inline mach_expression::value_t mach_expr_unary<add>::evaluate(mach_evaluate_info info) const
{
    return child_->evaluate(info);
}

template<>
inline mach_expression::value_t mach_expr_unary<sub>::evaluate(mach_evaluate_info info) const
{
    return -child_->evaluate(info);
}

template<>
inline mach_expression::value_t mach_expr_unary<par>::evaluate(mach_evaluate_info info) const
{
    return child_->evaluate(info);
}

template<>
inline context::dependency_collector mach_expr_binary<add>::get_dependencies(mach_evaluate_info info) const
{
    return left_->get_dependencies(info) + right_->get_dependencies(info);
}

template<>
inline context::dependency_collector mach_expr_binary<sub>::get_dependencies(mach_evaluate_info info) const
{
    return left_->get_dependencies(info) - right_->get_dependencies(info);
}
template<>
inline context::dependency_collector mach_expr_binary<rel_addr>::get_dependencies(mach_evaluate_info info) const
{
    return left_->get_dependencies(info) - right_->get_dependencies(info);
}
template<>
inline context::dependency_collector mach_expr_binary<mul>::get_dependencies(mach_evaluate_info info) const
{
    return left_->get_dependencies(info) * right_->get_dependencies(info);
}

template<>
inline context::dependency_collector mach_expr_binary<div>::get_dependencies(mach_evaluate_info info) const
{
    return left_->get_dependencies(info) / right_->get_dependencies(info);
}

template<>
inline context::dependency_collector mach_expr_unary<add>::get_dependencies(mach_evaluate_info info) const
{
    return child_->get_dependencies(info);
}

template<>
inline context::dependency_collector mach_expr_unary<sub>::get_dependencies(mach_evaluate_info info) const
{
    return context::dependency_collector() - child_->get_dependencies(info);
}

template<>
inline context::dependency_collector mach_expr_unary<par>::get_dependencies(mach_evaluate_info info) const
{
    return child_->get_dependencies(info);
}

} // namespace hlasm_plugin::parser_library::expressions

#endif