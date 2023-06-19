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

#ifndef HLASMPLUGIN_PARSERLIBRARY_CA_EXPRESSION_H
#define HLASMPLUGIN_PARSERLIBRARY_CA_EXPRESSION_H

#include <memory>
#include <set>

#include "context/common_types.h"
#include "context/ordinary_assembly/dependable.h"
#include "diagnosable_impl.h"

namespace hlasm_plugin::parser_library::expressions {

class ca_expr_visitor;
class ca_expression;
using ca_expr_ptr = std::unique_ptr<ca_expression>;
using undef_sym_set = std::set<context::id_index>;

struct evaluation_context;

enum class ca_expression_compatibility
{
    setb,
    aif,
};

enum class character_expression_purpose
{
    assignment,
    left_side_of_comparison,
};

struct ca_expression_ctx
{
    context::SET_t_enum kind;
    context::SET_t_enum parent_expr_kind;
    bool binary_operators_allowed;
};

// base class for conditional assembly expressions
class ca_expression
{
public:
    range expr_range;
    context::SET_t_enum expr_kind;

    ca_expression(context::SET_t_enum expr_kind, range expr_range);

    // retrieves set of attributed symbols that are not yet defined
    virtual bool get_undefined_attributed_symbols(undef_sym_set& symbols, const evaluation_context& eval_ctx) const = 0;

    // builds parts of the expression tree that could not be built during parsing
    virtual void resolve_expression_tree(ca_expression_ctx expr_ctx, diagnostic_op_consumer& diags) = 0;

    virtual bool is_character_expression(character_expression_purpose purpose) const = 0;

    virtual void apply(ca_expr_visitor& visitor) const = 0;

    template<typename T>
    T evaluate(const evaluation_context& eval_ctx) const;

    virtual context::SET_t evaluate(const evaluation_context& eval_ctx) const = 0;

    virtual bool is_compatible(ca_expression_compatibility) const { return false; }

    virtual ~ca_expression() = default;

protected:
    context::SET_t convert_return_types(
        context::SET_t retval, context::SET_t_enum type, const evaluation_context& eval_ctx) const;
};


template<typename T>
inline T ca_expression::evaluate(const evaluation_context& eval_ctx) const
{
    static_assert(context::object_traits<T>::type_enum != context::SET_t_enum::UNDEF_TYPE);
    auto ret = evaluate(eval_ctx);

    ret = convert_return_types(std::move(ret), context::object_traits<T>::type_enum, eval_ctx);

    if constexpr (context::object_traits<T>::type_enum == context::SET_t_enum::A_TYPE)
        return ret.access_a();
    if constexpr (context::object_traits<T>::type_enum == context::SET_t_enum::B_TYPE)
        return ret.access_b();
    if constexpr (context::object_traits<T>::type_enum == context::SET_t_enum::C_TYPE)
        return std::move(ret.access_c());
}

} // namespace hlasm_plugin::parser_library::expressions

#endif
