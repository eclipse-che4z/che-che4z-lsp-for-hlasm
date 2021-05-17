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

#include <string_view>

#include "context/source_snapshot.h"
#include "lexing/logical_line.h"
#include "statement_providers/statement_provider.h"

namespace hlasm_plugin::parser_library::parsing {
class hlasmparser;
class parser_error_listener;
class parser_error_listener_ctx;
struct parser_holder;
} // namespace hlasm_plugin::parser_library::parsing
namespace hlasm_plugin::parser_library::semantics {
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

// interface for hiding parser implementation
class opencode_provider final : public diagnosable_impl, public statement_provider
{
    struct lines_to_remove
    {
        size_t ainsert_buffer;
        size_t copy_files;
        size_t preprocessor_buffer;
        size_t current_text_lines;
    };
    lines_to_remove m_lines_to_remove = {};

    std::string_view m_original_text;

    std::string_view m_current_text;
    size_t m_current_line = 0;

    lexing::logical_line m_current_logical_line;

    std::deque<std::string> m_ainsert_buffer;
    std::vector<std::string_view> m_copy_files;

    std::vector<std::string> m_preprocessor_buffer;

    std::unique_ptr<parsing::parser_holder> m_parser;
    std::unique_ptr<parsing::parser_holder> m_second_parser;

    analyzing_context* m_ctx;
    workspaces::parse_lib_provider* m_lib_provider;
    processing::processing_state_listener* m_state_listener;
    semantics::source_info_processor* m_src_proc;

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
        parsing::parser_error_listener& err_listener);

    bool feed_line();
    bool process_comment();
    context::shared_stmt_ptr get_next(const processing::statement_processor& processor) override;

    bool finished() const override;
    bool line_fed = false;
    parsing::hlasmparser& parser();

    void collect_diags() const override;

private:
    bool extract_next_logical_line();
    void apply_pending_line_changes();
    const parsing::parser_holder& prepare_second_parser(const std::string& text,
        context::hlasm_context& hlasm_ctx,
        parsing::parser_error_listener_ctx& err_listener,
        semantics::range_provider& range_prov,
        range text_range,
        const processing_status& proc_status,
        bool unlimited_line);
};

} // namespace hlasm_plugin::parser_library::processing
#endif
