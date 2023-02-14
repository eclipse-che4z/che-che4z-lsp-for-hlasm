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

#include "context/hlasm_context.h"
#include "macrodef_processing_info.h"
#include "processing/processing_state_listener.h"
#include "statement_processor.h"
#include "workspaces/parse_lib_provider.h"

namespace hlasm_plugin::parser_library::processing {
class branching_provider;

// processor that creates macro definition from provided statements
class macrodef_processor final : public statement_processor
{
    processing_state_listener& listener_;
    branching_provider& branching_provider_;
    const macrodef_start_data start_;

    size_t initial_copy_nest_;
    size_t macro_nest_;
    size_t curr_line_;
    position curr_outer_position_;
    bool expecting_prototype_;
    bool expecting_MACRO_;
    bool omit_next_;

    macrodef_processing_result result_;
    bool last_in_inner_macro_ = false;
    bool finished_flag_;

    using process_table_t = std::unordered_map<context::id_index, std::function<void(const resolved_statement&)>>;

    const process_table_t table_;

public:
    macrodef_processor(analyzing_context ctx,
        processing_state_listener& listener,
        branching_provider& branching_provider_,
        macrodef_start_data start);

    std::optional<processing_status> get_processing_status(const semantics::instruction_si& instruction) const override;
    void process_statement(context::shared_stmt_ptr statement) override;
    void end_processing() override;
    bool terminal_condition(const statement_provider_kind kind) const override;
    bool finished() override;

    static processing_status get_macro_processing_status(
        const semantics::instruction_si& instruction, context::hlasm_context& hlasm_ctx);

    void collect_diags() const override;

private:
    void process_statement(const context::hlasm_statement& statement);

    void process_prototype(const resolved_statement& statement);
    void process_prototype_label(const resolved_statement& statement, std::vector<context::id_index>& param_names);
    void process_prototype_instruction(const resolved_statement& statement);
    void process_prototype_operand(const resolved_statement& statement, std::vector<context::id_index>& param_names);

    bool test_varsym_validity(const semantics::variable_symbol* var,
        const std::vector<context::id_index>& param_names,
        range op_range,
        bool add_empty);

    process_table_t create_table();

    void process_MACRO();
    void process_MEND();
    void process_COPY(const resolved_statement& statement);
    void process_LCL_GBL(const resolved_statement& statement, context::SET_t_enum set_type, bool global);
    void process_SET(const resolved_statement& statement, context::SET_t_enum set_type);

    void add_SET_sym_to_res(const semantics::variable_symbol* sym, context::SET_t_enum set_type, bool global);

    void process_sequence_symbol(const semantics::label_si& label);

    void add_correct_copy_nest();
};

} // namespace hlasm_plugin::parser_library::processing

#endif
