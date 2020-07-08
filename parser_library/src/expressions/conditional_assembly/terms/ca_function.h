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
#include "checking/ranged_diagnostic_collector.h"

namespace hlasm_plugin {
namespace parser_library {
namespace expressions {

class ca_function : public ca_expression
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

    virtual undef_sym_set get_undefined_attributed_symbols(const context::dependency_solver& solver) const override;

    virtual void resolve_expression_tree(context::SET_t_enum kind) override;

    virtual void collect_diags() const override;

    virtual bool is_character_expression() const override;

    virtual context::SET_t evaluate(evaluation_context& eval_ctx) const;

    static context::SET_t B2A(std::string_view param, const ranged_diagnostic_collector& add_diagnostic);
    static context::SET_t C2A(std::string_view param, const ranged_diagnostic_collector& add_diagnostic);
    static context::SET_t D2A(std::string_view param, const ranged_diagnostic_collector& add_diagnostic);
    static context::SET_t DCLEN(const context::C_t& param);
    static context::SET_t FIND(const context::C_t& lhs, const context::C_t& rhs);
    static context::SET_t INDEX(const context::C_t& lhs, const context::C_t& rhs);
    static context::SET_t ISBIN(const context::C_t& param, const ranged_diagnostic_collector& add_diagnostic);
    static context::SET_t ISDEC(const context::C_t& param, const ranged_diagnostic_collector& add_diagnostic);
    static context::SET_t ISHEX(const context::C_t& param, const ranged_diagnostic_collector& add_diagnostic);
    static context::SET_t ISSYM(const context::C_t& param, const ranged_diagnostic_collector& add_diagnostic);
    static context::SET_t X2A(std::string_view param, const ranged_diagnostic_collector& add_diagnostic);
    static context::SET_t A2B(context::A_t param);
    static context::SET_t A2C(context::A_t param);
    static context::SET_t A2D(context::A_t param);
    static context::SET_t A2X(context::A_t param);
    static context::SET_t B2C(const context::C_t& param, const ranged_diagnostic_collector& add_diagnostic);
    static context::SET_t B2D(const context::C_t& param, const ranged_diagnostic_collector& add_diagnostic);
    static context::SET_t B2X(const context::C_t& param, const ranged_diagnostic_collector& add_diagnostic);
    static context::SET_t BYTE(context::A_t param, const ranged_diagnostic_collector& add_diagnostic);
    static context::SET_t C2B(const context::C_t& param, const ranged_diagnostic_collector& add_diagnostic);
    static context::SET_t C2D(const context::C_t& param, const ranged_diagnostic_collector& add_diagnostic);
    static context::SET_t C2X(const context::C_t& param, const ranged_diagnostic_collector& add_diagnostic);
    static context::SET_t D2B(const context::C_t& param, const ranged_diagnostic_collector& add_diagnostic);
    static context::SET_t D2C(const context::C_t& param, const ranged_diagnostic_collector& add_diagnostic);
    static context::SET_t D2X(const context::C_t& param, const ranged_diagnostic_collector& add_diagnostic);
    static context::SET_t DCVAL(const context::C_t& param);
    static context::SET_t DEQUOTE(context::C_t param);
    static context::SET_t DOUBLE(const context::C_t& param, const ranged_diagnostic_collector& add_diagnostic);
    static context::SET_t ESYM(const context::C_t& param);
    static context::SET_t LOWER(context::C_t param);
    static context::SET_t SIGNED(context::A_t param);
    static context::SET_t SYSATTRA(const context::C_t& param);
    static context::SET_t SYSATTRP(const context::C_t& param);
    static context::SET_t UPPER(context::C_t param);
    static context::SET_t X2B(const context::C_t& param, const ranged_diagnostic_collector& add_diagnostic);
    static context::SET_t X2C(const context::C_t& param, const ranged_diagnostic_collector& add_diagnostic);
    static context::SET_t X2D(const context::C_t& param, const ranged_diagnostic_collector& add_diagnostic);

private:
    context::SET_t get_ith_param(size_t idx, evaluation_context& eval_ctx) const;
};

} // namespace expressions
} // namespace parser_library
} // namespace hlasm_plugin


#endif
