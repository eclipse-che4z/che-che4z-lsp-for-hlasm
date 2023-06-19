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

#ifndef HLASMPLUGIN_PARSERLIBRARY_CA_SYMBOL_H
#define HLASMPLUGIN_PARSERLIBRARY_CA_SYMBOL_H

#include "../ca_expression.h"

namespace hlasm_plugin::parser_library::expressions {

// represents CA expression ordinary symbol
class ca_symbol final : public ca_expression
{
public:
    const context::id_index symbol;

    ca_symbol(context::id_index symbol, range expr_range);

    bool get_undefined_attributed_symbols(undef_sym_set& symbols, const evaluation_context& eval_ctx) const override;

    void resolve_expression_tree(ca_expression_ctx expr_ctx, diagnostic_op_consumer& diags) override;

    bool is_character_expression(character_expression_purpose purpose) const override;

    void apply(ca_expr_visitor& visitor) const override;

    context::SET_t evaluate(const evaluation_context& eval_ctx) const override;
};

} // namespace hlasm_plugin::parser_library::expressions


#endif
