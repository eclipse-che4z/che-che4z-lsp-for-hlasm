/*
 * Copyright (c) 2023 Broadcom.
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

#include "../common_testing.h"
#include "branch_info.h"
#include "lsp/lsp_context.h"

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::lsp;
using namespace ::testing;

TEST(lsp_context, branch_info)
{
    analyzer a(R"(
         USING *,12
         J     X
         NOP   X
         BRC   0,X
         BRC   15,X
         BE    X
         JE    X
         BASR  0,1
         BASR  1,0
         BASR  1,ZERO
         BASR  1,ONE
         BCT   0,X
         JCT   0,X
         BCTR  2,1
         BCTR  2,0
X        DS    0H
         SVC   0
         BRASL 0,X
         PC    0
         PR
         CIJ   0,0,0,X
         CIJ   0,0,8,X
         CIJE  0,0,X
         LPSW  0

         MACRO
         MAC
         J     X
         MEND
         MACRO
         MAC2
         J     Y
Y        DS    0H
         MEND

         MAC
         MAC2

         MACRO
         MAC3
         J     X
         J     Z
         MEND
          MAC3
Z        DS    0H

ZERO     EQU   0
ONE      EQU   1
)");
    a.analyze();

    EXPECT_TRUE(a.diags().empty());

    auto bi = a.context().lsp_ctx->get_opencode_branch_info();

    const std::vector<branch_info> expect_bi {
        { 2, 9, branch_direction::down },
        { 5, 9, branch_direction::down },
        { 6, 9, branch_direction::down },
        { 7, 9, branch_direction::down },
        { 8, 9, branch_direction::somewhere },
        { 11, 9, branch_direction::somewhere },
        { 12, 9, branch_direction::down },
        { 13, 9, branch_direction::down },
        { 14, 9, branch_direction::somewhere },
        { 17, 9, branch_direction::somewhere },
        { 18, 9, branch_direction::up },
        { 19, 9, branch_direction::somewhere },
        { 20, 9, branch_direction::somewhere },
        { 22, 9, branch_direction::up },
        { 23, 9, branch_direction::up },
        { 24, 9, branch_direction::somewhere },
        { 36, 9, branch_direction::up },
        { 44, 10, branch_direction::down | branch_direction::up },
    };

    EXPECT_THAT(bi, UnorderedElementsAreArray(expect_bi));
}
