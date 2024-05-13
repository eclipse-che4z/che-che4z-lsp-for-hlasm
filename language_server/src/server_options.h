/*
 * Copyright (c) 2024 Broadcom.
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

#ifndef HLASMPLUGIN_HLASMLANGUAGESERVER_SERVER_OPTIONS_H
#define HLASMPLUGIN_HLASMLANGUAGESERVER_SERVER_OPTIONS_H

#include <cstdint>
#include <optional>
#include <span>

namespace hlasm_plugin::language_server {
struct server_options
{
    uint16_t port = 0;
    bool enable_vscode_extension = false;
    signed char log_level = -1;
};
std::optional<server_options> parse_options(std::span<const char* const> args);

} // namespace hlasm_plugin::language_server

#endif // !HLASMPLUGIN_HLASMLANGUAGESERVER_SERVER_OPTIONS_H
