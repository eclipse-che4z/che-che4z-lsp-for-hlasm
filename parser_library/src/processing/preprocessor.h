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
#include <variant>
#include <vector>

#include "diagnostic_consumer.h"
#include "document.h"

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

using library_fetcher = std::function<std::optional<std::string>(std::string_view)>;

class preprocessor
{
public:
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

    virtual void finished() = 0;

    virtual void collect_statements(
        std::vector<std::unique_ptr<semantics::preprocessor_statement_si>>& statement_collector) = 0;
    virtual const std::vector<std::unique_ptr<semantics::preprocessor_statement_si>>& get_statements() const = 0;

protected:
    using line_iterator = std::vector<document_line>::const_iterator;

    static line_iterator extract_nonempty_logical_line(lexing::logical_line& out,
        line_iterator it,
        line_iterator end,
        const lexing::logical_line_extractor_args& opts);

    static bool is_continued(std::string_view s);

    static void do_highlighting(
        const semantics::preprocessor_statement_si& stmt, semantics::source_info_processor& src_proc);
};
} // namespace hlasm_plugin::parser_library::processing

#endif
