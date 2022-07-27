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

#include "utils/resource_location.h"

#include <algorithm>
#include <assert.h>
#include <cstddef>
#include <deque>
#include <iterator>
#include <regex>

#include "utils/encoding.h"
#include "utils/path_conversions.h"
#include "utils/platform.h"

namespace hlasm_plugin::utils::resource {

namespace {
const std::string windows_drive_letter = "[A-Za-z]";
const std::string colon = "(?::|%3[aA])";
const std::string slash = "(?:/|\\\\)";

const std::regex windows_drive("^(" + windows_drive_letter + colon + ")");
const std::regex host_like_windows_path("^(" + windows_drive_letter + ")(" + colon + "?)$");
const std::regex path_like_windows_path("^(?:[///]|[//]|[/])?(" + windows_drive_letter + ")" + colon);
const std::regex local_windows("^file:" + slash + "*" + windows_drive_letter + colon);
const std::regex local_non_windows("^file:(?:" + slash + "{3}|(?:" + slash + "(?:[^/\\\\])+))");
} // namespace

resource_location::resource_location(std::string uri)
    : m_uri(std::move(uri))
{}

resource_location::resource_location(std::string_view uri)
    : resource_location(std::string(uri))
{}

resource_location::resource_location(const char* uri)
    : resource_location(std::string(uri))
{}

const std::string& resource_location::get_uri() const { return m_uri; }

std::string resource_location::get_path() const { return m_uri.size() != 0 ? utils::path::uri_to_path(m_uri) : m_uri; }

std::string resource_location::to_presentable(bool debug) const
{
    return utils::path::get_presentable_uri(m_uri, debug);
}

bool resource_location::is_local() const { return is_local(m_uri); }

bool resource_location::is_local(std::string_view uri)
{
    if (utils::platform::is_windows())
    {
        if (std::regex_search(uri.begin(), uri.end(), local_windows))
            return true;
    }
    else if (std::regex_search(uri.begin(), uri.end(), local_non_windows))
        return true;

    return false;
}

namespace {
void uri_append(std::string& uri, std::string_view r)
{
    if (!uri.empty())
    {
        if (uri.back() == '\\')
            uri.back() = '/';
        else if (uri.back() != '/')
            uri.append("/");

        if (r.starts_with("/"))
            r.remove_prefix(1);
    }

    uri.append(r);
}

struct uri_path_iterator
{
    using iterator_category = std::input_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = std::string_view;
    using pointer = std::string_view*;
    using reference = std::string_view;

    explicit uri_path_iterator(pointer uri_path)
        : m_uri_path(uri_path)
        , m_started(m_uri_path != nullptr ? m_uri_path->empty() : false)
    {}

    reference operator*() const { return m_element; }
    pointer operator->() { return &m_element; }

    uri_path_iterator& operator++()
    {
        next_element();
        return *this;
    }

    uri_path_iterator operator++(int)
    {
        uri_path_iterator tmp = *this;
        ++(*this);
        return tmp;
    }

    friend bool operator==(const uri_path_iterator& l, const uri_path_iterator& r)
    {
        return l.m_uri_path == r.m_uri_path;
    }

    uri_path_iterator begin()
    {
        next_element();
        return *this;
    }

    uri_path_iterator end() { return uri_path_iterator(nullptr); }

private:
    pointer m_uri_path;
    value_type m_element;
    bool m_started;

    void next_element()
    {
        assert(m_uri_path != nullptr);

        if (!m_started && !m_uri_path->empty() && (m_uri_path->front() == '/' || m_uri_path->front() == '\\'))
        {
            // First char of the path is a slash - promote it to element
            m_element = m_uri_path->substr(0, 1);
            m_uri_path->remove_prefix(1);
            m_started = true;
            return;
        }
        m_started = true;

        if (const auto not_slash = m_uri_path->find_first_not_of("/\\"); not_slash != std::string_view::npos)
        {
            // Store the current element name without any potential ending slash
            if (const auto next_slash = m_uri_path->find_first_of("/\\", not_slash);
                next_slash == std::string_view::npos)
            {
                m_element = m_uri_path->substr(not_slash);
                m_uri_path->remove_prefix(m_uri_path->size());
            }
            else
            {
                m_element = m_uri_path->substr(not_slash, next_slash - not_slash);
                m_uri_path->remove_prefix(next_slash);
            }
        }
        else if (*m_uri_path != "" && m_element != "")
        {
            // If we got here it must mean that the path now consists only of slashes (e.g. '/')
            // The last element is therefore empty but valid
            m_element = "";
        }
        else
            m_uri_path = nullptr;
    };
};

std::string normalize_path(std::string_view orig_path, bool has_file_scheme, bool has_host)
{
    std::deque<std::string_view> elements;
    std::string_view path = orig_path;
    std::string_view win_root_dir;

    for (uri_path_iterator this_it(&path); auto element : this_it)
    {
        if (element == ".")
            continue;
        else if (element == "..")
        {
            if (!elements.empty() && (elements.size() > 1 || (elements.front() != "/" && elements.front() != "\\")))
                elements.pop_back();
        }
        else if (elements.empty() && element.empty())
            elements.push_back("/");
        else if (has_file_scheme && !has_host && utils::platform::is_windows()
            && std::regex_match(element.begin(), element.end(), windows_drive))
            win_root_dir = element;
        else
            elements.push_back(element);
    }

    std::string ret = "";

    if (!win_root_dir.empty())
        ret.append(win_root_dir);

    // Append elements to uri
    while (!elements.empty())
    {
        uri_append(ret, elements.front());
        elements.pop_front();
    }

    // Add a missing '/' if the last part of the original uri is "/." , "/..", "." or ".."
    if (orig_path.ends_with("/.") || orig_path.ends_with("/..") || orig_path.ends_with(".")
        || orig_path.ends_with(".."))
        uri_append(ret, "");

    return ret;
}

void normalize_windows_like_uri_helper(
    utils::path::dissected_uri& dis_uri, unsigned char win_drive_letter, std::string_view path_suffix)
{
    std::string path;
    path.push_back('/');
    path.push_back(static_cast<const char>(tolower(win_drive_letter)));
    path.push_back(':');
    path.append(path_suffix);

    dis_uri.path = std::move(path);

    // Clear host but keep in mind that it needs to remain valid (albeit empty)
    dis_uri.auth = { std::nullopt, "", std::nullopt };
}

void normalize_windows_like_uri(utils::path::dissected_uri& dis_uri)
{
    if (dis_uri.scheme == "file")
    {
        if (!utils::platform::is_windows())
            return;

        if (std::smatch s; dis_uri.auth.has_value() && (!dis_uri.auth->port.has_value() || dis_uri.auth->port->empty())
            && !dis_uri.auth->user_info.has_value() && dis_uri.contains_host())
        {
            if (!std::regex_search(dis_uri.auth->host, s, host_like_windows_path)
                || (s[2].length() == 0 && !dis_uri.auth->port.has_value()))
                return;

            // auth consists only of host name resembling Windows drive and empty or missing port part
            normalize_windows_like_uri_helper(dis_uri, s[1].str()[0], dis_uri.path);
        }
        else if (!dis_uri.contains_host() && std::regex_search(dis_uri.path, s, path_like_windows_path))
            // Seems like we have a windows like path
            normalize_windows_like_uri_helper(dis_uri, s[1].str()[0], s.suffix().str());
    }
}
} // namespace

resource_location resource_location::lexically_normal() const
{
    auto uri = m_uri;

    std::replace(uri.begin(), uri.end(), '\\', '/');

    auto dis_uri = utils::path::dissect_uri(uri);
    if (dis_uri.path.empty())
        return resource_location(m_uri);

    std::transform(dis_uri.scheme.begin(), dis_uri.scheme.end(), dis_uri.scheme.begin(), [](unsigned char c) {
        return static_cast<const char>(tolower(c));
    });

    dis_uri.path = normalize_path(dis_uri.path, dis_uri.scheme == "file", dis_uri.contains_host());
    normalize_windows_like_uri(dis_uri);

    dis_uri.path = utils::encoding::percent_encode_and_ignore_utf8(dis_uri.path);

    return resource_location(utils::path::reconstruct_uri(dis_uri));
}

resource_location resource_location::lexically_relative(const resource_location& base) const
{
    std::string_view this_uri = m_uri;
    std::string_view base_uri = base.get_uri();

    // Compare schemes
    if (auto this_colon = this_uri.find_first_of(":") + 1; this_colon != std::string_view::npos)
    {
        if (0 != this_uri.compare(0, this_colon, base_uri, 0, this_colon))
            return resource_location();

        this_uri.remove_prefix(this_colon);
        base_uri.remove_prefix(this_colon);
    }

    // Now create iterators and mismatch them
    uri_path_iterator l_it(&this_uri);
    uri_path_iterator r_it(&base_uri);

    auto [this_it, base_it] = std::mismatch(l_it.begin(), l_it.end(), r_it.begin(), r_it.end());
    if (this_it == this_it.end() && base_it == base_it.end())
        return resource_location(".");

    // Figure out how many dirs to return
    int16_t number_of_dirs_to_return = 0;
    while (base_it != base_it.end())
    {
        if (auto element = *base_it; element == "..")
            number_of_dirs_to_return--;
        else if (!element.empty() && element != ".")
            number_of_dirs_to_return++;

        base_it++;
    }

    if (number_of_dirs_to_return < 0)
        return resource_location();

    if (number_of_dirs_to_return == 0 && (this_it == this_it.end() || *this_it == ""))
        return resource_location(".");

    // Append number of dirs to return
    std::string ret;
    while (number_of_dirs_to_return--)
    {
        uri_append(ret, "..");
    }

    // Append the rest of the remaining path
    while (this_it != this_it.end())
    {
        auto element = *this_it;
        uri_append(ret, element);
        this_it++;
    }

    return resource_location(ret);
}

bool resource_location::lexically_out_of_scope() const
{
    return m_uri == std::string_view("..") || m_uri.starts_with("../") || m_uri.starts_with("..\\");
}

void resource_location::join(std::string_view other)
{
    if (utils::path::is_uri(other))
        m_uri = other;
    else if (other.starts_with("/"))
    {
        auto dis_uri = utils::path::dissect_uri(m_uri);
        dis_uri.path = other;
        m_uri = utils::path::reconstruct_uri(dis_uri);
    }
    else
        uri_append(m_uri, other);
}

resource_location resource_location::join(resource_location rl, std::string_view other)
{
    rl.join(other);

    return rl;
}

namespace {
utils::path::dissected_uri::authority relative_reference_process_new_auth(
    const std::optional<utils::path::dissected_uri::authority>& old_auth, std::string_view host)
{
    utils::path::dissected_uri::authority new_auth;
    new_auth.host = host;

    if (!old_auth)
    {
        new_auth.user_info = old_auth->user_info;
        new_auth.port = old_auth->port;
    }

    return new_auth;
}

// Merge based on RFC 3986
void merge(std::string& uri, std::string_view r)
{
    if (auto f = uri.find_last_of("/:"); f != std::string::npos)
    {
        uri = uri.substr(0, f + 1);
        uri.append(r);
    }
    else
        uri = r;
}

// Algorithm from RFC 3986
std::string remove_dot_segments(std::string_view path)
{
    std::string ret;
    std::deque<std::string_view> elements;

    while (!path.empty())
    {
        if (path.starts_with("../"))
            path = path.substr(3);
        else if (path.starts_with("./"))
            path = path.substr(2);
        else if (path.starts_with("/./"))
            path = path.substr(2);
        else if (path == "." || path == ".." || path == "/." || path == "/..")
        {
            if (!elements.empty() && path == "/..")
                elements.pop_back();
            break;
        }
        else if (path.starts_with("/../"))
        {
            path = path.substr(3);

            if (!elements.empty())
                elements.pop_back();
        }
        else
        {
            auto first_slash = path.find_first_of("/", 1);
            elements.push_back(path.substr(0, first_slash));

            if (first_slash != std::string::npos)
                path = path.substr(first_slash);
            else
                path.remove_prefix(path.size());
        }
    }

    for (const auto& element : elements)
    {
        ret.append(element);
    }

    // Add a missing '/' if the last part of the original uri is "/." or "/.."
    if (path == "/." || path == "/..")
        uri_append(ret, "/");

    return ret;
}
} // namespace

void resource_location::relative_reference_resolution(
    std::string_view other) // TODO enhancements can be made based on rfc3986 if needed
{
    if (other.empty())
        return;

    if (utils::path::is_uri(other))
    {
        auto dis_uri = utils::path::dissect_uri(other);
        dis_uri.path = remove_dot_segments(dis_uri.path);
        m_uri = utils::path::reconstruct_uri(dis_uri);
    }
    else
    {
        auto dis_uri = utils::path::dissect_uri(m_uri);

        if (dis_uri.scheme.empty() && dis_uri.path.empty())
            uri_append(m_uri, other); // This is a regular path and not a uri. Let's proceed in a regular way
        else if (other.front() == '?')
            dis_uri.query = other.substr(1);
        else if (other.front() == '#')
            dis_uri.fragment = other.substr(1);
        else if (other.starts_with("//"))
        {
            dis_uri.auth = relative_reference_process_new_auth(dis_uri.auth, std::string_view(other).substr(2));
            dis_uri.path.clear();
            dis_uri.query = std::nullopt;
            dis_uri.fragment = std::nullopt;
        }
        else
        {
            if (other.starts_with("/"))
                dis_uri.path = other;
            else
                merge(dis_uri.path, other);

            dis_uri.path = remove_dot_segments(dis_uri.path);

            dis_uri.query = std::nullopt;
            dis_uri.fragment = std::nullopt;
        }

        m_uri = utils::path::reconstruct_uri(dis_uri);
    }
}

resource_location resource_location::relative_reference_resolution(resource_location rl, std::string_view other)
{
    rl.relative_reference_resolution(other);

    return rl;
}

std::size_t resource_location_hasher::operator()(const resource_location& rl) const
{
    return std::hash<std::string> {}(rl.get_uri());
}
} // namespace hlasm_plugin::utils::resource