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
#include <utility>
#include <vector>

#include "range.h"

namespace hlasm_plugin::parser_library {
struct diagnostic_op;
} // namespace hlasm_plugin::parser_library
namespace hlasm_plugin::parser_library::semantics {
struct deferred_statement;
} // namespace hlasm_plugin::parser_library::semantics
namespace hlasm_plugin::parser_library::processing {
struct resolved_statement;
} // namespace hlasm_plugin::parser_library::processing

namespace hlasm_plugin::parser_library::context {

struct hlasm_statement;

using shared_stmt_ptr = std::shared_ptr<const hlasm_statement>;

using statement_block = std::vector<shared_stmt_ptr>;

enum class statement_kind
{
    RESOLVED,
    DEFERRED
};

// abstract structure representing general HLASM statement
struct hlasm_statement
{
    const statement_kind kind;

    const processing::resolved_statement* access_resolved() const;
    processing::resolved_statement* access_resolved();

    const semantics::deferred_statement* access_deferred() const;
    semantics::deferred_statement* access_deferred();

    virtual position statement_position() const = 0;

    virtual std::pair<const diagnostic_op*, const diagnostic_op*> diagnostics() const { return {}; }

    virtual ~hlasm_statement() = default;

protected:
    hlasm_statement(const statement_kind kind);
};


} // namespace hlasm_plugin::parser_library::context

#endif