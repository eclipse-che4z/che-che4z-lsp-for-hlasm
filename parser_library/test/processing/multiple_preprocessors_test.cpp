/*
 * Copyright (c) 2022 Broadcom.
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


#include "../common_testing.h"
#include "../mock_parse_lib_provider.h"

TEST(multiple_preprocessors, cics_and_db2)
{
    std::string input = R"(
         LARL 0,DFHVALUE(FIRSTQUIESCE)
         LARL 0,RO
RO       SQL  TYPE IS ROWID
         END
)";
    analyzer a(input,
        analyzer_options(std::vector<preprocessor_options> {
            cics_preprocessor_options(false, false, false),
            db2_preprocessor_options(),
        }));

    a.analyze();
    a.collect_diags();

    EXPECT_TRUE(a.diags().empty());
}

TEST(multiple_preprocessors, endevor_and_cics)
{
    mock_parse_lib_provider libs({
        { "MEMBER", R"(
         LARL 0,DFHVALUE(FIRSTQUIESCE)
)" },
    });
    std::string input = R"(
TEST    DS     0C
-INC MEMBER
        LTORG
TESTLEN EQU    *-TEST
        END
)";

    analyzer a(input,
        analyzer_options { &libs,
            std::vector<preprocessor_options> {
                preprocessor_options(endevor_preprocessor_options()),
                preprocessor_options(cics_preprocessor_options(false, false, false)),
            } });
    a.analyze();
    a.collect_diags();

    EXPECT_TRUE(a.diags().empty());

    EXPECT_EQ(get_symbol_abs(a.hlasm_ctx(), "TESTLEN"), 12);
}
