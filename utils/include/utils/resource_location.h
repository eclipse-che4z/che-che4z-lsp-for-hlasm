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

    // Lexically functions behave very similarly to std::filesystem functions
    // Additionally tries to normalize URIs containing file scheme on Windows (file:C:/dir or file:/C:/dir or
    // file://C:/dir -> or file:///C://dir)
    std::string lexically_normal() const;
    std::string lexically_relative(const resource_location& base) const;
    bool lexically_out_of_scope() const;

    // Join behaves very similarly to std::filesystem functions
    void join(const std::string& other);
    static resource_location join(resource_location rl, const std::string& other);

    // Relative reference resolution based on RFC 3986
    void relative_reference_resolution(const std::string& other);
    static resource_location relative_reference_resolution(resource_location rl, const std::string& other);

    std::strong_ordering operator<=>(const resource_location& rl) const noexcept;
    bool operator==(const resource_location& rl) const noexcept;

private:
    std::string m_uri;
};

struct resource_location_hasher
{
    std::size_t operator()(const resource_location& rl) const;
};

} // namespace hlasm_plugin::utils::resource

#endif