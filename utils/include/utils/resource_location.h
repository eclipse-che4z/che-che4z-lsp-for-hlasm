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
#include <optional>
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

    static resource_location join(const resource_location& rl, std::string_view relative_path);

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