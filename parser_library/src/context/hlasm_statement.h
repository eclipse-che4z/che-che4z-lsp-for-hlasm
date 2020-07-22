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

#ifndef CONTEXT_HLASM_STATEMENT_H
#define CONTEXT_HLASM_STATEMENT_H

#include <memory>
#include <vector>

#include "range.h"

namespace hlasm_plugin::parser_library::semantics {
struct partial_statement;
struct complete_statement;
} // namespace hlasm_plugin::parser_library::semantics

namespace hlasm_plugin::parser_library::context {

struct hlasm_statement;

using shared_stmt_ptr = std::shared_ptr<const hlasm_statement>;
using unique_stmt_ptr = std::unique_ptr<hlasm_statement>;

using statement_block = std::vector<shared_stmt_ptr>;

enum class statement_kind
{
    COMPLETE,
    PARTIAL
};

// abstract structure representing general HLASM statement
struct hlasm_statement
{
    const statement_kind kind;

    const semantics::complete_statement* access_complete() const;
    semantics::complete_statement* access_complete();

    const semantics::partial_statement* access_partial() const;
    semantics::partial_statement* access_partial();

    virtual position statement_position() const = 0;

    virtual ~hlasm_statement() = default;

protected:
    hlasm_statement(const statement_kind kind);
};


} // namespace hlasm_plugin::parser_library::context

#endif