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

struct dissected_uri_view
{
    struct authority
    {
        std::optional<std::string_view> user_info;
        std::string_view host;
        std::optional<std::string_view> port;
    };

    std::string_view scheme;
    std::optional<authority> auth;
    std::string_view path;
    std::optional<std::string_view> query;
    std::optional<std::string_view> fragment;

    bool contains_host() const { return auth.has_value() && !auth->host.empty(); }
};

// Converts URI (RFC3986) to common filesystem path.
std::string uri_to_path(std::string_view uri);

// Converts from filesystem path to URI
std::string path_to_uri(std::string_view path);

// Checks if provided path has the URI format
bool is_likely_uri(std::string_view path);

// Returns URI in a presentable format for the user
std::string get_presentable_uri(std::string_view uri, bool debug);

// Do URI dissection
std::optional<dissected_uri_view> dissect_uri(std::string_view uri);

// Reconstruct dissected URI
std::string reconstruct_uri(const dissected_uri_view& dis_uri, std::string_view extra_path = {});

} // namespace hlasm_plugin::utils::path

#endif
