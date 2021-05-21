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

#include "feature.h"

#include <filesystem>

#include "network/uri/uri.hpp"

#include "logger.h"
#include "utils/path.h"
#include "utils/platform.h"

namespace hlasm_plugin::language_server {

const std::string untitled = "untitled";

std::string feature::uri_to_path(const std::string& uri)
{
    network::uri u(uri);

    // vscode sometimes sends us uri in form 'untitled:Untitled-1',
    // when user opens new file that is not saved to disk yet
    if (!u.scheme().compare(untitled))
        return uri;

    if (u.scheme().compare("file"))
        return "";
    if (!u.has_path())
        return "";
    network::string_view path = u.path();



    std::string auth_path;
    if (u.has_authority() && u.authority().to_string() != "")
    {
        auth_path = u.authority().to_string() + u.path().to_string();
        if (utils::platform::is_windows())
        {
            // handle remote locations correctly, like \\server\path
            auth_path = "//" + auth_path;
        }
    }
    else
    {
        if (utils::platform::is_windows())
        {
            // we get path always beginning with / on windows, e.g. /c:/Users/path
            path.remove_prefix(1);
        }
        auth_path = path.to_string();

        if (utils::platform::is_windows())
        {
            auth_path[0] = (char)tolower((unsigned char)auth_path[0]);
        }
    }

    return utils::path::lexically_normal(network::detail::decode(auth_path)).string();
}

std::string feature::path_to_uri(std::string_view path)
{
    if (path.substr(0, untitled.size()) == untitled)
        return std::string(path);

    // network::detail::encode_path(uri) ignores @, which is incompatible with VS Code
    std::string uri;
    auto out = std::back_inserter(uri);

    for (char c : path)
    {
        if (c == '\\')
            c = '/';
        network::detail::encode_char(c, out, "/.%;=");
    }

    if (utils::platform::is_windows())
    {
        // in case of remote address such as \\server\path\to\file
        if (uri.size() >= 2 && uri[0] == '/' && uri[1] == '/')
            uri.insert(0, "file:");
        else
            uri.insert(0, "file:///");
    }
    else
    {
        uri.insert(0, "file://");
    }

    return uri;
}

parser_library::range feature::parse_range(const json& range_json)
{
    return { parse_position(range_json["start"]), parse_position(range_json["end"]) };
}

parser_library::position feature::parse_position(const json& position_json)
{
    return { position_json["line"].get<nlohmann::json::number_unsigned_t>(),
        position_json["character"].get<nlohmann::json::number_unsigned_t>() };
}

json feature::range_to_json(const parser_library::range& range)
{
    return json { { "start", position_to_json(range.start) }, { "end", position_to_json(range.end) } };
}

json feature::position_to_json(const parser_library::position& position)
{
    return json { { "line", position.line }, { "character", position.column } };
}

} // namespace hlasm_plugin::language_server
