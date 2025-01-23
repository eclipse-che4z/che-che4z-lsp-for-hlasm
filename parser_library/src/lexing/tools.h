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

#ifndef HLASMPLUGIN_PARSER_LEXING_TOOLS_H
#define HLASMPLUGIN_PARSER_LEXING_TOOLS_H

#include <string_view>

namespace hlasm_plugin::parser_library::lexing {

bool is_valid_symbol_name(std::string_view s, bool extended_names_allowed = true);
bool is_ord_symbol(std::string_view s);

} // namespace hlasm_plugin::parser_library::lexing
#endif
