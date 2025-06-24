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

#include <functional>
#include <map>
#include <memory>
#include <string_view>
#include <vector>

#include "analyzing_context.h"
#include "branching_provider.h"
#include "fade_messages.h"
#include "opencode_provider.h"
#include "parse_lib_provider.h"
#include "processing_state_listener.h"
#include "statement_analyzers/lsp_analyzer.h"
#include "statement_fields_parser.h"
#include "utils/task.h"

namespace hlasm_plugin::parser_library {
class output_handler;
} // namespace hlasm_plugin::parser_library
namespace hlasm_plugin::parser_library::parsing {
class parser_holder;
} // namespace hlasm_plugin::parser_library::parsing

namespace hlasm_plugin::parser_library::processing {

// main class for processing of the opencode
// is constructed with base statement provider and has stack of statement processors which take statements from
// providers and go through the code creating other providers and processors it holds those providers and processors and
// manages the whole processing
class processing_manager final : public processing_state_listener, public branching_provider
{
public:
    processing_manager(std::unique_ptr<opencode_provider> base_provider,
        const analyzing_context& ctx,
        processing::processing_kind proc_kind,
        std::string dep_name,
        utils::resource::resource_location file_loc,
        std::string_view file_text,
        parse_lib_provider& lib_provider,
        statement_fields_parser& parser,
        std::shared_ptr<std::vector<fade_message>> fade_msgs,
        output_handler* output,
        diagnosable_ctx& diag_ctx);

    [[nodiscard]] utils::task co_step();

    void register_stmt_analyzer(statement_analyzer* stmt_analyzer);

    void run_analyzers(const context::hlasm_statement& statement, bool evaluated_model) const;
    void run_analyzers(const context::hlasm_statement& statement,
        statement_provider_kind prov_kind,
        processing_kind proc_kind,
        bool evaluated_model) const;

    void aread_cb(size_t line, std::string_view text) const;

    void process_postponed_statements(const std::vector<
        std::pair<std::unique_ptr<context::postponed_statement>, context::dependency_evaluation_context>>& stmts);

    parsing::parser_holder& opencode_parser(); // for testing only

private:
    analyzing_context ctx_;
    context::hlasm_context& hlasm_ctx_;
    parse_lib_provider& lib_provider_;
    opencode_provider& opencode_prov_;
    diagnosable_ctx& diag_ctx;

    std::vector<processor_ptr> procs_;
    std::vector<provider_ptr> provs_;

    utils::task helper_task_;

    lsp_analyzer lsp_analyzer_;
    std::vector<statement_analyzer*> stms_analyzers_;

    const utils::resource::resource_location file_loc_;

    context::source_snapshot lookahead_stop_;
    size_t lookahead_stop_ainsert_id = 0;
    enum class pending_seq_redifinition_state
    {
        lookahead_pending,
        lookahead_done,
        diagnostics,
    };
    std::unordered_map<context::id_index, std::pair<pending_seq_redifinition_state, std::vector<diagnostic>>>
        m_lookahead_seq_redifinitions;
    std::vector<decltype(m_lookahead_seq_redifinitions)::iterator> m_pending_seq_redifinitions;

    std::shared_ptr<std::vector<fade_message>> m_fade_msgs;

    std::map<std::pair<context::id_index, processing::processing_kind>, bool> m_external_requests;

    bool attr_lookahead_active() const;
    bool seq_lookahead_active() const;
    bool lookahead_active() const;

    statement_provider& find_provider() const;
    void finish_processor();
    void finish_preprocessor();

    void start_macro_definition(macrodef_start_data start) override;
    void finish_macro_definition(macrodef_processing_result result) override;
    void start_lookahead(lookahead_start_data start) override;
    void finish_lookahead(lookahead_processing_result result) override;
    void start_copy_member(copy_start_data start) override;
    void finish_copy_member(copy_processing_result result) override;
    void finish_opencode() override;
    std::optional<bool> request_external_processing(
        context::id_index name, processing::processing_kind proc_kind, std::function<void(bool)> callback) override;

    void schedule_helper_task(utils::task t) override;

    void start_macro_definition(macrodef_start_data start, std::optional<utils::resource::resource_location> file_loc);

    void jump_in_statements(context::id_index target, range symbol_range) override;
    void register_sequence_symbol(context::id_index target, range symbol_range) override;
    std::unique_ptr<context::opencode_sequence_symbol> create_opencode_sequence_symbol(
        context::id_index name, range symbol_range);

    void perform_opencode_jump(context::source_position statement_position, context::source_snapshot snapshot);
};

} // namespace hlasm_plugin::parser_library::processing

#endif
