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

#ifndef CONTEXT_POSTPONED_STATEMENT_H
#define CONTEXT_POSTPONED_STATEMENT_H

#include "context/source_context.h"

namespace hlasm_plugin::parser_library::processing {
struct postponed_statement_impl;
}

namespace hlasm_plugin::parser_library::context {

// statement whose check was postponed because of unsolved dependencies
// contains stack of file positions from where was it postponed
struct postponed_statement
{
    postponed_statement(processing_stack_t location_stack, const processing::resolved_statement* resolved_stmt)
        : location_stack(location_stack)
        , resolved_stmt(resolved_stmt)
    {}

    processing_stack_t location_stack;

    const processing::resolved_statement* resolved_stmt;

    virtual ~postponed_statement() = default;
};

using post_stmt_ptr = std::unique_ptr<postponed_statement>;

} // namespace hlasm_plugin::parser_library::context

#endif
