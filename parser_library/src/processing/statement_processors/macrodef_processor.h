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

#ifndef PROCESSING_MACRODEF_PROCESSOR_H
#define PROCESSING_MACRODEF_PROCESSOR_H

#include "macrodef_processing_info.h"
#include "statement_processor.h"

namespace hlasm_plugin::parser_library::context {
class hlasm_context;
} // namespace hlasm_plugin::parser_library::context

namespace hlasm_plugin::parser_library::processing {
class branching_provider;
class processing_state_listener;

// processor that creates macro definition from provided statements
class macrodef_processor final : public statement_processor
{
    processing_state_listener& listener_;
    branching_provider& branching_provider_;
    const macrodef_start_data start_;

    position curr_outer_position_;
    std::vector<size_t> copy_nest_limit;
    bool bumped_macro_nest = false;
    bool expecting_prototype_ = true;
    bool last_in_inner_macro_ = false;
    bool finished_flag_ = false;

    macrodef_processing_result result_;

    struct handler_table;

public:
    macrodef_processor(const analyzing_context& ctx,
        processing_state_listener& listener,
        branching_provider& branching_provider_,
        macrodef_start_data start,
        diagnosable_ctx& diag_ctx);

    std::optional<processing_status> get_processing_status(
        const std::optional<context::id_index>& instruction, const range& r) const override;
    void process_statement(context::shared_stmt_ptr statement) override;
    void end_processing() override;
    bool terminal_condition(const statement_provider_kind kind) const override;
    bool finished() override;

    static processing_status get_macro_processing_status(
        const std::optional<context::id_index>& instruction, context::hlasm_context& hlasm_ctx);

private:
    bool process_statement(const context::hlasm_statement& statement);

    void process_prototype(const resolved_statement& statement);
    void process_prototype_label(const resolved_statement& statement, std::vector<context::id_index>& param_names);
    void process_prototype_instruction(const resolved_statement& statement);
    void process_prototype_operand(const resolved_statement& statement, std::vector<context::id_index>& param_names);

    bool test_varsym_validity(const semantics::variable_symbol* var,
        const std::vector<context::id_index>& param_names,
        range op_range,
        bool add_empty);

    bool process_MACRO();
    bool process_MEND();
    bool process_COPY(const resolved_statement& statement);
    bool process_LCL_GBL(const resolved_statement& statement, context::SET_t_enum set_type, bool global);
    bool process_SET(const resolved_statement& statement, context::SET_t_enum set_type);

    void add_SET_sym_to_res(const semantics::variable_symbol* sym, context::SET_t_enum set_type, bool global);

    void process_sequence_symbol(const semantics::label_si& label);

    void add_correct_copy_nest();

    std::optional<context::id_index> resolve_concatenation(
        const semantics::concat_chain& concat, const range& r) const override;

    void update_outer_position(const context::hlasm_statement& stmt);
};

} // namespace hlasm_plugin::parser_library::processing

#endif
