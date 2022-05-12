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

auto asm_options(config::assembler_options o)
{
    asm_option result;
    processor_group("", std::move(o), {}).update_asm_options(result);
    return result;
}
auto pp_options(decltype(config::preprocessor_options::options) o)
{
    return processor_group("", {}, config::preprocessor_options { .options = std::move(o) }).preprocessor();
}

TEST(processor_group, assembler_options)
{
    EXPECT_EQ(asm_options({ .optable = "XA" }).instr_set, instruction_set_version::XA);
    EXPECT_EQ(asm_options({}).instr_set, asm_option::instr_set_default);
    EXPECT_EQ(asm_options({ .profile = "PROFILE" }).profile, "PROFILE");
    EXPECT_EQ(asm_options({ .sysparm = "SYSPARM" }).sysparm, "SYSPARM");
    EXPECT_EQ(asm_options({ .system_id = "SYSID" }).system_id, "SYSID");
    EXPECT_EQ(asm_options({}).system_id, asm_option::system_id_default);
    EXPECT_EQ(asm_options({}).sysopt_rent, asm_option::sysopt_rent_default);
    EXPECT_EQ(asm_options({ .goff = true }).sysopt_xobject, true);
    EXPECT_EQ(asm_options({ .goff = false }).sysopt_xobject, false);
    EXPECT_EQ(asm_options({}).sysopt_xobject, asm_option::sysopt_xobject_default);
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

TEST(processor_group, asm_options_optable_valid)
{
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
        EXPECT_EQ(asm_options({ .optable = input }).instr_set, expected)
            << "Input: " << input << " Expected: " << int(expected);
    }
}

TEST(processor_group, asm_options_optable_invalid)
{
    const auto cases = {
        std::make_pair("klgadh", instruction_set_version::UNI),
        std::make_pair("ZS5ZS6", instruction_set_version::UNI),
        std::make_pair("ZS0", instruction_set_version::UNI),
        std::make_pair("Z8", instruction_set_version::UNI),
    };

    for (const auto& [input, expected] : cases)
    {
        EXPECT_EQ(asm_options({ .optable = input }).instr_set, expected)
            << "Input: " << input << " Expected: " << int(expected);
    }
}

TEST(processor_group, asm_options_machine_valid)
{
    const auto cases = {
        std::make_pair("ARCH-0", instruction_set_version::XA),
        std::make_pair("ARCH-1", instruction_set_version::ESA),
        std::make_pair("ARCH-10", instruction_set_version::Z12),
        std::make_pair("ARCH-11", instruction_set_version::Z13),
        std::make_pair("ARCH-12", instruction_set_version::Z14),
        std::make_pair("ARCH-13", instruction_set_version::Z15),
        std::make_pair("ARCH-2", instruction_set_version::ESA),
        std::make_pair("ARCH-3", instruction_set_version::ESA),
        std::make_pair("ARCH-4", instruction_set_version::ESA),
        std::make_pair("ARCH-5", instruction_set_version::ZOP),
        std::make_pair("ARCH-6", instruction_set_version::YOP),
        std::make_pair("ARCH-7", instruction_set_version::Z9),
        std::make_pair("ARCH-8", instruction_set_version::Z10),
        std::make_pair("ARCH-9", instruction_set_version::Z11),
        std::make_pair("S370", instruction_set_version::_370),
        std::make_pair("S370ESA", instruction_set_version::ESA),
        std::make_pair("S370XA", instruction_set_version::XA),
        std::make_pair("S390", instruction_set_version::ESA),
        std::make_pair("S390E", instruction_set_version::ESA),
        std::make_pair("ZS", instruction_set_version::ZOP),
        std::make_pair("ZS-1", instruction_set_version::ZOP),
        std::make_pair("ZS-2", instruction_set_version::YOP),
        std::make_pair("ZS-3", instruction_set_version::Z9),
        std::make_pair("ZS-4", instruction_set_version::Z10),
        std::make_pair("ZS-5", instruction_set_version::Z11),
        std::make_pair("ZS-6", instruction_set_version::Z12),
        std::make_pair("ZS-7", instruction_set_version::Z13),
        std::make_pair("ZS-8", instruction_set_version::Z14),
        std::make_pair("ZS-9", instruction_set_version::Z15),
        std::make_pair("z10", instruction_set_version::Z10),
        std::make_pair("z11", instruction_set_version::Z11),
        std::make_pair("z114", instruction_set_version::Z11),
        std::make_pair("z12", instruction_set_version::Z12),
        std::make_pair("z13", instruction_set_version::Z13),
        std::make_pair("z14", instruction_set_version::Z14),
        std::make_pair("z15", instruction_set_version::Z15),
        std::make_pair("z196", instruction_set_version::Z11),
        std::make_pair("z800", instruction_set_version::ZOP),
        std::make_pair("z890", instruction_set_version::YOP),
        std::make_pair("z9", instruction_set_version::Z9),
        std::make_pair("z900", instruction_set_version::ZOP),
        std::make_pair("z990", instruction_set_version::YOP),
        std::make_pair("zBC12", instruction_set_version::Z12),
        std::make_pair("zEC12", instruction_set_version::Z12),
        std::make_pair("zSeries", instruction_set_version::ZOP),
        std::make_pair("zSeries-1", instruction_set_version::ZOP),
        std::make_pair("zSeries-2", instruction_set_version::YOP),
        std::make_pair("zSeries-3", instruction_set_version::Z9),
        std::make_pair("zSeries-4", instruction_set_version::Z10),
        std::make_pair("zSeries-5", instruction_set_version::Z11),
        std::make_pair("zSeries-6", instruction_set_version::Z12),
        std::make_pair("zSeries-7", instruction_set_version::Z13),
        std::make_pair("zSeries-8", instruction_set_version::Z14),
        std::make_pair("zSeries-9", instruction_set_version::Z15),
    };

    for (const auto& [input, expected] : cases)
    {
        EXPECT_EQ(asm_options({ .machine = input }).instr_set, expected)
            << "Input: " << input << " Expected: " << int(expected);
    }
}

TEST(processor_group, asm_options_machine_invalid)
{
    const auto cases = {
        std::make_pair("klgadh", instruction_set_version::UNI),
        std::make_pair("ZS5ZS6", instruction_set_version::UNI),
        std::make_pair("ZS-0", instruction_set_version::UNI),
        std::make_pair("zSeries-0", instruction_set_version::UNI),
    };

    for (const auto& [input, expected] : cases)
    {
        EXPECT_EQ(asm_options({ .machine = input }).instr_set, expected)
            << "Input: " << input << " Expected: " << int(expected);
    }
}