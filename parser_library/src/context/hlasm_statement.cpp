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

#include "semantics/statement.h"

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::context;

const semantics::complete_statement* hlasm_statement::access_complete() const
{
    return kind == statement_kind::COMPLETE ? static_cast<const semantics::complete_statement*>(this) : nullptr;
}
semantics::complete_statement* hlasm_statement::access_complete()
{
    return kind == statement_kind::COMPLETE ? static_cast<semantics::complete_statement*>(this) : nullptr;
}

const semantics::partial_statement* hlasm_statement::access_partial() const
{
    return kind == statement_kind::PARTIAL ? static_cast<const semantics::partial_statement*>(this) : nullptr;
}
semantics::partial_statement* hlasm_statement::access_partial()
{
    return kind == statement_kind::PARTIAL ? static_cast<semantics::partial_statement*>(this) : nullptr;
}

hlasm_statement::hlasm_statement(const statement_kind kind)
    : kind(kind)
{}
