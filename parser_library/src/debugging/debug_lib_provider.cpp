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

#include <cassert>

#include "analyzer.h"
#include "utils/task.h"
#include "workspaces/file_manager.h"
#include "workspaces/library.h"
#include "workspaces/workspace.h"

namespace hlasm_plugin::parser_library::debugging {

debug_lib_provider::debug_lib_provider(
    std::vector<std::shared_ptr<workspaces::library>> libraries, workspaces::file_manager& fm)
    : m_libraries(std::move(libraries))
    , m_file_manager(fm)
{}

utils::value_task<bool> debug_lib_provider::parse_library(
    std::string library, analyzing_context ctx, processing::processing_kind kind)
{
    utils::resource::resource_location url;
    for (const auto& lib : m_libraries)
    {
        if (!lib->has_file(library, &url))
            continue;

        auto content_o = co_await m_file_manager.get_file_content(url);
        if (!content_o.has_value())
            break;

        const auto& [location, content] = *m_files.try_emplace(std::move(url), std::move(content_o).value()).first;

        analyzer a(content,
            analyzer_options {
                location,
                this,
                std::move(ctx),
                analyzer_options::dependency(std::move(library), kind),
                collect_highlighting_info::no,
            });

        co_await a.co_analyze();

        co_return true;
    }
    co_return false;
}

bool debug_lib_provider::has_library(std::string_view library, utils::resource::resource_location* loc)
{
    for (const auto& lib : m_libraries)
        if (lib->has_file(library, loc))
            return true;
    return false;
}

utils::value_task<std::optional<std::pair<std::string, utils::resource::resource_location>>>
debug_lib_provider::get_library(std::string library)
{
    utils::resource::resource_location url;
    for (const auto& lib : m_libraries)
    {
        if (!lib->has_file(library, &url))
            continue;

        auto content_o = co_await m_file_manager.get_file_content(url);
        if (!content_o.has_value())
            break;

        co_return std::pair(std::move(content_o).value(), std::move(url));
    }
    co_return std::nullopt;
}

utils::task debug_lib_provider::prefetch_libraries() const
{
    std::vector<utils::task> pending_prefetches;
    for (const auto& lib : m_libraries)
        if (auto p = lib->prefetch(); p.valid() && !p.done())
            pending_prefetches.emplace_back(std::move(p));

    if (pending_prefetches.empty())
        return {};

    return utils::task::wait_all(std::move(pending_prefetches));
}

} // namespace hlasm_plugin::parser_library::debugging
