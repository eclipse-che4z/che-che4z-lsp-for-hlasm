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

#ifndef PROCESSING_LOOKAHEAD_PROCESSOR_H
#define PROCESSING_LOOKAHEAD_PROCESSOR_H

#include "lookahead_processing_info.h"
#include "processing/branching_provider.h"
#include "processing/processing_state_listener.h"
#include "statement_processor.h"
#include "workspaces/parse_lib_provider.h"

namespace hlasm_plugin::parser_library::processing {

// processor used for lookahead, hence finding desired symbol
class lookahead_processor final : public statement_processor
{
    using process_table_t =
        std::unordered_map<context::id_index, std::function<void(context::id_index, const resolved_statement&)>>;

    bool finished_flag_;
    lookahead_processing_result result_;
    size_t macro_nest_;
    branching_provider& branch_provider_;
    processing_state_listener& listener_;
    workspaces::parse_lib_provider& lib_provider_;

    std::set<context::id_index> to_find_;
    context::id_index target_;

public:
    const lookahead_action action;

    lookahead_processor(analyzing_context ctx,
        branching_provider& branch_provider,
        processing_state_listener& listener,
        workspaces::parse_lib_provider& lib_provider,
        lookahead_start_data start);

    std::optional<processing_status> get_processing_status(const semantics::instruction_si& instruction) const override;
    void process_statement(context::shared_stmt_ptr statement) override;
    void end_processing() override;
    bool terminal_condition(const statement_provider_kind kind) const override;
    bool finished() override;

    void collect_diags() const override;

private:
    void process_MACRO();
    void process_MEND();
    void process_COPY(const resolved_statement& statement);

    process_table_t asm_proc_table_;
    process_table_t create_table();

    void assign_EQU_attributes(context::id_index symbol_name, const resolved_statement& statement);
    void assign_data_def_attributes(context::id_index symbol_name, const resolved_statement& statement);
    void assign_section_attributes(context::id_index symbol_name, const resolved_statement& statement);
    void assign_cxd_attributes(context::id_index symbol_name, const resolved_statement& statement);

    void assign_machine_attributes(context::id_index symbol_name, const resolved_statement& statement);
    void assign_assembler_attributes(context::id_index symbol_name, const resolved_statement& statement);

    void find_seq(const semantics::core_statement& statement);
    void find_ord(const resolved_statement& statement);

    void register_attr_ref(context::id_index name, context::symbol_attributes attributes);
};

} // namespace hlasm_plugin::parser_library::processing

#endif
