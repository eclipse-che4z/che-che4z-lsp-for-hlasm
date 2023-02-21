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

#include <concepts>
#include <deque>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

#include "context/source_snapshot.h"
#include "lexing/logical_line.h"
#include "parsing/parser_error_listener.h"
#include "preprocessor.h"
#include "range.h"
#include "statement_providers/statement_provider.h"
#include "utils/unicode_text.h"
#include "virtual_file_monitor.h"


namespace hlasm_plugin::utils {
class task;
template<std::move_constructible T>
class value_task;
} // namespace hlasm_plugin::utils

namespace hlasm_plugin::parser_library {
class analyzer_options;
class diagnosable_ctx;
} // namespace hlasm_plugin::parser_library
namespace hlasm_plugin::parser_library::parsing {
class hlasmparser_multiline;
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
namespace hlasm_plugin::parser_library::workspaces {
class parse_lib_provider;
} // namespace hlasm_plugin::parser_library::workspaces

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
    document m_input_document;
    std::size_t m_next_line_index = 0;

    lexing::logical_line<utils::utf8_iterator<std::string_view::iterator, utils::utf8_utf16_counter>>
        m_current_logical_line;
    struct logical_line_origin
    {
        size_t begin_line;
        size_t first_index;
        size_t last_index;
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

    struct parser_set
    {
        std::unique_ptr<parsing::parser_holder> m_parser;
        std::unique_ptr<parsing::parser_holder> m_lookahead_parser;
        std::unique_ptr<parsing::parser_holder> m_operand_parser;
    };
    parser_set m_singleline;
    parser_set m_multiline;

    analyzing_context* m_ctx;
    workspaces::parse_lib_provider* m_lib_provider;
    processing::processing_state_listener* m_state_listener;
    semantics::source_info_processor* m_src_proc;
    diagnosable_ctx* m_diagnoser;

    opencode_provider_options m_opts;

    bool m_line_fed = false;

    std::unique_ptr<preprocessor> m_preprocessor;

    virtual_file_monitor* m_virtual_file_monitor;
    std::vector<virtual_file_handle> m_vf_handles;

public:
    // rewinds position in file
    void rewind_input(context::source_position pos);
    std::variant<std::string, utils::value_task<std::string>> aread();
    void ainsert(const std::string& rec, ainsert_destination dest);

    opencode_provider(std::string_view text,
        analyzing_context& ctx,
        workspaces::parse_lib_provider& lib_provider,
        processing::processing_state_listener& state_listener,
        semantics::source_info_processor& src_proc,
        diagnosable_ctx& diag_consumer,
        std::unique_ptr<preprocessor> preprocessor,
        opencode_provider_options opts,
        virtual_file_monitor* virtual_file_monitor);

    parsing::hlasmparser_multiline& parser(); // for testing only

    context::shared_stmt_ptr get_next(const processing::statement_processor& processor) override;

    bool finished() const override;

    processing::preprocessor* get_preprocessor();

    void onetime_action();

private:
    void feed_line(const parsing::parser_holder& p, bool is_process);
    bool is_comment();
    void process_comment();
    void generate_aread_highlighting(std::string_view text, size_t line_no) const;
    bool is_next_line_ictl() const;
    bool is_next_line_process() const;
    void generate_continuation_error_messages(diagnostic_op_consumer* diags) const;
    extract_next_logical_line_result extract_next_logical_line_from_copy_buffer();
    extract_next_logical_line_result extract_next_logical_line();

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

    bool should_run_preprocessor() const noexcept;
    utils::task run_preprocessor();
    enum class remove_empty : bool
    {
        no,
        yes,
    };
    bool suspend_copy_processing(remove_empty re) const;
    utils::task convert_ainsert_buffer_to_copybook();

    utils::task start_preprocessor();
    utils::task start_nested_parser(std::string_view text, analyzer_options opts, context::id_index vf_name) const;

    std::string aread_from_copybook() const;
    std::string try_aread_from_document();

    utils::value_task<std::string> deferred_aread(utils::task prep_task);
};

} // namespace hlasm_plugin::parser_library::processing
#endif
