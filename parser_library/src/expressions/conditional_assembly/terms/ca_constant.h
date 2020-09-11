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
class ca_constant : public ca_expression
{
public:
    const context::A_t value;

    ca_constant(context::A_t value, range expr_range);

    virtual undef_sym_set get_undefined_attributed_symbols(const context::dependency_solver& solver) const override;

    virtual void resolve_expression_tree(context::SET_t_enum kind) override;

    virtual void collect_diags() const override;

    virtual bool is_character_expression() const override;

    virtual context::SET_t evaluate(const evaluation_context& eval_ctx) const override;

    static context::A_t self_defining_term(
        std::string_view type, std::string_view value, diagnostic_adder& add_diagnostic);

    static context::A_t self_defining_term(const std::string& value, diagnostic_adder& add_diagnostic);

    static std::optional<context::A_t> try_self_defining_term(const std::string& value);
};

} // namespace hlasm_plugin::parser_library::expressions


#endif
