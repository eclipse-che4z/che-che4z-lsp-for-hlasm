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

#ifndef HLASMPLUGIN_PARSERLIBRARY_CA_CONSTANT_H
#define HLASMPLUGIN_PARSERLIBRARY_CA_CONSTANT_H

#include <optional>
#include <string_view>

#include "../ca_expression.h"
#include "diagnosable_ctx.h"
#include "diagnostic_adder.h"

namespace hlasm_plugin::parser_library::expressions {

// represents CA expression constant
class ca_constant final : public ca_expression
{
public:
    const context::A_t value;

    ca_constant(context::A_t value, range expr_range);

    undef_sym_set get_undefined_attributed_symbols(const evaluation_context& eval_ctx) const override;

    void resolve_expression_tree(context::SET_t_enum kind, diagnostic_op_consumer& diags) override;

    bool is_character_expression(character_expression_purpose purpose) const override;

    void apply(ca_expr_visitor& visitor) const override;

    context::SET_t evaluate(const evaluation_context& eval_ctx) const override;

    bool is_compatible(ca_expression_compatibility i) const override { return i == ca_expression_compatibility::setb; }

    static context::A_t self_defining_term(
        std::string_view type, std::string_view value, diagnostic_adder& add_diagnostic);

    static context::A_t self_defining_term(const std::string& value, diagnostic_adder& add_diagnostic);
    static context::A_t self_defining_term_or_abs_symbol(
        const std::string& value, const evaluation_context& eval_ctx, range expr_range);

    static std::optional<context::A_t> try_self_defining_term(const std::string& value);
};

} // namespace hlasm_plugin::parser_library::expressions


#endif
