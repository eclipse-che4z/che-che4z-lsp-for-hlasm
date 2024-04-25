/*
 * Copyright (c) 2021 Broadcom.
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

#ifndef HLASMPARSER_PARSERLIBRARY_PROCESSING_PREPROCESSOR_H
#define HLASMPARSER_PARSERLIBRARY_PROCESSING_PREPROCESSOR_H

#include <concepts>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "document.h"
#include "utils/resource_location.h"

namespace hlasm_plugin::parser_library {
struct cics_preprocessor_options;
struct db2_preprocessor_options;
struct endevor_preprocessor_options;

template<typename T>
class diagnostic_consumer_t;
struct diagnostic_op;

namespace lexing {
template<typename It>
struct logical_line;
struct logical_line_extractor_args;
} // namespace lexing

namespace semantics {
class source_info_processor;
struct preprocessor_statement_si;
} // namespace semantics
} // namespace hlasm_plugin::parser_library

namespace hlasm_plugin::utils {
template<std::move_constructible T>
class value_task;
} // namespace hlasm_plugin::utils

namespace hlasm_plugin::parser_library::processing {

using library_fetcher =
    std::function<utils::value_task<std::optional<std::pair<std::string, utils::resource::resource_location>>>(
        std::string)>;

class preprocessor
{
public:
    using line_iterator = std::vector<document_line>::const_iterator;

    struct included_member_details
    {
        std::string name;
        std::string text;
        utils::resource::resource_location loc;
    };

    virtual ~preprocessor() = default;

    [[nodiscard]] virtual utils::value_task<document> generate_replacement(document doc) = 0;

    static std::unique_ptr<preprocessor> create(const cics_preprocessor_options&,
        library_fetcher,
        diagnostic_consumer_t<diagnostic_op>*,
        semantics::source_info_processor&);

    static std::unique_ptr<preprocessor> create(const db2_preprocessor_options&,
        library_fetcher,
        diagnostic_consumer_t<diagnostic_op>*,
        semantics::source_info_processor&);

    static std::unique_ptr<preprocessor> create(const endevor_preprocessor_options&,
        library_fetcher,
        diagnostic_consumer_t<diagnostic_op>*,
        semantics::source_info_processor&);

    virtual std::vector<std::shared_ptr<semantics::preprocessor_statement_si>> take_statements();

    virtual const std::vector<std::unique_ptr<included_member_details>>& view_included_members();

    static line_iterator extract_nonempty_logical_line(lexing::logical_line<std::string_view::iterator>& out,
        line_iterator it,
        line_iterator end,
        const lexing::logical_line_extractor_args& opts);

protected:
    preprocessor() = default;
    preprocessor(const preprocessor&) = default;
    preprocessor(preprocessor&&) = default;

    void reset();
    void set_statement(std::shared_ptr<semantics::preprocessor_statement_si> stmt);
    void set_statements(std::vector<std::shared_ptr<semantics::preprocessor_statement_si>> stmts);

    static bool is_continued(std::string_view s);

    virtual void do_highlighting(const semantics::preprocessor_statement_si& stmt,
        semantics::source_info_processor& src_proc,
        size_t continue_column = 15) const;

    virtual void do_highlighting(const semantics::preprocessor_statement_si& stmt,
        const lexing::logical_line<std::string_view::iterator>& ll,
        semantics::source_info_processor& src_proc,
        size_t continue_column = 15) const;

    void append_included_member(std::unique_ptr<included_member_details> details);
    void append_included_members(std::vector<std::unique_ptr<included_member_details>> details);
    void capture_included_members(preprocessor& preproc);

private:
    std::vector<std::shared_ptr<semantics::preprocessor_statement_si>> m_statements;
    std::vector<std::unique_ptr<included_member_details>> m_inc_members;
};
} // namespace hlasm_plugin::parser_library::processing

#endif
