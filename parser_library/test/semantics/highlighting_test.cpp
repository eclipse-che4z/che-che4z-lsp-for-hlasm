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

#include "analyzer.h"
#include "workspaces/parse_lib_provider.h"


using namespace hlasm_plugin::parser_library;

TEST(highlighting, AIF)
{
    std::string source_file = "file_name";
    workspaces::empty_parse_lib_provider lib_provider;
    const std::string contents = "&VARP(31+L'C) SETA 45\n\nC EQU 1";
    analyzer a(contents, source_file, lib_provider, nullptr, true);
    a.analyze();
    auto tokens = a.lsp_processor().semantic_tokens();

    int i = 0;
}
