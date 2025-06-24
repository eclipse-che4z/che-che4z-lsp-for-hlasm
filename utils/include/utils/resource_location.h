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

#ifndef HLASMPLUGIN_UTILS_RESOURCE_LOCATION_H
#define HLASMPLUGIN_UTILS_RESOURCE_LOCATION_H

#include <atomic>
#include <compare>
#include <memory>
#include <string>
#include <string_view>

namespace hlasm_plugin::utils::resource {

class resource_location
{
public:
    explicit resource_location() = default;
    explicit resource_location(std::string uri);
    explicit resource_location(std::string_view uri);
    explicit resource_location(const char* uri);

    std::string_view get_uri() const { return m_data ? m_data->uri : std::string_view(); }
    std::string get_path() const;
    std::string to_presentable(bool debug = false) const;

    bool empty() const { return !m_data; }

    bool is_local() const;
    static bool is_local(std::string_view uri);

    // Lexically functions behave very similarly to std::filesystem functions
    // Additionally tries to
    // - normalize URIs containing file scheme on Windows (from file:C:/dir or file:/C:/dir or
    // file://C:/dir or file:///C://dir to file:///C:/dir)
    // - percent encode special characters
    resource_location lexically_normal() const;
    resource_location lexically_relative(const resource_location& base) const;
    bool lexically_out_of_scope() const;

    // Join behaves very similarly to std::filesystem functions
    resource_location& join(std::string_view other);
    static resource_location join(resource_location rl, std::string_view other);

    // Relative reference resolution based on RFC 3986
    resource_location& relative_reference_resolution(std::string_view other);
    static resource_location relative_reference_resolution(resource_location rl, std::string_view other);

    resource_location& replace_filename(std::string_view other);
    static resource_location replace_filename(resource_location rl, std::string_view other);

    std::string filename() const;
    resource_location parent() const;

    std::string get_local_path_or_uri() const;

    bool is_prefix_of(const resource_location& candidate) const;
    static bool is_prefix(const resource_location& candidate, const resource_location& base);

    bool operator==(const resource_location& rl) const noexcept
    {
        return m_data == rl.m_data
            || (m_data && rl.m_data && m_data->get_hash() == rl.m_data->get_hash() && m_data->uri == rl.m_data->uri);
    }
    std::strong_ordering operator<=>(const resource_location& rl) const noexcept
    {
        if (m_data == rl.m_data)
            return std::strong_ordering::equal;

        std::string_view l;
        std::string_view r;
        if (m_data)
            l = m_data->uri;
        if (rl.m_data)
            r = rl.m_data->uri;
        return l <=> r;
    }

    size_t hash() const noexcept { return m_data ? m_data->get_hash() : 0; }

private:
    struct data
    {
        data(std::string s);
        std::string uri;
        mutable std::atomic<size_t> hash = 0;

        size_t get_hash() const noexcept
        {
            if (auto h = hash.load(std::memory_order_relaxed))
                return h;
            return update_hash();
        }
        size_t update_hash() const noexcept;
    };
    std::shared_ptr<const data> m_data;
};

} // namespace hlasm_plugin::utils::resource

template<>
class std::hash<::hlasm_plugin::utils::resource::resource_location>
{
public:
    std::size_t operator()(const ::hlasm_plugin::utils::resource::resource_location& rl) const noexcept
    {
        return rl.hash();
    }
};

#endif
