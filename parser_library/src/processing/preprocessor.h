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

#include <deque>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include "diagnostic_consumer.h"
#include "document.h"
#include "utils/resource_location.h"

namespace hlasm_plugin::parser_library {
struct cics_preprocessor_options;
struct db2_preprocessor_options;
struct endevor_preprocessor_options;

namespace lexing {
struct logical_line;
struct logical_line_extractor_args;
} // namespace lexing

namespace semantics {
class source_info_processor;
struct preprocessor_statement_si;
} // namespace semantics

namespace context {
class id_storage;
}

} // namespace hlasm_plugin::parser_library

namespace hlasm_plugin::parser_library::processing {

using library_fetcher =
    std::function<std::optional<std::pair<std::string, hlasm_plugin::utils::resource::resource_location>>(
        std::string_view)>;

class preprocessor
{
public:
    struct included_member_details
    {
        std::string name;
        std::string text;
        utils::resource::resource_location loc;
    };

    virtual ~preprocessor() = default;

    virtual document generate_replacement(document doc) = 0;

    static std::unique_ptr<preprocessor> create(const cics_preprocessor_options&,
        library_fetcher,
        diagnostic_op_consumer*,
        semantics::source_info_processor&,
        context::id_storage&);

    static std::unique_ptr<preprocessor> create(const db2_preprocessor_options&,
        library_fetcher,
        diagnostic_op_consumer*,
        semantics::source_info_processor&,
        context::id_storage&);

    static std::unique_ptr<preprocessor> create(const endevor_preprocessor_options&,
        library_fetcher,
        diagnostic_op_consumer*,
        semantics::source_info_processor&,
        context::id_storage&);

    virtual std::vector<std::shared_ptr<semantics::preprocessor_statement_si>> take_statements();

    virtual const std::vector<std::unique_ptr<included_member_details>>& view_included_members();

protected:
    preprocessor() = default;
    preprocessor(const preprocessor&) = default;
    preprocessor(preprocessor&&) = default;

    using line_iterator = std::vector<document_line>::const_iterator;

    static line_iterator extract_nonempty_logical_line(lexing::logical_line& out,
        line_iterator it,
        line_iterator end,
        const lexing::logical_line_extractor_args& opts);

    void reset();
    void set_statement(std::shared_ptr<semantics::preprocessor_statement_si> stmt);
    void set_statements(std::vector<std::shared_ptr<semantics::preprocessor_statement_si>> stmts);

    static bool is_continued(std::string_view s);

    static void do_highlighting(
        const semantics::preprocessor_statement_si& stmt, semantics::source_info_processor& src_proc);

    void append_included_member(std::unique_ptr<included_member_details> details);
    void append_included_members(std::vector<std::unique_ptr<included_member_details>> details);
    void capture_included_members(preprocessor& preproc);

private:
    std::vector<std::shared_ptr<semantics::preprocessor_statement_si>> m_statements;
    std::vector<std::unique_ptr<included_member_details>> m_inc_members;
};
} // namespace hlasm_plugin::parser_library::processing

#endif
