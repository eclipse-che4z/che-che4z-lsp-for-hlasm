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

#include "context/hlasm_context.h"
#include "processing/op_code.h"
#include "semantics/range_provider.h"
#include "semantics/statement_fields.h"

namespace hlasm_plugin::parser_library::parsing {
class parser_error_listener_ctx;
struct parser_holder;
} // namespace hlasm_plugin::parser_library::parsing

namespace hlasm_plugin::parser_library::processing {

class statement_provider;
using provider_ptr = std::unique_ptr<statement_provider>;

// (re-)parsing of deferred statement fields
class statement_fields_parser final : public diagnosable_impl
{
    std::unique_ptr<parsing::parser_holder> m_parser;
    context::hlasm_context* m_hlasm_ctx;

    const parsing::parser_holder& prepare_parser(const std::string& text,
        bool unlimited_line,
        semantics::range_provider field_range,
        processing::processing_status status,
        diagnostic_op_consumer& add_diag);

public:
    using parse_result = std::pair<semantics::operands_si, semantics::remarks_si>;

    parse_result parse_operand_field(std::string field,
        bool after_substitution,
        semantics::range_provider field_range,
        processing::processing_status status,
        diagnostic_op_consumer& add_diag);

    explicit statement_fields_parser(context::hlasm_context* hlasm_ctx);
    ~statement_fields_parser();

    void collect_diags() const override;
};

} // namespace hlasm_plugin::parser_library::processing
#endif
