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


#ifndef HLASMPLUGIN_PARSERLIBRARY_TEST_EXPR_MOCK_H
#define HLASMPLUGIN_PARSERLIBRARY_TEST_EXPR_MOCK_H

#include "workspaces/parse_lib_provider.h"

using namespace hlasm_plugin::parser_library;

class dep_sol_mock : public context::dependency_solver
{
    const context::symbol* get_symbol(context::id_index) const override { return nullptr; };
};

class lib_prov_mock : public workspaces::parse_lib_provider
{
public:
    workspaces::parse_result parse_library(const std::string&, analyzing_context, workspaces::library_data) override
    {
        return false;
    };

    bool has_library(const std::string&, const std::string&) const override { return false; }
    std::optional<std::string> get_library(const std::string&, const std::string&, std::string*) const override
    {
        return std::nullopt;
    }
};

inline std::string big_string(char c = '1')
{
    std::string s;
    s.reserve(4000);
    for (size_t i = 0; i < 4000; i++)
        s.push_back(c);
    return s;
}

#endif
