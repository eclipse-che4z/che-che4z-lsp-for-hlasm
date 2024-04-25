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

#ifndef HLASMPARSER_PARSERLIBRARY_PROCESSING_ERROR_STATEMENT_H
#define HLASMPARSER_PARSERLIBRARY_PROCESSING_ERROR_STATEMENT_H

#include "context/hlasm_statement.h"
#include "diagnostic_op.h"

namespace hlasm_plugin::parser_library::processing {

class error_statement : public context::hlasm_statement
{
    std::vector<diagnostic_op> m_errors;
    range m_range;

public:
    error_statement(range r, std::vector<diagnostic_op>&& diags)
        : hlasm_statement(context::statement_kind::ERROR)
        , m_errors(std::make_move_iterator(diags.begin()), std::make_move_iterator(diags.end()))
        , m_range(r)
    {}

    position statement_position() const override;

    std::span<const diagnostic_op> diagnostics() const override;
};

} // namespace hlasm_plugin::parser_library::processing

#endif // HLASMPARSER_PARSERLIBRARY_PROCESSING_ERROR_STATEMENT_H
