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

#include "hlasm_statement.h"

#include "processing/statement.h"

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::context;

const processing::resolved_statement* hlasm_statement::access_resolved() const
{
    return kind == statement_kind::RESOLVED ? static_cast<const processing::resolved_statement*>(this) : nullptr;
}
processing::resolved_statement* hlasm_statement::access_resolved()
{
    return kind == statement_kind::RESOLVED ? static_cast<processing::resolved_statement*>(this) : nullptr;
}

const semantics::deferred_statement* hlasm_statement::access_deferred() const
{
    return kind == statement_kind::DEFERRED ? static_cast<const semantics::deferred_statement*>(this) : nullptr;
}
semantics::deferred_statement* hlasm_statement::access_deferred()
{
    return kind == statement_kind::DEFERRED ? static_cast<semantics::deferred_statement*>(this) : nullptr;
}

hlasm_statement::hlasm_statement(const statement_kind kind)
    : kind(kind)
{ }
