/*
 * Copyright (c) 2022 Broadcom.
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

#include "utils/string_operations.h"

namespace hlasm_plugin::utils {

std::pair<std::string_view, size_t> trim_left(std::string_view s)
{
    size_t to_trim = s.find_first_not_of(" ");
    if (to_trim == std::string_view::npos)
        return { "", s.length() };

    s.remove_prefix(to_trim);
    return { s, to_trim };
}
} // namespace hlasm_plugin::utils
