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

#include <compare>
#include <string>

namespace hlasm_plugin::utils::resource {

class resource_location
{
public:
    explicit resource_location() = default;
    explicit resource_location(std::string uri);
    explicit resource_location(std::string_view uri);
    explicit resource_location(const char* uri);

    const std::string& get_uri() const;
    std::string get_path() const;
    std::string to_presentable(bool debug = false) const;

    bool empty() const { return m_uri.empty(); }

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
    void relative_reference_resolution(std::string_view other);
    static resource_location relative_reference_resolution(resource_location rl, std::string_view other);

    resource_location& replace_filename(std::string_view other);
    static resource_location replace_filename(resource_location rl, std::string_view other);

    std::string filename() const;
    resource_location parent() const;

    std::string get_local_path_or_uri() const;

    std::strong_ordering operator<=>(const resource_location& rl) const noexcept = default;

private:
    std::string m_uri;
};

struct resource_location_hasher
{
    std::size_t operator()(const resource_location& rl) const;
};

} // namespace hlasm_plugin::utils::resource

#endif