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

#include <filesystem>
#include <regex>

#include "network/uri/uri.hpp"

#include "utils/path.h"
#include "utils/path_conversions.h"
#include "utils/platform.h"

namespace hlasm_plugin::utils::resource {

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

bool resource_location::lexically_out_of_scope() const
{
    return m_uri == std::string_view("..") || m_uri.starts_with("../") || m_uri.starts_with("..\\");
}

resource_location resource_location::join(const resource_location& rl, std::string_view relative_path)
{
    std::string uri = rl.get_uri();

    if (!rl.m_uri.empty() && rl.m_uri.back() != '/')
        return resource_location(uri.append("/").append(relative_path));

    return resource_location(uri.append(relative_path));
}

std::size_t resource_location_hasher::operator()(const resource_location& rl) const
{
    return std::hash<std::string> {}(rl.get_uri());
}
} // namespace hlasm_plugin::utils::resource