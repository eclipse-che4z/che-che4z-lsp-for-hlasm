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

#include "processing/attribute_provider.h"
#include "workspaces/parse_lib_provider.h"

using namespace hlasm_plugin::parser_library;

class dep_sol_mock : public context::dependency_solver
{
    virtual const context::symbol* get_symbol(context::id_index) const { return nullptr; };
};

class attr_prov_mock : public processing::attribute_provider
{
    virtual const resolved_reference_storage& lookup_forward_attribute_references(forward_reference_storage) override
    {
        static resolved_reference_storage st;
        return st;
    }
};

class lib_prov_mock : public workspaces::parse_lib_provider
{
    virtual workspaces::parse_result parse_library(
        const std::string&, context::hlasm_context&, const workspaces::library_data)
    {
        return false;
    };

    virtual bool has_library(const std::string&, context::hlasm_context&) const { return false; }
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
