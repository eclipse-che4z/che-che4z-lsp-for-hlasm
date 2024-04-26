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

#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>

#include "analyzer.h"
#include "parse_lib_provider.h"
#include "utils/general_hashers.h"

namespace hlasm_plugin::parser_library {

struct mock_file_stats_t
{
    size_t parse_requests = 0;
    size_t existence_requests = 0;
    size_t content_requests = 0;
};

struct mock_file_t
{
    mock_file_t(std::string c)
        : content(std::move(c))
    {}

    std::string content;

    mutable mock_file_stats_t stats;
};

class mock_parse_lib_provider : public parse_lib_provider
{
    std::unordered_map<std::string, mock_file_t, utils::hashers::string_hasher, std::equal_to<>> m_files;

public:
    std::unordered_map<std::string, std::unique_ptr<analyzer>, utils::hashers::string_hasher, std::equal_to<>>
        analyzers;

    mock_parse_lib_provider() = default;
    mock_parse_lib_provider(std::initializer_list<std::pair<std::string, std::string>> entries)
        : m_files(entries.begin(), entries.end())
    {}
    template<typename T>
    mock_parse_lib_provider(T&& c)
        : m_files(c.begin(), c.end())
    {}

    utils::value_task<bool> parse_library(
        std::string library, analyzing_context ctx, processing::processing_kind kind) override
    {
        auto it = m_files.find(library);
        if (it == m_files.end())
        {
            co_return false;
        }

        ++it->second.stats.parse_requests;
        auto a = std::make_unique<analyzer>(it->second.content,
            analyzer_options {
                hlasm_plugin::utils::resource::resource_location(library),
                this,
                std::move(ctx),
                analyzer_options::dependency(library, kind),
            });
        co_await a->co_analyze();

        analyzers.insert_or_assign(std::move(library), std::move(a));

        co_return true;
    }

    bool has_library(std::string_view library, utils::resource::resource_location* loc) override
    {
        auto it = m_files.find(library);
        if (it == m_files.end())
            return false;

        ++it->second.stats.existence_requests;
        if (loc)
            *loc = utils::resource::resource_location(library);
        return true;
    }


    utils::value_task<std::optional<std::pair<std::string, utils::resource::resource_location>>> get_library(
        std::string library) override
    {
        auto it = m_files.find(library);
        if (it == m_files.end())
        {
            co_return std::nullopt;
        }

        ++it->second.stats.content_requests;
        co_return std::pair<std::string, utils::resource::resource_location>(
            it->second.content, utils::resource::resource_location(std::move(library)));
    }

    std::optional<mock_file_stats_t> get_stats(std::string_view library) const
    {
        auto it = m_files.find(library);
        if (it == m_files.end())
            return std::nullopt;
        else
            return it->second.stats;
    }
};

} // namespace hlasm_plugin::parser_library
