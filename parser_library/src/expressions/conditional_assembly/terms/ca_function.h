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

#include <vector>

#include "../ca_expr_policy.h"
#include "../ca_expression.h"

namespace hlasm_plugin {
namespace parser_library {
namespace expressions {

class ca_function : public ca_expression
{
public:
    const ca_expr_funcs function;
    std::vector<ca_expr_ptr> parameters;
    ca_expr_ptr duplication_factor;


    ca_function(
        ca_expr_funcs function, std::vector<ca_expr_ptr> parameters, ca_expr_ptr duplication_factor, range expr_range);

    virtual undef_sym_set get_undefined_attributed_symbols(const context::dependency_solver& solver) const override;

    virtual void resolve_expression_tree(context::SET_t_enum kind) override;

    virtual void collect_diags() const override;

    virtual bool is_character_expression() const override;

    virtual context::SET_t evaluate(evaluation_context& eval_ctx) const;

    static context::SET_t B2A(context::SET_t param, range expr_range, evaluation_context& eval_ctx);
    static context::SET_t C2A(context::SET_t param, range expr_range, evaluation_context& eval_ctx);
    static context::SET_t D2A(context::SET_t param, range expr_range, evaluation_context& eval_ctx);
    static context::SET_t DCLEN(context::SET_t param);
    static context::SET_t FIND(context::SET_t lhs, context::SET_t rhs);
    static context::SET_t INDEX(context::SET_t lhs, context::SET_t rhs);
    static context::SET_t ISBIN(context::SET_t param, range expr_range, evaluation_context& eval_ctx);
    static context::SET_t ISDEC(context::SET_t param, range expr_range, evaluation_context& eval_ctx);
    static context::SET_t ISHEX(context::SET_t param, range expr_range, evaluation_context& eval_ctx);
    static context::SET_t ISSYM(context::SET_t param, range expr_range, evaluation_context& eval_ctx);
    static context::SET_t X2A(context::SET_t param, range expr_range, evaluation_context& eval_ctx);
    static context::SET_t A2B(context::SET_t param);
    static context::SET_t A2C(context::SET_t param);
    static context::SET_t A2D(context::SET_t param);
    static context::SET_t A2X(context::SET_t param);
    static context::SET_t B2C(context::SET_t param, range expr_range, evaluation_context& eval_ctx);
    static context::SET_t B2D(context::SET_t param, range expr_range, evaluation_context& eval_ctx);
    static context::SET_t B2X(context::SET_t param, range expr_range, evaluation_context& eval_ctx);
    static context::SET_t BYTE(context::SET_t param, range expr_range, evaluation_context& eval_ctx);
    static context::SET_t C2B(context::SET_t param, range expr_range, evaluation_context& eval_ctx);
    static context::SET_t C2D(context::SET_t param, range expr_range, evaluation_context& eval_ctx);
    static context::SET_t C2X(context::SET_t param, range expr_range, evaluation_context& eval_ctx);
    static context::SET_t D2B(context::SET_t param, range expr_range, evaluation_context& eval_ctx);
    static context::SET_t D2C(context::SET_t param, range expr_range, evaluation_context& eval_ctx);
    static context::SET_t D2X(context::SET_t param, range expr_range, evaluation_context& eval_ctx);
    static context::SET_t DCVAL(context::SET_t param);
    static context::SET_t DEQUOTE(context::SET_t param);
    static context::SET_t DOUBLE(context::SET_t param, range expr_range, evaluation_context& eval_ctx);
    static context::SET_t ESYM(context::SET_t param);
    static context::SET_t LOWER(context::SET_t param);
    static context::SET_t SIGNED(context::SET_t param);
    static context::SET_t SYSATTRA(context::SET_t param);
    static context::SET_t SYSATTRP(context::SET_t param);
    static context::SET_t UPPER(context::SET_t param);
    static context::SET_t X2B(context::SET_t param, range expr_range, evaluation_context& eval_ctx);
    static context::SET_t X2C(context::SET_t param, range expr_range, evaluation_context& eval_ctx);
    static context::SET_t X2D(context::SET_t param, range expr_range, evaluation_context& eval_ctx);

private:
    context::SET_t get_ith_param(size_t idx, evaluation_context& eval_ctx) const;
};

} // namespace expressions
} // namespace parser_library
} // namespace hlasm_plugin


#endif
