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

#include "processing/instruction_sets/asm_processor.h"
#include "processing/instruction_sets/ca_processor.h"
#include "processing/instruction_sets/mach_processor.h"
#include "processing/instruction_sets/macro_processor.h"
#include "processing/processing_tracer.h"
#include "statement_processor.h"
#include "workspaces/parse_lib_provider.h"

namespace hlasm_plugin::parser_library::processing {

// statement processor that evaluates the writen code, processes instructions
class ordinary_processor : public statement_processor
{
    static constexpr size_t NEST_LIMIT = 100;
    static constexpr size_t ACTR_LIMIT = 100;

    expressions::evaluation_context eval_ctx;

    ca_processor ca_proc_;
    macro_processor mac_proc_;
    asm_processor asm_proc_;
    mach_processor mach_proc_;

    bool finished_flag_;

    processing_tracer* tracer_;

public:
    ordinary_processor(analyzing_context ctx,
        branching_provider& branch_provider,
        workspaces::parse_lib_provider& lib_provider,
        processing_state_listener& state_listener,
        statement_fields_parser& parser,
        processing_tracer* tracer);

    virtual processing_status get_processing_status(const semantics::instruction_si& instruction) const override;
    virtual void process_statement(context::shared_stmt_ptr statement) override;
    virtual void end_processing() override;
    virtual bool terminal_condition(const statement_provider_kind kind) const override;
    virtual bool finished() override;

    static std::optional<processing_status> get_instruction_processing_status(
        context::id_index instruction, context::hlasm_context& hlasm_ctx);

    virtual void collect_diags() const override;

private:
    void check_postponed_statements(std::vector<context::post_stmt_ptr> stmts);
    bool check_fatals(range line_range);

    context::id_index resolve_instruction(const semantics::concat_chain& chain, range instruction_range) const;

    void collect_ordinary_symbol_definitions();
};

} // namespace hlasm_plugin::parser_library::processing
#endif
