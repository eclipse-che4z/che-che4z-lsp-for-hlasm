/*
 * Copyright (c) 2021 Broadcom.
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

#include "mach_operator.h"

#include <cassert>

#include "utils/similar.h"

namespace hlasm_plugin::parser_library::expressions {

template<typename T>
bool mach_expr_binary<T>::do_is_similar(const mach_expression& expr) const
{
    return utils::is_similar(*this,
        static_cast<const mach_expr_binary<T>&>(expr),
        &mach_expr_binary<T>::left_,
        &mach_expr_binary<T>::right_);
}

template bool mach_expr_binary<add>::do_is_similar(const mach_expression& expr) const;
template bool mach_expr_binary<sub>::do_is_similar(const mach_expression& expr) const;
template bool mach_expr_binary<rel_addr>::do_is_similar(const mach_expression& expr) const;
template bool mach_expr_binary<mul>::do_is_similar(const mach_expression& expr) const;
template bool mach_expr_binary<div>::do_is_similar(const mach_expression& expr) const;

template<>
size_t mach_expr_binary<add>::hash() const
{
    return hash_combine((size_t)0xb97f4e373e923282, hash_combine(left_->hash(), right_->hash()));
}
template<>
size_t mach_expr_binary<sub>::hash() const
{
    return hash_combine((size_t)0x41e9ae4811f88d69, hash_combine(left_->hash(), right_->hash()));
}
template<>
size_t mach_expr_binary<rel_addr>::hash() const
{
    return hash_combine((size_t)0xa5aac0163f6f9304, hash_combine(left_->hash(), right_->hash()));
}
template<>
size_t mach_expr_binary<mul>::hash() const
{
    return hash_combine((size_t)0xc1cffe0cc1aa0820, hash_combine(left_->hash(), right_->hash()));
}
template<>
size_t mach_expr_binary<div>::hash() const
{
    return hash_combine((size_t)0xdc34b8e67fb7ed92, hash_combine(left_->hash(), right_->hash()));
}

template<typename T>
bool mach_expr_unary<T>::do_is_similar(const mach_expression& expr) const
{
    return utils::is_similar(child_, static_cast<const mach_expr_unary<T>&>(expr).child_);
}

template bool mach_expr_unary<add>::do_is_similar(const mach_expression& expr) const;
template bool mach_expr_unary<sub>::do_is_similar(const mach_expression& expr) const;
template bool mach_expr_unary<par>::do_is_similar(const mach_expression& expr) const;

template<>
size_t mach_expr_unary<add>::hash() const
{
    return hash_combine((size_t)0xf65c5e195b070c15, child_->hash());
}
template<>
size_t mach_expr_unary<sub>::hash() const
{
    return hash_combine((size_t)0x837fe8200ff456f6, child_->hash());
}
template<>
size_t mach_expr_unary<par>::hash() const
{
    return hash_combine((size_t)0xe45af0e42cda6037, child_->hash());
}

// !!! DO NOT CHANGE THE CODE INTO return l->evaluate/get... (op) r->evaluate/get... !!!
// function arguments evaluation order is unspecified.
// TODO: once literal registration is re-worked, this could be possibly changed back.

template<>
mach_expression::value_t mach_expr_binary<add>::evaluate(
    context::dependency_solver& info, diagnostic_op_consumer& diags) const
{
    auto l = left_->evaluate(info, diags);
    auto r = right_->evaluate(info, diags);
    return std::move(l) + std::move(r);
}

template<>
mach_expression::value_t mach_expr_binary<sub>::evaluate(
    context::dependency_solver& info, diagnostic_op_consumer& diags) const
{
    auto l = left_->evaluate(info, diags);
    auto r = right_->evaluate(info, diags);
    return std::move(l) - std::move(r);
}

template<>
mach_expression::value_t mach_expr_binary<rel_addr>::evaluate(
    context::dependency_solver& info, diagnostic_op_consumer& diags) const
{
    auto location = left_->evaluate(info, diags);
    auto target = right_->evaluate(info, diags);
    if (target.value_kind() == context::symbol_value_kind::ABS)
    {
        diags.add_diagnostic(diagnostic_op::warn_D032(get_range(), std::to_string(target.get_abs())));
        return target;
    }

    auto result = (target - location).ignore_qualification();
    if (result.value_kind() == context::symbol_value_kind::ABS)
    {
        if (result.get_abs() % 2 != 0)
            diags.add_diagnostic(diagnostic_op::error_ME003(get_range()));
        result = mach_expression::value_t(result.get_abs() / 2);
    }
    return result;
}

template<>
mach_expression::value_t mach_expr_binary<mul>::evaluate(
    context::dependency_solver& info, diagnostic_op_consumer& diags) const
{
    auto left_res = left_->evaluate(info, diags);
    auto right_res = right_->evaluate(info, diags);

    if (!(left_res.value_kind() == context::symbol_value_kind::ABS
            && right_res.value_kind() == context::symbol_value_kind::ABS)
        && left_res.value_kind() != context::symbol_value_kind::UNDEF
        && right_res.value_kind() != context::symbol_value_kind::UNDEF)
        diags.add_diagnostic(diagnostic_op::error_ME002(get_range()));


    return left_res * right_res;
}

template<>
mach_expression::value_t mach_expr_binary<div>::evaluate(
    context::dependency_solver& info, diagnostic_op_consumer& diags) const
{
    auto left_res = left_->evaluate(info, diags);
    auto right_res = right_->evaluate(info, diags);

    if (!(left_res.value_kind() == context::symbol_value_kind::ABS
            && right_res.value_kind() == context::symbol_value_kind::ABS)
        && left_res.value_kind() != context::symbol_value_kind::UNDEF
        && right_res.value_kind() != context::symbol_value_kind::UNDEF)
        diags.add_diagnostic(diagnostic_op::error_ME002(get_range()));

    return left_res / right_res;
}

template<>
mach_expression::value_t mach_expr_unary<add>::evaluate(
    context::dependency_solver& info, diagnostic_op_consumer& diags) const
{
    return child_->evaluate(info, diags);
}

template<>
mach_expression::value_t mach_expr_unary<sub>::evaluate(
    context::dependency_solver& info, diagnostic_op_consumer& diags) const
{
    return -child_->evaluate(info, diags);
}

template<>
mach_expression::value_t mach_expr_unary<par>::evaluate(
    context::dependency_solver& info, diagnostic_op_consumer& diags) const
{
    return child_->evaluate(info, diags);
}

template<>
context::dependency_collector mach_expr_binary<add>::get_dependencies(context::dependency_solver& info) const
{
    auto l = left_->get_dependencies(info);
    auto r = right_->get_dependencies(info);
    return std::move(l) + std::move(r);
}

template<>
context::dependency_collector mach_expr_binary<sub>::get_dependencies(context::dependency_solver& info) const
{
    auto l = left_->get_dependencies(info);
    auto r = right_->get_dependencies(info);
    return std::move(l) - std::move(r);
}

template<>
context::dependency_collector mach_expr_binary<rel_addr>::get_dependencies(context::dependency_solver& info) const
{
    auto l = left_->get_dependencies(info);
    auto r = right_->get_dependencies(info);
    return std::move(l) - std::move(r);
}

template<>
context::dependency_collector mach_expr_binary<mul>::get_dependencies(context::dependency_solver& info) const
{
    auto l = left_->get_dependencies(info);
    auto r = right_->get_dependencies(info);
    return std::move(l) * std::move(r);
}

template<>
context::dependency_collector mach_expr_binary<div>::get_dependencies(context::dependency_solver& info) const
{
    auto l = left_->get_dependencies(info);
    auto r = right_->get_dependencies(info);
    return std::move(l) / std::move(r);
}

template<>
context::dependency_collector mach_expr_unary<add>::get_dependencies(context::dependency_solver& info) const
{
    return child_->get_dependencies(info);
}

template<>
context::dependency_collector mach_expr_unary<sub>::get_dependencies(context::dependency_solver& info) const
{
    return context::dependency_collector() - child_->get_dependencies(info);
}

template<>
context::dependency_collector mach_expr_unary<par>::get_dependencies(context::dependency_solver& info) const
{
    return child_->get_dependencies(info);
}

} // namespace hlasm_plugin::parser_library::expressions
