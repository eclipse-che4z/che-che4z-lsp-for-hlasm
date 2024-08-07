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

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "parse_lib_provider.h"
#include "utils/resource_location.h"

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
class debug_lib_provider final : public parse_lib_provider
{
    std::unordered_map<utils::resource::resource_location, std::string> m_files;
    std::vector<std::shared_ptr<workspaces::library>> m_libraries;
    workspaces::file_manager& m_file_manager;

public:
    debug_lib_provider(std::vector<std::shared_ptr<workspaces::library>> libraries, workspaces::file_manager& fm);

    [[nodiscard]] utils::value_task<bool> parse_library(
        std::string library, analyzing_context ctx, processing::processing_kind kind) override;

    bool has_library(std::string_view library, utils::resource::resource_location* loc) override;

    [[nodiscard]] utils::value_task<std::optional<std::pair<std::string, utils::resource::resource_location>>>
    get_library(std::string library) override;

    [[nodiscard]] utils::task prefetch_libraries() const;
};

} // namespace hlasm_plugin::parser_library::debugging

#endif
