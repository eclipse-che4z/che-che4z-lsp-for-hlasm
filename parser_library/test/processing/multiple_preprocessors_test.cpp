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
