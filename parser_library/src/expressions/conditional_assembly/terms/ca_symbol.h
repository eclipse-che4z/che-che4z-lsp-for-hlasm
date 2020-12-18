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
class ca_symbol : public ca_expression
{
public:
    const context::id_index symbol;

    ca_symbol(context::id_index symbol, range expr_range);

    virtual undef_sym_set get_undefined_attributed_symbols(const evaluation_context& eval_ctx) const override;

    virtual void resolve_expression_tree(context::SET_t_enum kind) override;

    virtual void collect_diags() const override;

    virtual bool is_character_expression() const override;

    virtual context::SET_t evaluate(const evaluation_context& eval_ctx) const override;
};

} // namespace hlasm_plugin::parser_library::expressions


#endif
