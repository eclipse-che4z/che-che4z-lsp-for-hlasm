/*
 * Copyright (c) 2024 Broadcom.
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

#include "gmock/gmock.h"

#include "lsp/folding.h"

using namespace hlasm_plugin::parser_library::lsp;
using namespace ::testing;

TEST(lsp_folding, identify_lines)
{
    const auto le = generate_indentation_map(R"(*
.* comment
    SAM31
    SAM31
L   SAM31
        SAM31
        SAM31
    SAM31
*--------- SEPARATOR ----------------------------------------
    SAM31                                                              X
    
    )");

    const line_entry expected[] {
        { 0, 1, 1, -1, true, false, true, false, false, false },
        { 1, 2, 2, -1, true, false, false, false, false, false },
        { 2, 3, 0, 4, false, false, false, false, false, false },
        { 3, 4, 0, 4, false, false, false, false, false, false },
        { 4, 5, 0, 4, false, false, false, false, true, false },
        { 5, 6, 0, 8, false, false, false, false, false, false },
        { 6, 7, 0, 8, false, false, false, false, false, false },
        { 7, 8, 0, 4, false, false, false, false, false, false },
        { 8, 9, 1, -1, true, false, false, true, false, false },
        { 9, 11, 0, 4, false, false, false, false, false, false },
        { 11, 12, 0, -1, false, true, false, false, false, false },
    };

    EXPECT_THAT(le, ElementsAreArray(expected));
}

TEST(lsp_folding, exception_indent)
{
    std::vector<line_entry> entries;

    for (size_t i = 0; i < 100; ++i)
        entries.push_back({ i, i + 1, 0, 4, false, false, false, false, false, false });

    entries[50].indent = 1;

    mark_suspicious(entries);

    for (size_t i = 0; i < entries.size(); ++i)
    {
        SCOPED_TRACE(i);
        EXPECT_EQ(entries[i].suspicious, i == 50);
    }
}

TEST(lsp_folding, indentation)
{
    const line_entry input[] {
        { 0, 1, 0, 4, false, false, false, false, false, false },
        { 1, 2, 0, 4, false, false, false, false, false, false },
        { 2, 3, 0, 4, false, false, false, false, false, false },
        { 3, 4, 0, 8, false, false, false, false, false, false },
        { 4, 5, 0, 8, false, false, false, false, false, false },
        { 5, 6, 0, 4, false, false, false, false, false, false },
        { 6, 7, 0, 8, false, false, false, false, false, false },
        { 7, 8, 0, 4, false, false, false, false, false, false },
    };

    std::vector<fold_data> result(std::size(input));

    folding_by_indentation(result, input);

    const fold_data expected[] = {
        { 0, 0, 0, false },
        { 0, 0, 0, false },
        { 4, 0, 0, false },
        { 0, 0, 0, false },
        { 0, 0, 0, false },
        { 0, 0, 0, true },
        { 0, 0, 0, true },
        { 0, 0, 0, false },
    };

    EXPECT_THAT(result, ElementsAreArray(expected));
}

TEST(lsp_folding, comments)
{
    const line_entry input[] {
        { 0, 1, 0, 4, false, false, false, false, false, false },
        { 1, 2, 0, 4, false, false, false, false, false, false },
        { 2, 3, 1, 0, true, false, false, false, false, false },
        { 3, 4, 1, 0, true, false, false, false, false, false },
        { 4, 5, 1, 0, true, false, false, false, false, false },
        { 5, 6, 0, 4, false, false, false, false, false, false },
        { 6, 7, 0, 8, false, false, false, false, false, false },
        { 7, 8, 0, 4, false, false, false, false, false, false },
    };

    std::vector<fold_data> result(std::size(input));

    folding_by_comments(result, input);

    const fold_data expected[] = {
        { 0, 0, 0, false },
        { 0, 0, 0, false },
        { 0, 4, 0, false },
        { 0, 0, 0, false },
        { 0, 0, 0, false },
        { 0, 0, 0, false },
        { 0, 0, 0, false },
        { 0, 0, 0, false },
    };

    EXPECT_THAT(result, ElementsAreArray(expected));
}

TEST(lsp_folding, not_comments)
{
    const line_entry input[] {
        { 0, 1, 0, 4, false, false, false, false, false, false },
        { 1, 2, 0, 4, false, false, false, false, false, false },
        { 2, 3, 1, 0, true, false, false, false, false, false },
        { 3, 4, 1, 0, true, false, false, false, false, false },
        { 4, 5, 1, 0, true, false, false, false, false, false },
        { 5, 6, 0, 4, false, false, false, false, false, false },
        { 6, 7, 0, 8, false, false, false, false, false, false },
        { 7, 8, 0, 4, false, false, false, false, false, false },
    };

    std::vector<fold_data> result(std::size(input));

    folding_between_comments(result, input);

    const fold_data expected[] = {
        { 0, 0, 1, false },
        { 0, 0, 0, false },
        { 0, 0, 0, false },
        { 0, 0, 0, false },
        { 0, 0, 0, false },
        { 0, 0, 7, false },
        { 0, 0, 0, false },
        { 0, 0, 0, false },
    };

    EXPECT_THAT(result, ElementsAreArray(expected));
}

TEST(lsp_folding, mixed_mode)
{
    std::vector<fold_data> data {
        { 0, 0, 9, false },
        { 0, 0, 0, false },
        { 4, 0, 0, false },
        { 0, 0, 0, false },
        { 0, 0, 0, false },
        { 0, 0, 0, true },
        { 0, 0, 0, true },
        { 0, 0, 0, false },
        { 0, 0, 0, false },
        { 0, 0, 0, false },
    };

    adjust_folding_data(data);

    const fold_data expected[] {
        { 0, 0, 1, false },
        { 0, 0, 0, false },
        { 4, 0, 0, false },
        { 0, 0, 0, false },
        { 0, 0, 0, false },
        { 0, 0, 0, true },
        { 0, 0, 0, true },
        { 0, 0, 9, false },
        { 0, 0, 0, false },
        { 0, 0, 0, false },
    };

    EXPECT_THAT(data, ElementsAreArray(expected));
}

TEST(lsp_folding, generate_ranges)
{
    const fold_data input[] {
        { 0, 0, 1, false },
        { 0, 0, 0, false },
        { 4, 0, 0, false },
        { 0, 0, 0, false },
        { 0, 0, 0, false },
        { 0, 0, 0, true },
        { 0, 0, 0, true },
        { 0, 0, 9, false },
        { 0, 0, 0, false },
        { 0, 0, 0, false },
        { 0, 11, 0, false },
        { 0, 0, 0, false },
    };

    auto result = generate_folding_ranges(input);

    using hlasm_plugin::parser_library::fold_type;
    using hlasm_plugin::parser_library::folding_range;
    const folding_range expected[] = {
        { 0, 1, fold_type::none },
        { 2, 4, fold_type::none },
        { 7, 9, fold_type::none },
        { 10, 11, fold_type::comment },
    };

    EXPECT_THAT(result, ElementsAreArray(expected));
}
