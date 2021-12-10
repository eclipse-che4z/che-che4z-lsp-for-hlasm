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

} // namespace hlasm_plugin::parser_library::expressions
