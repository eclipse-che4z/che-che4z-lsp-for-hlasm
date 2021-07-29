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

#ifndef HLASMPLUGIN_PARSERLIBRARY_CA_STRING_H
#define HLASMPLUGIN_PARSERLIBRARY_CA_STRING_H

#include "../ca_expression.h"
#include "semantics/concatenation.h"

namespace hlasm_plugin::parser_library::expressions {

// represents CA expression string
class ca_string : public ca_expression
{
public:
    struct substring_t
    {
        ca_expr_ptr start;
        ca_expr_ptr count;
        range substring_range;
        substring_t();
    };

    const semantics::concat_chain value;
    ca_expr_ptr duplication_factor;
    substring_t substring;
    static constexpr size_t MAX_STR_SIZE = 4064;

    ca_string(semantics::concat_chain value, ca_expr_ptr duplication_factor, substring_t substring, range expr_range);

    bool get_undefined_attributed_symbols(const evaluation_context& eval_ctx, undef_sym_set& result) const override;

    void resolve_expression_tree(context::SET_t_enum kind) override;

    void collect_diags() const override;

    bool is_character_expression() const override;

    void apply(ca_expr_visitor& visitor) const override;

    context::SET_t evaluate(const evaluation_context& eval_ctx) const override;

    static std::string duplicate(
        const ca_expr_ptr& dupl_factor, std::string value, range expr_range, const evaluation_context& eval_ctx);
};

} // namespace hlasm_plugin::parser_library::expressions


#endif
