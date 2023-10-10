/*
 * Copyright (c) 2023 Broadcom.
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

#ifndef HLASMPLUGIN_PARSERLIBRARY_CONTEXT_STATEMENT_ID_H
#define HLASMPLUGIN_PARSERLIBRARY_CONTEXT_STATEMENT_ID_H

#include <cstdint>

namespace hlasm_plugin::parser_library::context {
struct statement_id
{
    std::size_t value = (std::size_t)-1;

    bool operator==(const statement_id&) const noexcept = default;
};
} // namespace hlasm_plugin::parser_library::context

#endif
