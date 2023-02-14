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

namespace hlasm_plugin::parser_library {

class mock_parse_lib_provider : public workspaces::parse_lib_provider
{
    std::unordered_map<std::string, std::string, utils::hashers::string_hasher, std::equal_to<>> m_files;

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

    void parse_library(std::string_view library,
        analyzing_context ctx,
        workspaces::library_data data,
        std::function<void(bool)> callback) override
    {
        auto it = m_files.find(library);
        if (it == m_files.end())
        {
            callback(false);
            return;
        }

        auto a = std::make_unique<analyzer>(it->second,
            analyzer_options { hlasm_plugin::utils::resource::resource_location(library), this, std::move(ctx), data });
        a->analyze();
        a->collect_diags();
        analyzers.insert_or_assign(std::string(library), std::move(a));

        callback(true);
    }

    bool has_library(std::string_view library, utils::resource::resource_location* loc) const override
    {
        if (!m_files.count(library))
            return false;
        if (loc)
            *loc = utils::resource::resource_location(library);
        return true;
    }


    std::optional<std::pair<std::string, utils::resource::resource_location>> get_library(
        std::string_view library) const override
    {
        auto it = m_files.find(library);
        if (it == m_files.end())
            return std::nullopt;

        return std::pair<std::string, utils::resource::resource_location>(
            it->second, utils::resource::resource_location(library));
    }
};

} // namespace hlasm_plugin::parser_library
