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

#include "gtest/gtest.h"

#include "../mock_parse_lib_provider.h"
#include "analyzer.h"
#include "workspaces/parse_lib_provider.h"


using namespace hlasm_plugin::parser_library;

TEST(highlighting, AIF)
{
    mock_parse_lib_provider lib_provider;
    const std::string contents = " AIF (&VAR EQ 4).JUMP";
    analyzer a(contents, SOURCE_FILE, lib_provider, nullptr, true);
    a.analyze();
    auto tokens = a.lsp_processor().semantic_tokens();
    
    int i = 0;
}
