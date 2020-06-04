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

namespace hlasm_plugin {
namespace parser_library {
namespace expressions {

class ca_expr_list : public ca_expression
{
public:
    std::vector<ca_expr_ptr> expr_list;

    ca_expr_list(std::vector<ca_expr_ptr> expr_list, range expr_range);

    virtual undef_sym_set get_undefined_attributed_symbols(const context::dependency_solver& solver) const override;

    virtual void resolve_expression_tree(context::SET_t_enum kind) override;

    virtual void collect_diags() const override;

    virtual bool is_character_expression() const override;

    virtual context::SET_t evaluate(evaluation_context& eval_ctx) const;

private:
    template<typename T> void resolve();
    template<typename EXPR_POLICY> ca_expr_ptr retrieve_term(size_t& it, int priority);
    template<typename EXPR_POLICY> std::pair<int, ca_expr_ops> retrieve_binary_operator(size_t& it, bool& err);
};


} // namespace expressions
} // namespace parser_library
} // namespace hlasm_plugin


#endif
