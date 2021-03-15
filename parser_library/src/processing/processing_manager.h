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

#ifndef PROCESSING_PROCESSING_MANAGER_H
#define PROCESSING_PROCESSING_MANAGER_H

#include <set>
#include <stack>

#include "branching_provider.h"
#include "opencode_provider.h"
#include "processing_state_listener.h"
#include "processing_tracer.h"
#include "statement_analyzers/statement_analyzer.h"
#include "statement_fields_parser.h"
#include "workspaces/parse_lib_provider.h"

namespace hlasm_plugin::parser_library::processing {

// main class for processing of the opencode
// is constructed with base statement provider and has stack of statement processors which take statements from
// providers and go through the code creating other providers and processors it holds those providers and processors and
// manages the whole processing
class processing_manager : public processing_state_listener, public branching_provider, public diagnosable_ctx
{
public:
    processing_manager(std::unique_ptr<opencode_provider> base_provider,
        analyzing_context ctx,
        const workspaces::library_data data,
        std::string file_name,
        const std::string & file_text,
        workspaces::parse_lib_provider& lib_provider,
        statement_fields_parser& parser,
        processing_tracer* tracer);

    // method that starts the processing loop
    void start_processing(std::atomic<bool>* cancel);

    virtual void collect_diags() const override;

private:
    analyzing_context ctx_;
    context::hlasm_context& hlasm_ctx_;
    workspaces::parse_lib_provider& lib_provider_;
    opencode_provider& opencode_prov_;

    std::vector<processor_ptr> procs_;
    std::vector<provider_ptr> provs_;
    analyzer_ptr stmt_analyzer_;

    context::source_snapshot lookahead_stop_;

    processing_tracer* tracer_ = nullptr;

    bool attr_lookahead_active() const;

    statement_provider& find_provider();
    void finish_processor();

    virtual void start_macro_definition(macrodef_start_data start) override;
    virtual void finish_macro_definition(macrodef_processing_result result) override;
    virtual void start_lookahead(lookahead_start_data start) override;
    virtual void finish_lookahead(lookahead_processing_result result) override;
    virtual void start_copy_member(copy_start_data start) override;
    virtual void finish_copy_member(copy_processing_result result) override;
    virtual void finish_opencode() override;

    void start_macro_definition(macrodef_start_data start, std::optional<std::string> file);

    virtual void jump_in_statements(context::id_index target, range symbol_range) override;
    virtual void register_sequence_symbol(context::id_index target, range symbol_range) override;
    std::unique_ptr<context::opencode_sequence_symbol> create_opencode_sequence_symbol(
        context::id_index name, range symbol_range);

    void perform_opencode_jump(context::source_position statement_position, context::source_snapshot snapshot);
};

} // namespace hlasm_plugin::parser_library::processing

#endif
