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

#include "error_statement.h"


namespace hlasm_plugin::parser_library::processing {


position error_statement::statement_position() const { return m_range.start; }

std::span<const diagnostic_op> error_statement::diagnostics() const
{
    return { m_errors.data(), m_errors.data() + m_errors.size() };
}

} // namespace hlasm_plugin::parser_library::processing