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

#ifndef HLASMPLUGIN_UTILS_PATH_CONVERSIONS_H
#define HLASMPLUGIN_UTILS_PATH_CONVERSIONS_H

#include <optional>
#include <string>

namespace hlasm_plugin::utils::path {

struct dissected_uri
{
    struct authority
    {
        std::optional<std::string> user_info;
        std::string host;
        std::optional<std::string> port;
    };

    std::string scheme;
    std::optional<authority> auth;
    std::string path;
    std::optional<std::string> query;
    std::optional<std::string> fragment;

    bool contains_host() const { return auth.has_value() && !auth->host.empty(); }
};

// Converts URI (RFC3986) to common filesystem path.
std::string uri_to_path(std::string_view uri);

// Converts from filesystem path to URI
std::string path_to_uri(std::string_view path);

// Checks if provided path has the URI format
bool is_uri(std::string_view path) noexcept;

// Returns URI in a presentable format for the user
std::string get_presentable_uri(std::string_view uri, bool debug);

// Do URI dissection
dissected_uri dissect_uri(std::string_view uri);

// Reconstruct dissected URI
std::string reconstruct_uri(const dissected_uri& dis_uri);

} // namespace hlasm_plugin::utils::path

#endif
