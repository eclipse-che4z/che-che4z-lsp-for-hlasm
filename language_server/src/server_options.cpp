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

#include "server_options.h"

#include <charconv>

#include "logger.h"

namespace hlasm_plugin::language_server {

std::optional<server_options> parse_options(std::span<const char* const> args)
{
    server_options result {};

    for (std::string_view arg : args)
    {
        if (arg == "--vscode-extensions")
        {
            result.enable_vscode_extension = true;
        }
        else if (static constexpr std::string_view log_level = "--log-level="; arg.starts_with(log_level))
        {
            arg.remove_prefix(log_level.size());
            unsigned ll = 0;
            auto [ptr, err] = std::from_chars(std::to_address(arg.begin()), std::to_address(arg.end()), ll);
            if (err != std::errc {} || ptr != std::to_address(arg.end()) || ll > logger::max_log_level)
                return std::nullopt;
            result.log_level = (signed char)ll;
        }
        else if (static constexpr std::string_view lsp_tcp_port = "--lsp-port="; arg.starts_with(lsp_tcp_port))
        {
            arg.remove_prefix(lsp_tcp_port.size());
            auto [ptr, err] = std::from_chars(std::to_address(arg.begin()), std::to_address(arg.end()), result.port);
            if (err != std::errc {} || ptr != std::to_address(arg.end()) || result.port == 0)
                return std::nullopt;
        }
        else
            return std::nullopt;
    }

    return result;
}

} // namespace hlasm_plugin::language_server
