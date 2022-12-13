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

#ifndef HLASMPLUGIN_UTILS_STRING_OPERATIONS_H
#define HLASMPLUGIN_UTILS_STRING_OPERATIONS_H

#include <cctype>
#include <string_view>
#include <utility>

namespace hlasm_plugin::utils {

std::pair<std::string_view, size_t> trim_left(std::string_view s);

inline bool isblank32(char32_t c) { return c <= 255 && std::isblank(static_cast<unsigned char>(c)); }

} // namespace hlasm_plugin::utils

#endif