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
class ca_expr_list : public ca_expression
{
public:
    std::vector<ca_expr_ptr> expr_list;

    ca_expr_list(std::vector<ca_expr_ptr> expr_list, range expr_range);

    virtual undef_sym_set get_undefined_attributed_symbols(const evaluation_context& eval_ctx) const override;

    virtual void resolve_expression_tree(context::SET_t_enum kind) override;

    virtual void collect_diags() const override;

    virtual bool is_character_expression() const override;

    virtual context::SET_t evaluate(const evaluation_context& eval_ctx) const override;

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
    void resolve();
    // retrieves single term with possible unary operators before it
    // also checks for following binary operator,
    // if it has higher prio than the current one, recursively calls retrieve_term for the second term for the higher
    // priority operator
    template<typename EXPR_POLICY>
    ca_expr_ptr retrieve_term(size_t& it, int priority);
    // retrieves following binary operator with its priority
    template<typename EXPR_POLICY>
    std::pair<int, ca_expr_ops> retrieve_binary_operator(size_t& it, bool& err);
};

} // namespace hlasm_plugin::parser_library::expressions

#endif
