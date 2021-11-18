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

#ifndef PROCESSING_LOW_LANGUAGE_PROCESSOR_H
#define PROCESSING_LOW_LANGUAGE_PROCESSOR_H

#include "checking/instruction_checker.h"
#include "context/ordinary_assembly/loctr_dependency_resolver.h"
#include "instruction_processor.h"
#include "processing/statement_fields_parser.h"

namespace hlasm_plugin::parser_library::processing {

// common ancestor for ASM and MACH processing containing useful methods
class low_language_processor : public instruction_processor, public context::loctr_dependency_resolver
{
public:
    static bool check(const resolved_statement& stmt,
        const context::processing_stack_t& processing_stack,
        context::dependency_solver& dep_solver,
        const checking::instruction_checker& checker,
        const diagnosable_ctx& diagnoser);

    void resolve_unknown_loctr_dependency(context::space_ptr sp,
        const context::address& addr,
        range err_range,
        context::dependency_evaluation_context dep_ctx) override;

protected:
    statement_fields_parser& parser;

    low_language_processor(analyzing_context ctx,
        branching_provider& branch_provider,
        workspaces::parse_lib_provider& lib_provider,
        statement_fields_parser& parser);

    rebuilt_statement preprocess(std::shared_ptr<const processing::resolved_statement> stmt);

    // adds dependency and also check for cyclic dependency and adds diagnostics if so
    template<typename... Args>
    auto add_dependency(range err_range, Args&&... args)
        -> std::void_t<decltype(hlasm_ctx.ord_ctx.symbol_dependencies.add_dependency(std::forward<Args>(args)...))>
    {
        bool cycle_ok = hlasm_ctx.ord_ctx.symbol_dependencies.add_dependency(std::forward<Args>(args)...);
        if (!cycle_ok)
            add_diagnostic(diagnostic_op::error_E033(err_range));
    }

    // finds symbol in the label field
    context::id_index find_label_symbol(const rebuilt_statement& stmt) const;

    // helper method to create symbol
    bool create_symbol(range err_range,
        context::id_index symbol_name,
        context::symbol_value value,
        context::symbol_attributes attributes);

    // helper method to check address for the ORG instruction
    bool check_address_for_ORG(range err_range,
        const context::address& addr_to_check,
        const context::address& curr_addr,
        size_t boundary,
        int offset);


private:
    using preprocessed_part = std::pair<std::optional<semantics::label_si>, std::optional<semantics::operands_si>>;
    preprocessed_part preprocess_inner(const resolved_statement& stmt);

    using transform_result = std::optional<std::vector<checking::check_op_ptr>>;
    // transform semantic operands to checking operands - machine mnemonics instructions
    static transform_result transform_mnemonic(
        const resolved_statement& stmt, context::dependency_solver& dep_solver, diagnostic_collector collector);
    // transform semantic operands to checking operands - default machine instructions
    static transform_result transform_default(
        const resolved_statement& stmt, context::dependency_solver& dep_solver, diagnostic_collector collector);

    static checking::check_op_ptr get_check_op(const semantics::operand* op,
        context::dependency_solver& dep_solver,
        diagnostic_collector collector,
        const resolved_statement& stmt,
        size_t op_position,
        const context::mnemonic_code* mnemonic = nullptr);
};

} // namespace hlasm_plugin::parser_library::processing
#endif