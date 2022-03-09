/*
 * Copyright (c) 2021 Broadcom.
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

#ifndef CONTEXT_ORDINARY_ASSEMBLY_DEPENDENCY_SOLVER_H
#define CONTEXT_ORDINARY_ASSEMBLY_DEPENDENCY_SOLVER_H

#include "ordinary_assembly_context.h"

namespace hlasm_plugin::parser_library::context {

class ordinary_assembly_dependency_solver final : public dependency_solver
{
    ordinary_assembly_context& ord_context;
    std::optional<context::address> loctr_addr;
    size_t literal_pool_generation = (size_t)-1;
    size_t unique_id = 0;

public:
    explicit ordinary_assembly_dependency_solver(ordinary_assembly_context& ord_context)
        : ord_context(ord_context)
        , literal_pool_generation(ord_context.current_literal_pool_generation())
        , unique_id(ord_context.current_unique_id())
    {}

    ordinary_assembly_dependency_solver(ordinary_assembly_context& ord_context, context::address loctr_addr)
        : ord_context(ord_context)
        , loctr_addr(std::move(loctr_addr))
        , literal_pool_generation(ord_context.current_literal_pool_generation())
        , unique_id(ord_context.current_unique_id())
    {}

    ordinary_assembly_dependency_solver(
        ordinary_assembly_context& ord_context, const dependency_evaluation_context& dep_ctx)
        : ord_context(ord_context)
        , loctr_addr(dep_ctx.loctr_address)
        , literal_pool_generation(dep_ctx.literal_pool_generation)
        , unique_id(dep_ctx.unique_id)
    {}

    const symbol* get_symbol(id_index name) const override;
    std::optional<address> get_loctr() const override;
    id_index get_literal_id(const std::shared_ptr<const expressions::data_definition>& lit) override;

    dependency_evaluation_context derive_current_dependency_evaluation_context() const;
};

} // namespace hlasm_plugin::parser_library::context

#endif