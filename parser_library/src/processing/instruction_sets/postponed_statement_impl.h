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

#ifndef PROCESSING_POSTPONED_STATEMENT_IMPL_H
#define PROCESSING_POSTPONED_STATEMENT_IMPL_H

#include <variant>

#include "context/ordinary_assembly/postponed_statement.h"
#include "processing/statement.h"

namespace hlasm_plugin::parser_library::processing {

// implementation of postponed_statement interface
struct postponed_statement_impl : public context::postponed_statement
{
    postponed_statement_impl(rebuilt_statement&& s, context::processing_stack_t stmt_location_stack)
        : context::postponed_statement(stmt_location_stack, &stmt)
        , stmt(std::move(s))
    {}

    rebuilt_statement stmt;
};

} // namespace hlasm_plugin::parser_library::processing
#endif
