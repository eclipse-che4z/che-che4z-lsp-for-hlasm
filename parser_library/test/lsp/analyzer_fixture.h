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
#include "empty_parse_lib_provider.h"

namespace hlasm_plugin::parser_library {

struct analyzer_fixture : public ::testing::Test
{
    static const inline auto opencode_loc = utils::resource::resource_location("source");
    analyzer a;
    analyzer_fixture(const std::string& input,
        parse_lib_provider& provider = empty_parse_lib_provider::instance,
        asm_option opts = {})
        : a(input, analyzer_options { opencode_loc, &provider, std::move(opts) })
    {
        a.analyze();
    }
};

} // namespace hlasm_plugin::parser_library

#endif
