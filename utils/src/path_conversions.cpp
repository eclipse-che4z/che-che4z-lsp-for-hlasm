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

#include <algorithm>
#include <optional>
#include <regex>

#include "utils/encoding.h"
#include "utils/path.h"
#include "utils/platform.h"
#include "utils/truth_table.h"

namespace hlasm_plugin::utils::path {
const std::regex windows_drive("^[a-zA-Z]%3A");
const std::regex uri_unlike_windows_path("^[A-Za-z][A-Za-z0-9+\\-.]+:");
const std::regex uri_like_windows_path("^[A-Za-z](?::|%3[aA])");

std::string uri_to_path(std::string_view uri)
{
    auto dis_uri = dissect_uri(uri);

    if (!dis_uri)
        return std::string(uri);

    if (dis_uri->scheme.compare("file"))
        return "";
    if (dis_uri->path.empty())
        return "";

    std::string auth_path;
    if (dis_uri->contains_host())
    {
        if (!utils::platform::is_windows())
            return ""; // There is no path representation for URIs like "file://share/home/etc" on linux
        if (dis_uri->auth->user_info.has_value() || dis_uri->auth->port.has_value())
            return ""; // Credentials cannot be included in the UNC path

        auth_path.reserve(dis_uri->auth->host.size() + dis_uri->path.size() + 2);
        auth_path.assign(dis_uri->auth->host.cbegin(), dis_uri->auth->host.cend());
        auth_path.append(dis_uri->path.cbegin(), dis_uri->path.cend());

        if (!std::regex_search(auth_path, windows_drive))
            // handle remote locations correctly, like \\server\path, if the auth doesn't start with e.g. C:/
            auth_path.insert(0, "//");
    }
    else
    {
        auto path = dis_uri->path;

        if (utils::platform::is_windows() && path.size() >= 2
            && ((path[0] == '/' || path[0] == '\\') && path[1] != '/' && path[1] != '\\'))
            // If Windows path begins with exactly 1 slash, remove it e.g. /c:/Users/path -> c:/Users/path
            path.remove_prefix(1);

        auth_path.assign(path.cbegin(), path.cend());

        if (utils::platform::is_windows())
        {
            auth_path[0] = (char)tolower((unsigned char)auth_path[0]);
        }
    }

    return utils::path::lexically_normal(utils::encoding::percent_decode(auth_path)).string();
}

std::string path_to_uri(std::string_view path)
{
    // Don't consider one-letter schemes to be URI, consider them to be the beginnings of Windows path
    if (std::regex_search(path.begin(), path.end(), uri_unlike_windows_path))
        return std::string(path);

    std::string uri = utils::encoding::percent_encode_path(path);

    if (utils::platform::is_windows())
    {
        // in case of remote address such as \\server\path\to\file
        if (uri.size() >= 2 && uri[0] == '/' && uri[1] == '/')
            uri.insert(0, "file:");
        else
        {
            if (!uri.empty())
                uri.front() = (char)tolower((unsigned char)uri.front());
            uri.insert(0, "file:///");
        }
    }
    else
    {
        uri.insert(0, "file://");
    }

    return uri;
}

bool is_likely_uri(std::string_view path)
{
    return std::regex_search(path.begin(), path.end(), uri_unlike_windows_path);
}

namespace {
enum class char_type : unsigned char
{
    alpha = 1,
    num = 2,
    unreserved = 4,
    hexnum = 8,
    scheme_extra = 16,
    subdelim_extra = 32,
    end_extra = 64,
    userinfo_extra = 128,
};

constexpr char_type operator|(char_type l, char_type r) noexcept
{
    return (char_type)((unsigned char)l | (unsigned char)r);
}

constexpr char_type operator&(char_type l, char_type r) noexcept
{
    return (char_type)((unsigned char)l & (unsigned char)r);
}


constexpr auto char_types = []() {
    auto alpha = utils::create_truth_table(u8"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ", char_type::alpha);
    auto numbers = utils::create_truth_table(u8"0123456789", char_type::num);
    auto unreserved = utils::create_truth_table(u8"-._~", char_type::unreserved);
    auto hexnumbers = utils::create_truth_table(u8"0123456789abcdefABCDEF", char_type::hexnum);
    auto scheme_extra = utils::create_truth_table(u8"+-.", char_type::scheme_extra);
    auto subdelim_extra = utils::create_truth_table(u8"!$&'()*+,;=", char_type::subdelim_extra);
    auto end_extra = utils::create_truth_table(u8"@:/?", char_type::end_extra);
    auto userinfo_extra = utils::create_truth_table(u8":", char_type::userinfo_extra);

    decltype(alpha) result {};

    std::ranges::transform(alpha, result, result.begin(), std::bit_or());
    std::ranges::transform(numbers, result, result.begin(), std::bit_or());
    std::ranges::transform(unreserved, result, result.begin(), std::bit_or());
    std::ranges::transform(hexnumbers, result, result.begin(), std::bit_or());
    std::ranges::transform(scheme_extra, result, result.begin(), std::bit_or());
    std::ranges::transform(subdelim_extra, result, result.begin(), std::bit_or());
    std::ranges::transform(end_extra, result, result.begin(), std::bit_or());
    std::ranges::transform(userinfo_extra, result, result.begin(), std::bit_or());

    return result;
}();

constexpr auto scheme_type = char_type::alpha | char_type::num | char_type::scheme_extra;
constexpr auto unreserved = char_type::alpha | char_type::num | char_type::unreserved;
constexpr auto subdelim_type = unreserved | char_type::subdelim_extra;
constexpr auto userinfo_type = subdelim_type | char_type::userinfo_extra;
constexpr auto host_type = subdelim_type;
constexpr auto path_eq_type = subdelim_type | char_type::end_extra; // '?' not in path by construction
constexpr auto query_type = subdelim_type | char_type::end_extra;
constexpr auto fragment_type = query_type;

bool is_valid(unsigned char c, char_type ct)
{
    static_assert((unsigned char)-1 < char_types.size());

    return (char_types[c] & ct) != char_type {};
}

enum class pct : bool
{
    no = false,
    yes = true,
};

bool is_valid(std::string_view s, char_type type, pct allow_pct)
{
    for (size_t i = 0; i < s.size(); ++i)
    {
        unsigned char c = s[i];
        if (is_valid(c, type))
            continue;
        if (allow_pct == pct::no || c != (unsigned char)'%')
            return false;
        if (s.size() - i < 3)
            return false;
        if (!is_valid(s[i + 1], char_type::hexnum))
            return false;
        if (!is_valid(s[i + 2], char_type::hexnum))
            return false;
        i += 2;
    }
    return true;
}

void validate_or_reset(std::optional<dissected_uri_view>& dis_uri)
{
    if (!dis_uri.has_value())
        return;

    static_assert((unsigned char)-1 < char_types.size());

    // Single letter scheme is by our definition an invalid one
    if (dis_uri->scheme.size() <= 1 || !is_valid(dis_uri->scheme.front(), char_type::alpha))
    {
        dis_uri.reset();
        return;
    }
    if (!is_valid(dis_uri->scheme, scheme_type, pct::no))
    {
        dis_uri.reset();
        return;
    }
    if (dis_uri->auth)
    {
        if (dis_uri->auth->user_info && !is_valid(*dis_uri->auth->user_info, userinfo_type, pct::yes))
        {
            dis_uri.reset();
            return;
        }
        if (dis_uri->auth->port && !is_valid(*dis_uri->auth->port, char_type::num, pct::no))
        {
            dis_uri.reset();
            return;
        }
        if (const auto host = dis_uri->auth->host; host.starts_with("[") && host.ends_with("]")) { /* IPv6+ */ }
        else if (host.size() >= 2 && is_valid(host.front(), char_type::num) && is_valid(host.back(), char_type::num))
        { /*IPv4*/ }
        else if (is_valid(host, host_type, pct::yes)) { /*hostname*/ }
        else
        {
            dis_uri.reset();
            return;
        }
    }
    if (!is_valid(dis_uri->path, path_eq_type, pct::yes))
    {
        dis_uri.reset();
        return;
    }
    if (dis_uri->query && !is_valid(*dis_uri->query, query_type, pct::yes))
    {
        dis_uri.reset();
        return;
    }
    if (dis_uri->fragment && !is_valid(*dis_uri->fragment, fragment_type, pct::yes))
    {
        dis_uri.reset();
        return;
    }
}
} // namespace

std::optional<dissected_uri_view> dissect_uri(std::string_view uri)
{
    std::optional<dissected_uri_view> result;

    auto& dis_uri = result.emplace();

    // missing scheme or scheme has one lettter => not considered URL for our purposes
    if (const auto scheme = uri.find(':'); scheme == std::string_view::npos || scheme <= 1)
        return result.reset(), result;
    else
    {
        result->scheme = uri.substr(0, scheme);
        uri.remove_prefix(scheme + 1);
    }

    if (const auto rest = uri.find_first_of("?#"); rest != std::string_view::npos)
    {
        if (uri[rest] == '#')
        {
            dis_uri.fragment = uri.substr(rest + 1);
        }
        else if (const auto f = uri.find_first_of('#', rest); f == std::string_view::npos)
        {
            dis_uri.query = uri.substr(rest + 1);
        }
        else
        {
            dis_uri.query = uri.substr(rest + 1, f - rest - 1);
            dis_uri.fragment = uri.substr(f + 1);
        }
        uri = uri.substr(0, rest);
    }

    if (!uri.starts_with("//"))
    {
        dis_uri.path = uri;
        validate_or_reset(result);
        return result;
    }
    dis_uri.auth.emplace();
    uri.remove_prefix(2);

    if (const auto pstart = uri.find('/'); pstart != std::string_view::npos)
    {
        dis_uri.path = uri.substr(pstart);
        uri = uri.substr(0, pstart);
    }

    if (const auto userinfo = uri.find('@'); userinfo != std::string_view::npos)
    {
        dis_uri.auth->user_info = uri.substr(0, userinfo);
        uri.remove_prefix(userinfo + 1);
    }

    if (uri.starts_with("["))
    {
        const auto end_rb = uri.find(']');
        if (end_rb == std::string_view::npos)
        {
            result.reset();
            return result;
        }
        dis_uri.auth->host = uri.substr(0, end_rb + 1);
        uri.remove_prefix(end_rb + 1);
        if (!uri.empty())
        {
            if (uri.front() != ':')
            {
                result.reset();
                return result;
            }
            dis_uri.auth->port = uri.substr(1);
        }
    }
    else
    {
        if (const auto port = uri.rfind(':'); port != std::string_view::npos)
        {
            dis_uri.auth->port = uri.substr(port + 1);
            uri = uri.substr(0, port);
        }

        dis_uri.auth->host = uri;
    }

    validate_or_reset(result);
    return result;
}

std::size_t reconstruct_uri_size(const dissected_uri_view& dis_uri, std::string_view extra_path)
{
    std::size_t result = 0;

    result += dis_uri.scheme.size();
    result += 1;

    if (dis_uri.auth)
    {
        result += 2;
        if (dis_uri.auth->user_info)
        {
            result += dis_uri.auth->user_info->size();
            result += 1;
        }
        result += dis_uri.auth->host.size();
        if (dis_uri.auth->port)
        {
            result += 1;
            result += dis_uri.auth->port->size();
        }
    }

    result += dis_uri.path.size();
    result += extra_path.size();

    if (dis_uri.query)
    {
        result += 1;
        result += dis_uri.query->size();
    }

    if (dis_uri.fragment)
    {
        result += 1;
        result += dis_uri.fragment->size();
    }

    return result;
}

std::string reconstruct_uri(const dissected_uri_view& dis_uri, std::string_view extra_path)
{
    std::string uri;
    uri.reserve(reconstruct_uri_size(dis_uri, extra_path));

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
    uri.append(extra_path);

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
std::string decorate_path(const std::optional<dissected_uri_view::authority>& auth, std::string_view path)
{
    std::string hostname;

    if (auth.has_value() && !auth->host.empty())
    {
        hostname = "//";
        hostname += auth->host;

        if (auth->port && !auth->port->empty())
            hostname.append(":").append(*auth->port);

        if (path.starts_with("/"))
            path.remove_prefix(1);
    }

    std::string s = utils::path::lexically_normal(utils::path::join(hostname, path)).string();

    const size_t skip_first = utils::platform::is_windows() && !s.empty() && hostname.empty();

    return utils::encoding::percent_decode(std::string_view(s).substr(skip_first));
}

std::string to_presentable_internal(const dissected_uri_view& dis_uri)
{
    if (dis_uri.scheme == "file" && (utils::platform::is_windows() || !dis_uri.contains_host()))
        return decorate_path(dis_uri.auth, dis_uri.path); // Decorate path with authority

    std::string s;

    if (!dis_uri.scheme.empty())
        s.append(dis_uri.scheme).append(":");

    if (dis_uri.auth.has_value())
    {
        if (!dis_uri.auth->host.empty())
            s.append("//");
        s.append(dis_uri.auth->host);

        if (dis_uri.auth->port.has_value())
            s.append(":").append(*dis_uri.auth->port);
    }

    s.append(dis_uri.path);

    // TODO: Think about presenting query and fragment parts

    return s;
}

std::string to_presentable_internal_debug(const dissected_uri_view& dis_uri, std::string_view raw_uri)
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
    auto dis_uri = dissect_uri(uri);

    if (!dis_uri)
        dis_uri.emplace(); // TODO: this looks odd

    if (debug)
        return to_presentable_internal_debug(*dis_uri, uri);
    else
        return to_presentable_internal(*dis_uri);
}

} // namespace hlasm_plugin::utils::path
