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

namespace hlasm_plugin {
namespace parser_library {
namespace expressions {

class ca_expression;
using ca_expr_ptr = std::unique_ptr<ca_expression>;
using undef_sym_set = std::set<context::id_index>;

struct evaluation_context;

class ca_expression : public diagnosable_op_impl
{
public:
    range expr_range;
    context::SET_t_enum expr_kind;

    ca_expression(context::SET_t_enum expr_kind, range expr_range)
        : expr_range(std::move(expr_range))
        , expr_kind(expr_kind)
    {}

    virtual undef_sym_set get_undefined_attributed_symbols(const context::dependency_solver& solver) const = 0;

    virtual void resolve_expression_tree(context::SET_t_enum kind) = 0;

    virtual bool is_character_expression() const = 0;

    template <typename T> T evaluate(evaluation_context& eval_ctx) const
    {
        static_assert(context::object_traits<T>::type_enum != context::SET_t_enum::UNDEF_TYPE);
        auto ret = evaluate(eval_ctx);
        
        if (context::object_traits<T>::type_enum != ret.type)
        {
            add_diagnostic(diagnostic_op::error_CE004(expr_range));
            return context::object_traits<T>::default_v();
        }
        if constexpr (context::object_traits<T>::type_enum == context::SET_t_enum::A_TYPE)
            return ret.access_a();
        if constexpr (context::object_traits<T>::type_enum == context::SET_t_enum::B_TYPE)
            return ret.access_b();
        if constexpr (context::object_traits<T>::type_enum == context::SET_t_enum::C_TYPE)
            return std::move(ret.access_c());
    }

    virtual context::SET_t evaluate(evaluation_context& eval_ctx) const = 0;

    virtual ~ca_expression() = default;
};



} // namespace expressions
} // namespace parser_library
} // namespace hlasm_plugin

#endif
