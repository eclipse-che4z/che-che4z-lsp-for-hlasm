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

#ifndef HLASMPLUGIN_PARSERLIBRARY_CA_EXPR_LIST_H
#define HLASMPLUGIN_PARSERLIBRARY_CA_EXPR_LIST_H

#include <vector>

#include "../ca_expr_policy.h"
#include "../ca_expression.h"

namespace hlasm_plugin::parser_library::expressions {

// represents unresolved list of terms in logical CA expression
class ca_expr_list final : public ca_expression
{
public:
    ca_expr_list(std::vector<ca_expr_ptr> expr_list, range expr_range, bool parenthesized);

    undef_sym_set get_undefined_attributed_symbols(const evaluation_context& eval_ctx) const override;

    void resolve_expression_tree(ca_expression_ctx expr_ctx, diagnostic_op_consumer& diags) override;

    bool is_character_expression(character_expression_purpose purpose) const override;

    void apply(ca_expr_visitor& visitor) const override;

    context::SET_t evaluate(const evaluation_context& eval_ctx) const override;

    bool is_compatible(ca_expression_compatibility i) const override
    {
        return parenthesized && (i == ca_expression_compatibility::aif || i == ca_expression_compatibility::setb);
    }

    std::span<const ca_expr_ptr> expression_list() const;

private:
    // this function is present due to the fact that in hlasm you can omit space between operator and operands if
    // operators are in parentheses (eg. ('A')FIND('B') )
    // however, the parser recognizes it as a function with one parameter and a duplication factor
    // this function checks whether any function in expr_list is unknown
    // if so, then it breaks it to three objects so the resolve method can handle it
    void unknown_functions_to_operators();

    // in a loop it tries to retrieve first term, binary operator, second term
    // each loop iteration it pastes them together and continue until list is exhausted
    template<typename T>
    void resolve(ca_expression_ctx expr_ctx, diagnostic_op_consumer& diags);

    std::vector<ca_expr_ptr> expr_list;
    const bool parenthesized;
};

} // namespace hlasm_plugin::parser_library::expressions

#endif
