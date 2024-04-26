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

#ifndef PROCESSING_ORDINARY_PROCESSOR_H
#define PROCESSING_ORDINARY_PROCESSOR_H

#include "library_info_transitional.h"
#include "processing/instruction_sets/asm_processor.h"
#include "processing/instruction_sets/ca_processor.h"
#include "processing/instruction_sets/mach_processor.h"
#include "processing/instruction_sets/macro_processor.h"
#include "processing/opencode_provider.h"
#include "statement_processor.h"

namespace hlasm_plugin::parser_library::context {
struct dependency_evaluation_context;
struct postponed_statement;
} // namespace hlasm_plugin::parser_library::context

namespace hlasm_plugin::parser_library::workspaces {
class parse_lib_provider;
} // namespace hlasm_plugin::parser_library::workspaces

namespace hlasm_plugin::parser_library::processing {
class branching_provider;
class processing_state_listener;

// statement processor that evaluates the written code, processes instructions
class ordinary_processor final : public statement_processor
{
    static constexpr size_t NEST_LIMIT = 100;

    branching_provider& branch_provider_;
    library_info_transitional lib_info;
    expressions::evaluation_context eval_ctx;

    ca_processor ca_proc_;
    macro_processor mac_proc_;
    asm_processor asm_proc_;
    mach_processor mach_proc_;

    bool finished_flag_;

    processing_state_listener& listener_;
    processing_manager& proc_mgr;

public:
    ordinary_processor(const analyzing_context& ctx,
        branching_provider& branch_provider,
        parse_lib_provider& lib_provider,
        processing_state_listener& state_listener,
        statement_fields_parser& parser,
        opencode_provider& open_code,
        processing_manager& proc_mgr,
        output_handler* output);

    std::optional<processing_status> get_processing_status(
        const std::optional<context::id_index>& instruction, const range& r) const override;
    void process_statement(context::shared_stmt_ptr statement) override;
    void end_processing() override;

    bool terminal_condition(const statement_provider_kind kind) const override;
    bool finished() override;

    static std::optional<processing_status> get_instruction_processing_status(
        context::id_index instruction, context::hlasm_context& hlasm_ctx);

    void collect_diags() const override;

private:
    void process_postponed_statements(const std::vector<
        std::pair<std::unique_ptr<context::postponed_statement>, context::dependency_evaluation_context>>& stmts);
    void check_postponed_statements(const std::vector<
        std::pair<std::unique_ptr<context::postponed_statement>, context::dependency_evaluation_context>>& stmts);
    bool check_fatals(range line_range);

    context::id_index resolve_instruction_concat_chain(
        const semantics::concat_chain& chain, range instruction_range) const;

    std::optional<context::id_index> resolve_concatenation(
        const semantics::concat_chain& concat, const range& r) const override;
};

} // namespace hlasm_plugin::parser_library::processing
#endif
