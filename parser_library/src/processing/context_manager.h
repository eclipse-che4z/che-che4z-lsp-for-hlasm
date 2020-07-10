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
    expressions::evaluation_context* eval_ctx_;

public:
    using name_result = std::pair<bool, context::id_index>;

    // wrapped context
    context::hlasm_context& hlasm_ctx;

    context_manager(context::hlasm_context& hlasm_ctx);
    context_manager(expressions::evaluation_context* eval_ctx);

    context::SET_t get_var_sym_value(
        context::id_index name, const std::vector<context::A_t>& subscript, range symbol_range) const;

    context::id_index get_symbol_name(const semantics::vs_ptr& symbol, expressions::evaluation_context eval_ctx) const;
    context::id_index get_symbol_name(const std::string& symbol, range symbol_range) const;
    name_result try_get_symbol_name(const std::string& symbol) const;

    bool test_symbol_for_read(
        const context::var_sym_ptr& var, const std::vector<context::A_t>& subscript, range symbol_range) const;

    virtual void collect_diags() const override;

    ~context_manager();
};

} // namespace hlasm_plugin::parser_library::processing
#endif
