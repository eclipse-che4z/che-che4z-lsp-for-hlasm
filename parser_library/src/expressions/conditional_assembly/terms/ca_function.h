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

#ifndef HLASMPLUGIN_PARSERLIBRARY_CA_FUNCTION_H
#define HLASMPLUGIN_PARSERLIBRARY_CA_FUNCTION_H

#include <string_view>
#include <vector>

#include "../ca_expr_policy.h"
#include "../ca_expression.h"
#include "diagnostic_adder.h"

namespace hlasm_plugin::parser_library::expressions {

// represents CA expression built-in function
class ca_function final : public ca_expression
{
public:
    context::id_index function_name;
    const ca_expr_funcs function;
    std::vector<ca_expr_ptr> parameters;
    ca_expr_ptr duplication_factor;


    ca_function(context::id_index function_name,
        ca_expr_funcs function,
        std::vector<ca_expr_ptr> parameters,
        ca_expr_ptr duplication_factor,
        range expr_range);

    bool get_undefined_attributed_symbols(
        std::vector<context::id_index>& symbols, const evaluation_context& eval_ctx) const override;

    void resolve_expression_tree(ca_expression_ctx expr_ctx, diagnostic_op_consumer& diags) override;

    bool is_character_expression(character_expression_purpose purpose) const override;

    void apply(ca_expr_visitor& visitor) const override;

    context::SET_t evaluate(const evaluation_context& eval_ctx) const override;

    static context::SET_t B2A(std::string_view param, diagnostic_adder& add_diagnostic);
    static context::SET_t C2A(std::string_view param, diagnostic_adder& add_diagnostic);
    static context::SET_t D2A(std::string_view param, diagnostic_adder& add_diagnostic);
    static context::SET_t DCLEN(const context::C_t& param);
    static context::SET_t FIND(const context::C_t& lhs, const context::C_t& rhs);
    static context::SET_t INDEX(const context::C_t& lhs, const context::C_t& rhs);
    static context::SET_t ISBIN(const context::C_t& param, diagnostic_adder& add_diagnostic);
    static context::SET_t ISDEC(const context::C_t& param, diagnostic_adder& add_diagnostic);
    static context::SET_t ISHEX(const context::C_t& param, diagnostic_adder& add_diagnostic);
    static context::SET_t ISSYM(const context::C_t& param, diagnostic_adder& add_diagnostic);
    static context::SET_t X2A(std::string_view param, diagnostic_adder& add_diagnostic);
    static context::SET_t A2B(context::A_t param);
    static context::SET_t A2C(context::A_t param);
    static context::SET_t A2D(context::A_t param);
    static context::SET_t A2X(context::A_t param);
    static context::SET_t B2C(const context::C_t& param, diagnostic_adder& add_diagnostic);
    static context::SET_t B2D(const context::C_t& param, diagnostic_adder& add_diagnostic);
    static context::SET_t B2X(const context::C_t& param, diagnostic_adder& add_diagnostic);
    static context::SET_t BYTE(context::A_t param, diagnostic_adder& add_diagnostic);
    static context::SET_t C2B(const context::C_t& param, diagnostic_adder& add_diagnostic);
    static context::SET_t C2D(const context::C_t& param, diagnostic_adder& add_diagnostic);
    static context::SET_t C2X(const context::C_t& param, diagnostic_adder& add_diagnostic);
    static context::SET_t D2B(const context::C_t& param, diagnostic_adder& add_diagnostic);
    static context::SET_t D2C(const context::C_t& param, diagnostic_adder& add_diagnostic);
    static context::SET_t D2X(const context::C_t& param, diagnostic_adder& add_diagnostic);
    static context::SET_t DCVAL(const context::C_t& param);
    static context::SET_t DEQUOTE(context::C_t param);
    static context::SET_t DOUBLE(const context::C_t& param, diagnostic_adder& add_diagnostic);
    static context::SET_t ESYM(const context::C_t& param);
    static context::SET_t LOWER(context::C_t param);
    static context::SET_t SIGNED(context::A_t param);
    static context::SET_t SYSATTRA(const context::C_t& param, const evaluation_context& eval_ctx);
    static context::SET_t SYSATTRP(const context::C_t& param, const evaluation_context& eval_ctx);
    static context::SET_t UPPER(context::C_t param);
    static context::SET_t X2B(const context::C_t& param, diagnostic_adder& add_diagnostic);
    static context::SET_t X2C(const context::C_t& param, diagnostic_adder& add_diagnostic);
    static context::SET_t X2D(const context::C_t& param, diagnostic_adder& add_diagnostic);

private:
    context::SET_t get_ith_param(size_t idx, const evaluation_context& eval_ctx) const;
};

} // namespace hlasm_plugin::parser_library::expressions


#endif
