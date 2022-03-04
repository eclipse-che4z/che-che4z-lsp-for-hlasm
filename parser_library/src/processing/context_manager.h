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

#ifndef PROCESSING_CONTEXT_MANAGER_H
#define PROCESSING_CONTEXT_MANAGER_H

#include "context/hlasm_context.h"
#include "diagnosable_ctx.h"
#include "expressions/evaluation_context.h"
#include "processing_format.h"
#include "semantics/range_provider.h"
#include "workspaces/parse_lib_provider.h"


namespace hlasm_plugin::parser_library::processing {

// class wrapping context providing ranges, checks and diagnostics to hlasm_context
class context_manager : public diagnosable_ctx
{
    const expressions::evaluation_context* eval_ctx_;

public:
    // wrapped context
    context::hlasm_context& hlasm_ctx;

    explicit context_manager(context::hlasm_context& hlasm_ctx);
    explicit context_manager(const expressions::evaluation_context* eval_ctx);

    context::SET_t get_var_sym_value(
        context::id_index name, const std::vector<context::A_t>& subscript, range symbol_range) const;

    context::id_index get_symbol_name(const std::string& symbol, range symbol_range) const;

    bool test_symbol_for_read(
        const context::var_sym_ptr& var, const std::vector<context::A_t>& subscript, range symbol_range) const;

    void collect_diags() const override;

private:
    void add_diagnostic(diagnostic_s diagnostic) const override;

    bool test_set_symbol_for_read(
        const context::set_symbol_base* set_sym, const std::vector<context::A_t>& subscript, range& symbol_range) const;

    bool test_macro_param_for_read(
        const context::macro_param_base* mac_par, const std::vector<context::A_t>& subscript, range& symbol_range) const;
};

} // namespace hlasm_plugin::parser_library::processing
#endif
