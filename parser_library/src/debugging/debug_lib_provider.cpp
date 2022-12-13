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

#include "debug_lib_provider.h"

#include "analyzer.h"
#include "workspaces/file_manager.h"
#include "workspaces/library.h"
#include "workspaces/workspace.h"

namespace hlasm_plugin::parser_library::debugging {

debug_lib_provider::debug_lib_provider(std::vector<std::shared_ptr<workspaces::library>> libraries,
    workspaces::file_manager& fm,
    std::atomic<bool>* cancel)
    : m_libraries(std::move(libraries))
    , m_file_manager(fm)
    , m_cancel(cancel)
{}

workspaces::parse_result debug_lib_provider::parse_library(
    const std::string& library, analyzing_context ctx, workspaces::library_data data)
{
    utils::resource::resource_location url;
    for (const auto& lib : m_libraries)
    {
        if (!lib->has_file(library, &url))
            continue;

        auto content_o = m_file_manager.get_file_content(url);
        if (!content_o.has_value())
            return false;

        const auto& [location, content] = *m_files.try_emplace(std::move(url), std::move(content_o).value()).first;
        analyzer a(content,
            analyzer_options {
                location,
                this,
                std::move(ctx),
                data,
                collect_highlighting_info::no,
            });
        a.analyze(m_cancel);
        return m_cancel == nullptr || !*m_cancel;
    }

    return false;
}

bool debug_lib_provider::has_library(const std::string& library, const utils::resource::resource_location&) const
{
    for (const auto& lib : m_libraries)
        if (lib->has_file(library))
            return true;
    return false;
}

std::optional<std::pair<std::string, utils::resource::resource_location>> debug_lib_provider::get_library(
    const std::string& library, const utils::resource::resource_location&) const
{
    utils::resource::resource_location url;
    for (const auto& lib : m_libraries)
    {
        if (!lib->has_file(library, &url))
            continue;

        auto content_o = m_file_manager.get_file_content(url);
        if (!content_o.has_value())
            break;

        return std::pair(std::move(content_o).value(), std::move(url));
    }
    return {};
}

} // namespace hlasm_plugin::parser_library::debugging
