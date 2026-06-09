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

#ifndef SEMANTICS_VARIABLE_SYMBOL_H
#define SEMANTICS_VARIABLE_SYMBOL_H

#include <memory>
#include <utility>
#include <variant>
#include <vector>

#include "context/id_index.h"
#include "expressions/conditional_assembly/ca_expression.h"
#include "range.h"

// this file is a composition of structures that create concat_chain
// concat_chain is used to represent model statement fields

namespace hlasm_plugin::parser_library::semantics {
struct concatenation_point;
using concat_chain = std::vector<concatenation_point>;
using vs_eval = std::pair<context::id_index, std::vector<context::A_t>>;

struct variable_symbol
{
    std::variant<context::id_index, concat_chain> value;
    std::vector<expressions::ca_expr_ptr> subscript;
    range symbol_range;

    const auto* named() const noexcept { return std::get_if<context::id_index>(&value); }
    const auto* created() const noexcept { return std::get_if<concat_chain>(&value); }

    vs_eval evaluate_symbol(const expressions::evaluation_context& eval_ctx) const;
    std::vector<context::A_t> evaluate_subscript(const expressions::evaluation_context& eval_ctx) const;
    context::SET_t evaluate(const expressions::evaluation_context& eval_ctx) const;

    context::id_index evaluate_name(const expressions::evaluation_context& eval_ctx) const;
    void resolve(context::SET_t_enum parent_expr_kind, diagnostic_op_consumer& diag);
};

using vs_ptr = std::unique_ptr<variable_symbol>;

} // namespace hlasm_plugin::parser_library::semantics

#endif
