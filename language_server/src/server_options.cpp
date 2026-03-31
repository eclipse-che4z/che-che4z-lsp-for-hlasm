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

constexpr std::string_view supported_pseudo_charsets[] = { "IBM1148", "IBM1143", "IBM278" };
std::string_view to_string(pseudo_charsets pc) { return supported_pseudo_charsets[static_cast<size_t>(pc)]; }

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
        else if (static constexpr std::string_view pseudo_charset = "--pseudo-charset=";
                 arg.starts_with(pseudo_charset))
        {
            arg.remove_prefix(pseudo_charset.size());

            const auto pc = std::ranges::find(supported_pseudo_charsets, arg);
            if (pc == std::ranges::end(supported_pseudo_charsets))
                return std::nullopt;

            const auto index = std::ranges::distance(std::ranges::begin(supported_pseudo_charsets), pc);
            result.pseudo_charset = static_cast<pseudo_charsets>(index);
        }
        else
            return std::nullopt;
    }

    return result;
}

} // namespace hlasm_plugin::language_server
