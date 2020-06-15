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
#ifdef _WIN32 // handle remote locations correctly, like \\server\path
        auth_path = "//" + auth_path;
#endif
    }
    else
    {
#ifdef _WIN32 // we get path always beginning with / on windows, e.g. /c:/Users/path
        path.remove_prefix(1);
#endif
        auth_path = path.to_string();

#ifdef _WIN32
        auth_path[0] = (char)tolower(auth_path[0]);
#endif
    }

    std::filesystem::path p(network::detail::decode(auth_path));
    return p.lexically_normal().string();
}

std::string feature::path_to_uri(std::string path)
{
    if (path.substr(0, untitled.size()) == untitled)
        return path;

    std::replace(path.begin(), path.end(), '\\', '/');
    // network::detail::encode_path(uri) ignores @, which is incompatible with VS Code
    std::string uri;
    auto out = std::back_inserter(uri);
    auto it = path.cbegin();
    while (it != path.cend())
    {
        network::detail::encode_char(*it, out, "/.%;=");
        ++it;
    }

#ifdef _WIN32
    // in case of remote address such as \\server\path\to\file
    if (uri.size() >= 2 && uri[0] == '/' && uri[1] == '/')
        uri.insert(0, "file:");
    else
        uri.insert(0, "file:///");
#else
    uri.insert(0, "file://");
#endif // _WIN32

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

json feature::range_to_json(const parser_library::range & range)
{
    return json { { "start", position_to_json(range.start) }, { "end", position_to_json(range.end) } };
}

json feature::position_to_json(const parser_library::position & position)
{
    return json { { "line", position.line }, { "character", position.column } };
}

} // namespace hlasm_plugin::language_server
