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

#ifndef PROCESSING_STATEMENT_FIELDS_PARSER_H
#define PROCESSING_STATEMENT_FIELDS_PARSER_H

#include "processing/op_code.h"
#include "semantics/range_provider.h"
#include "semantics/statement_fields.h"

namespace hlasm_plugin::parser_library::context {
class hlasm_context;
} // namespace hlasm_plugin::parser_library::context

namespace hlasm_plugin::parser_library::lexing {
struct u8string_view_with_newlines;
} // namespace hlasm_plugin::parser_library::lexing

namespace hlasm_plugin::parser_library::parsing {
class parser_error_listener_ctx;
class parser_holder;
} // namespace hlasm_plugin::parser_library::parsing

namespace hlasm_plugin::parser_library::processing {

class statement_provider;
using provider_ptr = std::unique_ptr<statement_provider>;

// (re-)parsing of deferred statement fields

class statement_fields_parser final
{
    std::unique_ptr<parsing::parser_holder> m_parser;
    context::hlasm_context* m_hlasm_ctx;

public:
    struct parse_result
    {
        semantics::operands_si operands;
        semantics::remarks_si remarks;
        std::vector<semantics::literal_si> literals;
    };

    parse_result parse_operand_field(lexing::u8string_view_with_newlines field,
        bool after_substitution,
        semantics::range_provider field_range,
        size_t logical_column,
        processing::processing_status status,
        diagnostic_op_consumer& add_diag);

    explicit statement_fields_parser(context::hlasm_context& hlasm_ctx);
    ~statement_fields_parser();
};

} // namespace hlasm_plugin::parser_library::processing
#endif
