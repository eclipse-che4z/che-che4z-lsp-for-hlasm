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

#ifndef PROCESSING_OPENCODE_PROVIDER_H
#define PROCESSING_OPENCODE_PROVIDER_H

#include <deque>
#include <memory>
#include <string_view>
#include <vector>

#include "context/source_snapshot.h"
#include "lexing/logical_line.h"
#include "parsing/parser_error_listener.h"
#include "preprocessor.h"
#include "statement_providers/statement_provider.h"

namespace hlasm_plugin::parser_library::parsing {
class hlasmparser;
class parser_error_listener;
class parser_error_listener_ctx;
struct parser_holder;
} // namespace hlasm_plugin::parser_library::parsing
namespace hlasm_plugin::parser_library::semantics {
class collector;
struct range_provider;
class source_info_processor;
} // namespace hlasm_plugin::parser_library::semantics
namespace hlasm_plugin::parser_library {
struct analyzing_context;
} // namespace hlasm_plugin::parser_library

namespace hlasm_plugin::parser_library::processing {

enum class ainsert_destination
{
    back,
    front,
};

struct opencode_provider_options
{
    bool ictl_allowed;
    int process_remaining;
};

enum class extract_next_logical_line_result
{
    failed,
    normal,
    ictl,
    process,
};

// uses the parser implementation to produce statements in the opencode(-like) scenario
class opencode_provider final : public statement_provider
{
    struct lines_to_remove
    {
        size_t ainsert_buffer;
        size_t copy_files;
        size_t current_text_lines;
    };
    lines_to_remove m_lines_to_remove = {};

    std::string_view m_original_text;
    size_t m_current_line = 0;

    std::string_view m_next_line_text;

    lexing::logical_line m_current_logical_line;
    struct logical_line_origin
    {
        size_t begin_offset;
        size_t end_offset;
        size_t begin_line;
        size_t end_line;
        enum class source_type
        {
            none,
            file,
            copy,
            ainsert,
        } source;
    } m_current_logical_line_source;

    std::deque<std::string> m_ainsert_buffer;

    std::unordered_map<context::id_index, std::string> m_virtual_files;

    std::unique_ptr<parsing::parser_holder> m_parser;
    std::unique_ptr<parsing::parser_holder> m_lookahead_parser;
    std::unique_ptr<parsing::parser_holder> m_operand_parser;

    analyzing_context* m_ctx;
    workspaces::parse_lib_provider* m_lib_provider;
    processing::processing_state_listener* m_state_listener;
    semantics::source_info_processor* m_src_proc;
    diagnosable_ctx* m_diagnoser;

    opencode_provider_options m_opts;

    bool m_line_fed = false;

    std::unique_ptr<preprocessor> m_preprocessor;

public:
    // rewinds position in file
    void rewind_input(context::source_position pos);
    std::string aread();
    void ainsert(const std::string& rec, ainsert_destination dest);

    opencode_provider(std::string_view text,
        analyzing_context& ctx,
        workspaces::parse_lib_provider& lib_provider,
        processing::processing_state_listener& state_listener,
        semantics::source_info_processor& src_proc,
        diagnosable_ctx& diag_consumer,
        std::unique_ptr<preprocessor> preprocessor,
        opencode_provider_options opts);

    parsing::hlasmparser& parser(); // for testing only

    context::shared_stmt_ptr get_next(const processing::statement_processor& processor) override;

    bool finished() const override;

private:
    extract_next_logical_line_result feed_line(parsing::parser_holder& p);
    bool is_comment();
    void process_comment();
    void generate_aread_highlighting(std::string_view text, size_t line_no) const;
    bool is_next_line_ictl() const;
    bool is_next_line_process() const;
    void generate_continuation_error_messages(diagnostic_op_consumer* diags) const;
    extract_next_logical_line_result extract_next_logical_line_from_copy_buffer();
    extract_next_logical_line_result extract_next_logical_line();
    void apply_pending_line_changes();
    const parsing::parser_holder& prepare_operand_parser(const std::string& text,
        context::hlasm_context& hlasm_ctx,
        diagnostic_op_consumer* diag_collector,
        semantics::range_provider range_prov,
        range text_range,
        const processing_status& proc_status,
        bool unlimited_line);

    std::shared_ptr<const context::hlasm_statement> process_lookahead(const statement_processor& proc,
        semantics::collector& collector,
        const std::optional<std::string>& op_text,
        const range& op_range);
    std::shared_ptr<const context::hlasm_statement> process_ordinary(const statement_processor& proc,
        semantics::collector& collector,
        const std::optional<std::string>& op_text,
        const range& op_range,
        diagnostic_op_consumer* diags);

    bool try_running_preprocessor();
    bool suspend_copy_processing(bool remove_empty) const;
    void convert_ainsert_buffer_to_copybook();
};

} // namespace hlasm_plugin::parser_library::processing
#endif
