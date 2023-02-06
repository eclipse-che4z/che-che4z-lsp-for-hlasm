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

#ifndef HLASMPLUGIN_PARSERLIBRARY_WILDCARD_H
#define HLASMPLUGIN_PARSERLIBRARY_WILDCARD_H

#include <regex>

namespace hlasm_plugin::parser_library::workspaces {
// Returns a regex that can be used for wildcard matching.
std::regex wildcard2regex(std::string wildcard);
std::regex percent_encoded_pathmask_to_regex(std::string_view s);

} // namespace hlasm_plugin::parser_library::workspaces

#endif
