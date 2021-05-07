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

#ifndef HLASMPLUGIN_PARSERLIBRARY_TEST_LSP_ANALYZER_FIXTURE_H
#define HLASMPLUGIN_PARSERLIBRARY_TEST_LSP_ANALYZER_FIXTURE_H

#include "gtest/gtest.h"

#include "../gtest_stringers.h"
#include "analyzer.h"

namespace hlasm_plugin::parser_library {

struct analyzer_fixture : public ::testing::Test
{
    const static inline std::string opencode_file_name = "source";
    analyzer a;
    analyzer_fixture(const std::string& input,
        workspaces::parse_lib_provider& provider = workspaces::empty_parse_lib_provider::instance)
        : a(input, analyzer_options { opencode_file_name, &provider, provider.get_asm_options(opencode_file_name) })
    {
        a.analyze();
    }
};

} // namespace hlasm_plugin::parser_library

#endif