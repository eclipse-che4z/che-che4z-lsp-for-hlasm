/*
 * Copyright (c) 2019 Broadcom.
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

#ifndef HLASMPLUGIN_PARSERLIBRARY_DEBUG_LIB_PROVIDER_H
#define HLASMPLUGIN_PARSERLIBRARY_DEBUG_LIB_PROVIDER_H

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "utils/resource_location.h"
#include "workspaces/parse_lib_provider.h"

namespace hlasm_plugin::utils {
class task;
} // namespace hlasm_plugin::utils

namespace hlasm_plugin::parser_library {
class analyzer;
} // namespace hlasm_plugin::parser_library

namespace hlasm_plugin::parser_library::workspaces {
class file_manager;
class library;
} // namespace hlasm_plugin::parser_library::workspaces

namespace hlasm_plugin::parser_library::debugging {

// Implements dependency (macro and COPY files) fetcher for macro tracer.
// Takes the information from a workspace, but calls special methods for
// parsing that do not collide with LSP.
class debug_lib_provider final : public workspaces::parse_lib_provider
{
    std::unordered_map<utils::resource::resource_location, std::string, utils::resource::resource_location_hasher>
        m_files;
    std::vector<std::shared_ptr<workspaces::library>> m_libraries;
    workspaces::file_manager& m_file_manager;
    std::vector<utils::task>& m_analyzers;

public:
    debug_lib_provider(std::vector<std::shared_ptr<workspaces::library>> libraries,
        workspaces::file_manager& fm,
        std::vector<utils::task>& analyzers);

    void parse_library(std::string_view library,
        analyzing_context ctx,
        workspaces::library_data data,
        std::function<void(bool)> callback) override;

    bool has_library(std::string_view library, utils::resource::resource_location* loc) const override;

    void get_library(std::string_view library,
        std::function<void(std::optional<std::pair<std::string, utils::resource::resource_location>>)> callback)
        const override;
};

} // namespace hlasm_plugin::parser_library::debugging

#endif
