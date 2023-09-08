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

#ifndef HLASMPLUGIN_UTILS_PLATFORM_H
#define HLASMPLUGIN_UTILS_PLATFORM_H

#include <optional>
#include <string>

namespace hlasm_plugin::utils::platform {
bool is_windows();
bool is_web();
void log(std::string_view);
const std::string& home();
std::optional<std::string> read_file(const std::string& file);

} // namespace hlasm_plugin::utils::platform

#endif
