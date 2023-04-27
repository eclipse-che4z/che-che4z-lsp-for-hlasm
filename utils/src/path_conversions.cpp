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

#include "utils/path_conversions.h"

#include <optional>
#include <regex>

#include "network/uri/uri.hpp"

#include "utils/encoding.h"
#include "utils/path.h"
#include "utils/platform.h"

namespace hlasm_plugin::utils::path {
const std::regex windows_drive("^[a-z]%3A");
const std::regex uri_unlike_windows_path("^[A-Za-z][A-Za-z0-9+\\-.]+:");
const std::regex uri_like_windows_path("^[A-Za-z](?::|%3[aA])");

std::string uri_to_path(std::string_view uri)
{
    std::error_code ec;
    network::uri u(uri.begin(), uri.end(), ec);

    if (ec)
        return std::string(uri);

    if (u.scheme().compare("file"))
        return "";
    if (!u.has_path())
        return "";

    std::string auth_path;
    if (u.has_authority() && u.authority().to_string() != "")
    {
        if (!utils::platform::is_windows())
            return ""; // There is no path representation for URIs like "file://share/home/etc" on linux

        auth_path = u.authority().to_string() + u.path().to_string();

        if (!std::regex_search(auth_path, windows_drive))
            // handle remote locations correctly, like \\server\path, if the auth doesn't start with e.g. C:/
            auth_path = "//" + auth_path;
    }
    else
    {
        network::string_view path = u.path();

        if (utils::platform::is_windows() && path.size() >= 2
            && ((path[0] == '/' || path[0] == '\\') && path[1] != '/' && path[1] != '\\'))
            // If Windows path begins with exactly 1 slash, remove it e.g. /c:/Users/path -> c:/Users/path
            path.remove_prefix(1);

        auth_path = path.to_string();

        if (utils::platform::is_windows())
        {
            auth_path[0] = (char)tolower((unsigned char)auth_path[0]);
        }
    }

    return utils::path::lexically_normal(network::detail::decode(auth_path)).string();
}

std::string path_to_uri(std::string_view path)
{
    // Don't consider one-letter schemes to be URI, consider them to be the beginnings of Windows path
    if (std::regex_search(path.begin(), path.end(), uri_unlike_windows_path))
        return std::string(path);

    // network::detail::encode_path(uri) ignores @, which is incompatible with VS Code
    std::string uri = utils::encoding::percent_encode(path);

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

bool is_uri(std::string_view path) noexcept
{
    if (path.empty())
        return false;

    // one letter schemas are valid, but Windows paths collide
    if (std::regex_search(path.begin(), path.end(), uri_like_windows_path))
        return false;

    std::error_code ec;
    network::uri u(path.begin(), path.end(), ec);

    return ec.value() == 0;
}

dissected_uri dissect_uri(std::string_view uri)
{
    dissected_uri dis_uri;

    std::error_code ec;
    network::uri u(uri.begin(), uri.end(), ec);

    if (ec)
        return dis_uri;

    // Process non-authority parts
    if (u.has_scheme())
        dis_uri.scheme = u.scheme().to_string();
    if (u.has_path())
        dis_uri.path = u.path().to_string();
    if (u.has_query())
        dis_uri.query = u.query().to_string();
    if (u.has_fragment())
        dis_uri.fragment = u.fragment().to_string();

    // Process authority parts
    std::optional<std::string> user_info;
    std::optional<std::string> host;
    std::optional<std::string> port;

    if (u.has_user_info())
        user_info = u.user_info().to_string();
    if (u.has_host())
        host = u.host().to_string();
    if (u.has_port())
        port = u.port().to_string();

    if (user_info.has_value() || host.has_value() || port.has_value())
    {
        dis_uri.auth =
            dissected_uri::authority { std::move(user_info), std::move(host).value_or(std::string()), std::move(port) };
    }

    return dis_uri;
}

std::string reconstruct_uri(const dissected_uri& dis_uri)
{
    std::string uri;

    uri.append(dis_uri.scheme);
    uri.push_back(':');

    if (dis_uri.auth)
    {
        uri.append("//");
        if (dis_uri.auth->user_info)
        {
            uri.append(*dis_uri.auth->user_info);
            uri.push_back('@');
        }
        uri.append(dis_uri.auth->host);
        if (dis_uri.auth->port)
        {
            uri.push_back(':');
            uri.append(*dis_uri.auth->port);
        }
    }

    uri.append(dis_uri.path);

    if (dis_uri.query)
    {
        uri.push_back('?');
        uri.append(*dis_uri.query);
    }

    if (dis_uri.fragment)
    {
        uri.push_back('#');
        uri.append(*dis_uri.fragment);
    }

    return uri;
}

namespace {
void format_path_pre_processing(std::string& hostname, std::string_view port, std::string& path)
{
    if (!port.empty() && !hostname.empty())
        hostname.append(":").append(port);

    if (!hostname.empty() && !path.empty() && path[0] == '/')
        path = path.substr(1, path.size() - 1);
}

void format_path_post_processing_win(std::string_view hostname, std::string& path)
{
    if (!path.empty() && hostname.empty())
        path.erase(0, 1);
}

std::string decorate_path(const std::optional<dissected_uri::authority>& auth, std::string path)
{
    std::string hostname;
    std::string port;

    if (auth.has_value())
    {
        hostname = auth->host;

        if (auth->port.has_value())
            port = *auth->port;
    }

    format_path_pre_processing(hostname, port, path);

    std::string s = utils::path::lexically_normal(utils::path::join(hostname, path)).string();

    if (utils::platform::is_windows())
        format_path_post_processing_win(hostname, s);

    return network::detail::decode(s);
}

void handle_local_host_file_scheme(dissected_uri& dis_uri)
{
    // Decorate path with authority
    dis_uri.path = decorate_path(dis_uri.auth, std::move(dis_uri.path));

    // Clear not useful stuff before it goes to printer
    dis_uri.scheme.clear();
    dis_uri.auth = std::nullopt;
}

void to_presentable_pre_processing(dissected_uri& dis_uri)
{
    if (dis_uri.contains_host())
        dis_uri.auth->host.insert(0, "//");

    if (dis_uri.scheme == "file")
    {
        if (utils::platform::is_windows())
            handle_local_host_file_scheme(dis_uri);
        else if (!dis_uri.contains_host())
            handle_local_host_file_scheme(dis_uri);
    }
}

std::string to_presentable_internal(const dissected_uri& dis_uri)
{
    std::string s;

    if (!dis_uri.scheme.empty())
        s.append(dis_uri.scheme).append(":");

    if (dis_uri.auth.has_value())
    {
        s.append(dis_uri.auth->host);

        if (dis_uri.auth->port.has_value())
            s.append(":").append(*dis_uri.auth->port);
    }

    s.append(dis_uri.path);

    // TODO Think about presenting query and fragment parts

    return s;
}

std::string to_presentable_internal_debug(const dissected_uri& dis_uri, std::string_view raw_uri)
{
    std::string s;
    s.append("Scheme: ").append(dis_uri.scheme).append("\n");
    if (dis_uri.auth.has_value())
    {
        const auto& auth = *dis_uri.auth;
        if (auth.user_info.has_value())
            s.append("User info: ").append(*auth.user_info).append("\n");
        s.append("Hostname: ").append(auth.host).append("\n");
        if (auth.port.has_value())
            s.append("Port: ").append(*auth.port).append("\n");
    }
    s.append("Path: ").append(dis_uri.path).append("\n");
    if (dis_uri.query.has_value())
        s.append("Query: ").append(*dis_uri.query).append("\n");
    if (dis_uri.fragment.has_value())
        s.append("Fragment: ").append(*dis_uri.fragment).append("\n");
    s.append("Raw URI: ").append(raw_uri);

    return s;
}
} // namespace

std::string get_presentable_uri(std::string_view uri, bool debug)
{
    dissected_uri dis_uri = dissect_uri(uri);

    if (debug)
        return to_presentable_internal_debug(dis_uri, uri);
    else
    {
        to_presentable_pre_processing(dis_uri);
        return to_presentable_internal(dis_uri);
    }
}

} // namespace hlasm_plugin::utils::path
