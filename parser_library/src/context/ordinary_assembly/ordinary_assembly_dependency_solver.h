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

#include "context/opcode_generation.h"
#include "dependable.h"
#include "tagged_index.h"

namespace hlasm_plugin::parser_library {
class library_info;
} // namespace hlasm_plugin::parser_library

namespace hlasm_plugin::parser_library::context {
class using_collection;
class ordinary_assembly_context;
struct dependency_evaluation_context;

class ordinary_assembly_dependency_solver final : public dependency_solver
{
    ordinary_assembly_context& ord_context;
    std::optional<context::address> loctr_addr;
    size_t literal_pool_generation = (size_t)-1;
    size_t unique_id = 0;
    index_t<using_collection> active_using;
    opcode_generation opcode_gen;

    const library_info& lib_info;

public:
    explicit ordinary_assembly_dependency_solver(ordinary_assembly_context& ord_context, const library_info& li);

    explicit ordinary_assembly_dependency_solver(
        ordinary_assembly_context& ord_context, context::address loctr_addr, const library_info& li);

    explicit ordinary_assembly_dependency_solver(
        ordinary_assembly_context& ord_context, const dependency_evaluation_context& dep_ctx, const library_info& li);

    const symbol* get_symbol(id_index name) const override;
    std::optional<address> get_loctr() const override;
    id_index get_literal_id(const std::shared_ptr<const expressions::data_definition>& lit) override;
    bool using_active(id_index label, const section* sect) const override;
    using_evaluate_result using_evaluate(
        id_index label, const section* owner, int32_t offset, bool long_offset) const override;
    std::variant<const symbol*, symbol_candidate> get_symbol_candidate(id_index name) const override;
    std::string get_opcode_attr(id_index name) const override;
    const asm_option& get_options() const noexcept override;
    const section* get_section(id_index name) const noexcept override;

    dependency_evaluation_context derive_current_dependency_evaluation_context() const&;
    dependency_evaluation_context derive_current_dependency_evaluation_context() &&;
};

} // namespace hlasm_plugin::parser_library::context

#endif
