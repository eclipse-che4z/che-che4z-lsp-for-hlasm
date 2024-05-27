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

#include "logical_line.h"

namespace hlasm_plugin::parser_library::lexing {
std::pair<std::string_view, logical_line_segment_eol> extract_line(std::string_view& input)
{
    auto it = input.begin();
    auto [its, eol] = extract_line(it, input.end());
    input.remove_prefix(std::ranges::distance(input.begin(), it));

    return std::make_pair(std::string_view(its.first, its.second), eol);
}
} // namespace hlasm_plugin::parser_library::lexing
