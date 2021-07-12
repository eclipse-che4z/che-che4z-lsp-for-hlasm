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

#include "analyzer.h"



namespace hlasm_plugin::parser_library {

class mock_parse_lib_provider : public workspaces::parse_lib_provider
{
    std::unordered_map<std::string, std::string> m_files;

public:
    std::unordered_map<std::string, std::unique_ptr<analyzer>> analyzers;
    mock_parse_lib_provider(std::initializer_list<std::pair<std::string, std::string>> entries)
    {
        for (const auto& e : entries)
            m_files.insert(e);
    }

    workspaces::parse_result parse_library(
        const std::string& library, analyzing_context ctx, workspaces::library_data data) override
    {
        auto it = m_files.find(library);
        if (it == m_files.end())
            return false;

        auto a = std::make_unique<analyzer>(it->second, analyzer_options { library, this, std::move(ctx), data });
        a->analyze();
        a->collect_diags();
        analyzers[library] = std::move(a);
        return true;
    }

    bool has_library(const std::string& library, const std::string&) const override { return m_files.count(library); }


    std::optional<std::string> get_library(const std::string& library, const std::string&, std::string*) const override
    {
        auto it = m_files.find(library);
        if (it == m_files.end())
            return std::nullopt;

        return it->second;
    }
};

} // namespace hlasm_plugin::parser_library