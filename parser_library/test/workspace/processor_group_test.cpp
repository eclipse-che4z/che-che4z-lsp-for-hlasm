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

auto asm_options(config::assembler_options o) { return processor_group("", "", std::move(o), {}).asm_options(); }
auto pp_options(decltype(config::preprocessor_options::options) o)
{
    return processor_group("", "", {}, config::preprocessor_options { .options = std::move(o) }).preprocessor();
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

class processor_group_test : public diagnosable_impl, public testing::Test
{
public:
    void collect_diags() const override {}
};

TEST_F(processor_group_test, asm_options_optable_valid)
{
    std::string grp_name = "Group";
    config::assembler_options asm_opts;
    asm_opts.optable = "UNI";

    const auto cases = {
        std::make_pair("ZOP", instruction_set_version::ZOP),
        std::make_pair("ZS1", instruction_set_version::ZOP),
        std::make_pair("YOP", instruction_set_version::YOP),
        std::make_pair("ZS2", instruction_set_version::YOP),
        std::make_pair("Z9", instruction_set_version::Z9),
        std::make_pair("ZS3", instruction_set_version::Z9),
        std::make_pair("Z10", instruction_set_version::Z10),
        std::make_pair("ZS4", instruction_set_version::Z10),
        std::make_pair("Z11", instruction_set_version::Z11),
        std::make_pair("ZS5", instruction_set_version::Z11),
        std::make_pair("Z12", instruction_set_version::Z12),
        std::make_pair("ZS6", instruction_set_version::Z12),
        std::make_pair("Z13", instruction_set_version::Z13),
        std::make_pair("ZS7", instruction_set_version::Z13),
        std::make_pair("Z14", instruction_set_version::Z14),
        std::make_pair("ZS8", instruction_set_version::Z14),
        std::make_pair("Z15", instruction_set_version::Z15),
        std::make_pair("ZS9", instruction_set_version::Z15),
        std::make_pair("UNI", instruction_set_version::UNI),
        std::make_pair("DOS", instruction_set_version::DOS),
        std::make_pair("370", instruction_set_version::_370),
        std::make_pair("XA", instruction_set_version::XA),
        std::make_pair("ESA", instruction_set_version::ESA),
        std::make_pair("", instruction_set_version::UNI),
    };

    for (const auto& [input, expected] : cases)
    {
        diags().clear();

        asm_opts.optable = input;
        workspaces::processor_group proc_group("Group", "", asm_opts, {});

        auto instr_set = proc_group.asm_options().instr_set;

        collect_diags_from_child(proc_group);
        EXPECT_EQ(diags().size(), (size_t)0);

        EXPECT_EQ(instr_set, expected);
    }
}

TEST_F(processor_group_test, asm_options_optable_invalid)
{
    std::string grp_name = "Group";
    config::assembler_options asm_opts;
    asm_opts.optable = "UNI";

    const auto cases = {
        std::make_pair("klgadh", instruction_set_version::UNI),
        std::make_pair("ZS5ZS6", instruction_set_version::UNI),
        std::make_pair("ZS0", instruction_set_version::UNI),
        std::make_pair("Z8", instruction_set_version::UNI),
    };

    for (const auto& [input, expected] : cases)
    {
        diags().clear();

        asm_opts.optable = input;
        workspaces::processor_group proc_group("Group", "", asm_opts, {});

        auto instr_set = proc_group.asm_options().instr_set;

        collect_diags_from_child(proc_group);
        EXPECT_EQ(diags().size(), (size_t)1);
        EXPECT_TRUE(matches_message_codes(diags(), { "W0007" }));

        EXPECT_EQ(instr_set, expected);
    }
}
