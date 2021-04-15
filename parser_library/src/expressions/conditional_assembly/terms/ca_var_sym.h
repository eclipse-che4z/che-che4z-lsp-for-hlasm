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

#ifndef HLASMPLUGIN_PARSERLIBRARY_CA_VAR_SYM_H
#define HLASMPLUGIN_PARSERLIBRARY_CA_VAR_SYM_H

#include "../ca_expression.h"
#include "semantics/variable_symbol.h"

namespace hlasm_plugin::parser_library::expressions {

// represents CA expression variable symbol
class ca_var_sym : public ca_expression
{
public:
    const semantics::vs_ptr symbol;

    ca_var_sym(semantics::vs_ptr symbol, range expr_range);

    undef_sym_set get_undefined_attributed_symbols(const evaluation_context& eval_ctx) const override;

    void resolve_expression_tree(context::SET_t_enum kind) override;

    void collect_diags() const override;

    bool is_character_expression() const override;

    void apply(ca_expr_visitor& visitor) const override;

    context::SET_t evaluate(const evaluation_context& eval_ctx) const override;

    static undef_sym_set get_undefined_attributed_symbols_vs(
        const semantics::vs_ptr& symbol, const evaluation_context& eval_ctx);
    static void resolve_expression_tree_vs(const semantics::vs_ptr& symbol);

private:
    context::SET_t convert_return_types(
        context::SET_t retval, context::SET_t_enum type, const evaluation_context& eval_ctx) const;
};

} // namespace hlasm_plugin::parser_library::expressions


#endif
