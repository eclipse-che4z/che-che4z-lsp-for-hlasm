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

#include "postponed_statement.h"

#include "processing/instruction_sets/postponed_statement_impl.h"

namespace hlasm_plugin::parser_library::context {

const processing::postponed_statement_impl* postponed_statement::impl() const
{
    return static_cast<const processing::postponed_statement_impl*>(this);
}

} // namespace hlasm_plugin::parser_library::context
