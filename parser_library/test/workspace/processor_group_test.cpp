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

#include "gtest/gtest.h"

#include "../common_testing.h"
#include "config/proc_grps.h"
#include "workspaces/processor_group.h"

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::workspaces;

auto asm_options(config::assembler_options o) { return processor_group("", std::move(o), {}).asm_options(); }
auto pp_options(decltype(config::preprocessor_options::options) o)
{
    return processor_group("", {}, config::preprocessor_options { .options = std::move(o) }).preprocessor();
}

TEST(processor_group, assembler_options)
{
    EXPECT_EQ(asm_options({ .profile = "PROFILE" }).profile, "PROFILE");
    EXPECT_EQ(asm_options({ .sysparm = "SYSPARM" }).sysparm, "SYSPARM");
    EXPECT_EQ(asm_options({ .system_id = "SYSID" }).system_id, "SYSID");
    EXPECT_EQ(asm_options({}).system_id, asm_option::system_id_default);
    EXPECT_EQ(asm_options({}).sysopt_rent, asm_option::sysopt_rent_default);
}

TEST(processor_group, preprocessor_options)
{
    constexpr auto cics = [](auto... p) { return preprocessor_options(cics_preprocessor_options(std::move(p)...)); };
    constexpr auto db2 = [](auto... p) { return preprocessor_options(db2_preprocessor_options(std::move(p)...)); };

    EXPECT_EQ(pp_options(std::monostate()), preprocessor_options());

    EXPECT_EQ(pp_options(config::db2_preprocessor {}), db2());
    EXPECT_EQ(pp_options(config::db2_preprocessor { .version = "A" }), db2("A"));

    EXPECT_EQ(pp_options(config::cics_preprocessor {}), cics());
    EXPECT_EQ(pp_options(config::cics_preprocessor { .leasm = true }), cics(true, true, true));
    EXPECT_EQ(pp_options(config::cics_preprocessor { .prolog = false }), cics(false, true, false));
    EXPECT_EQ(pp_options(config::cics_preprocessor { .epilog = false }), cics(true, false, false));
    EXPECT_EQ(pp_options(config::cics_preprocessor { .prolog = false, .leasm = true }), cics(false, true, true));
}
