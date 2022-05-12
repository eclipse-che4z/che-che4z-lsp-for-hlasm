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

#include <utility>

#include "gtest/gtest.h"

#include "compiler_options.h"
#include "config/assembler_options.h"
#include "nlohmann/json.hpp"

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::config;

TEST(assembler_options, read)
{
    const auto cases = {
        std::make_pair(R"({})"_json, assembler_options {}),
        std::make_pair(R"({"PROFILE":"MAC"})"_json, assembler_options { .profile = "MAC" }),
        std::make_pair(R"({"SYSPARM":"TESTPARM"})"_json, assembler_options { .sysparm = "TESTPARM" }),
        std::make_pair(R"({"MACHINE":"zSeries-2"})"_json, assembler_options { .machine = "zSeries-2" }),
        std::make_pair(R"({"OPTABLE":"ZS9"})"_json, assembler_options { .optable = "ZS9" }),
        std::make_pair(R"({"SYSTEM_ID":"VSE"})"_json, assembler_options { .system_id = "VSE" }),
        std::make_pair(R"({"GOFF":true})"_json, assembler_options { .goff = true }),
        std::make_pair(R"({"XOBJECT":true})"_json, assembler_options { .goff = true }),
        std::make_pair(R"({"GOFF":true,"PROFILE":"MAC","SYSPARM":"TESTPARM","OPTABLE":"ZS9","SYSTEM_ID":"VSE"})"_json,
            assembler_options {
                .sysparm = "TESTPARM", .profile = "MAC", .optable = "ZS9", .system_id = "VSE", .goff = true }),
    };

    for (const auto& [input, expected] : cases)
    {
        EXPECT_EQ(input.get<assembler_options>(), expected);
    }
}

TEST(assembler_options, write)
{
    const auto cases = {
        std::make_pair(R"({})"_json, assembler_options {}),
        std::make_pair(R"({"PROFILE":"MAC"})"_json, assembler_options { .profile = "MAC" }),
        std::make_pair(R"({"SYSPARM":"TESTPARM"})"_json, assembler_options { .sysparm = "TESTPARM" }),
        std::make_pair(R"({"MACHINE":"zSeries-2"})"_json, assembler_options { .machine = "zSeries-2" }),
        std::make_pair(R"({"OPTABLE":"ZS9"})"_json, assembler_options { .optable = "ZS9" }),
        std::make_pair(R"({"SYSTEM_ID":"VSE"})"_json, assembler_options { .system_id = "VSE" }),
        std::make_pair(R"({"GOFF":true})"_json, assembler_options { .goff = true }),
        std::make_pair(R"({"GOFF":true,"PROFILE":"MAC","SYSPARM":"TESTPARM","OPTABLE":"ZS9","SYSTEM_ID":"VSE"})"_json,
            assembler_options {
                .sysparm = "TESTPARM", .profile = "MAC", .optable = "ZS9", .system_id = "VSE", .goff = true }),
    };

    for (const auto& [expected, input] : cases)
        EXPECT_EQ(nlohmann::json(input), expected);
}

TEST(assembler_options, validate)
{
    const auto cases = {
        std::make_pair(assembler_options {}, true),
        std::make_pair(assembler_options { "A" }, true),
        std::make_pair(assembler_options { "SYSPARM" }, true),
        std::make_pair(assembler_options { std::string(255, 'A') }, true),
        std::make_pair(assembler_options { std::string(256, 'A') }, false),
        std::make_pair(assembler_options { "", "", "" }, true),
        std::make_pair(assembler_options { "", "", "", "" }, false),
        std::make_pair(assembler_options { .optable = "UNI" }, true),
        std::make_pair(assembler_options { .optable = "A" }, false),
        std::make_pair(assembler_options { .machine = "zSeries-2" }, true),
        std::make_pair(assembler_options { .machine = "ZsErieS-2" }, true),
        std::make_pair(assembler_options { .machine = "A" }, false),
        std::make_pair(assembler_options { .goff = false }, true),
        std::make_pair(assembler_options { .goff = true }, true),
    };

    for (const auto& [input, expected] : cases)
        EXPECT_EQ(input.valid(), expected);
}

TEST(assembler_options, has_value)
{
    const auto cases = {
        std::make_pair(assembler_options {}, false),
        std::make_pair(assembler_options { "A" }, true),
        std::make_pair(assembler_options { "SYSPARM" }, true),
        std::make_pair(assembler_options { std::string(255, 'A') }, true),
        std::make_pair(assembler_options { std::string(256, 'A') }, true),
        std::make_pair(assembler_options { "", "", "" }, true),
        std::make_pair(assembler_options { "", "", "", "" }, true),
        std::make_pair(assembler_options { .optable = "UNI" }, true),
        std::make_pair(assembler_options { .optable = "A" }, true),
        std::make_pair(assembler_options { .machine = "ZSERIES-2" }, true),
        std::make_pair(assembler_options { .machine = "A" }, true),
        std::make_pair(assembler_options { .goff = false }, true),
        std::make_pair(assembler_options { .goff = true }, true),
    };

    for (const auto& [input, expected] : cases)
        EXPECT_EQ(input.has_value(), expected);
}

TEST(assembler_options, apply)
{
    const auto cases = {
        std::make_pair(std::vector<assembler_options> {}, asm_option {}),
        std::make_pair(
            std::vector<assembler_options> {
                { .sysparm = "PARM" },
            },
            asm_option { .sysparm = "PARM" }),
        std::make_pair(
            std::vector<assembler_options> {
                { .profile = "PARM" },
            },
            asm_option { .profile = "PARM" }),
        std::make_pair(
            std::vector<assembler_options> {
                { .system_id = "PARM" },
            },
            asm_option { .system_id = "PARM" }),
        std::make_pair(
            std::vector<assembler_options> {
                { .optable = "Z10" },
            },
            asm_option { .instr_set = instruction_set_version::Z10 }),
        std::make_pair(
            std::vector<assembler_options> {
                { .optable = "Z10" },
                { .optable = "Z11" },
            },
            asm_option { .instr_set = instruction_set_version::Z11 }),
        std::make_pair(
            std::vector<assembler_options> {
                { .optable = "Z10" },
                { .optable = "invalid" },
            },
            asm_option { .instr_set = instruction_set_version::Z10 }),
        std::make_pair(
            std::vector<assembler_options> {
                { .machine = "zSeries-2" },
            },
            asm_option { .instr_set = instruction_set_version::YOP }),
        std::make_pair(
            std::vector<assembler_options> {
                { .machine = "zSeries-2" },
                { .machine = "zSeries-3" },
            },
            asm_option { .instr_set = instruction_set_version::Z9 }),
        std::make_pair(
            std::vector<assembler_options> {
                { .machine = "zSeries-2" },
                { .machine = "invalid" },
            },
            asm_option { .instr_set = instruction_set_version::YOP }),
        std::make_pair(
            std::vector<assembler_options> {
                { .optable = "Z10" },
                { .machine = "zSeries-2" },
            },
            asm_option { .instr_set = instruction_set_version::YOP }),
        std::make_pair(std::vector<assembler_options> { { .goff = true } }, asm_option { .sysopt_xobject = true }),
        std::make_pair(
            std::vector<assembler_options> {
                { .goff = true },
                { .goff = true },
            },
            asm_option { .sysopt_xobject = true }),
        std::make_pair(
            std::vector<assembler_options> {
                { .goff = true },
                { .goff = false },
            },
            asm_option { .sysopt_xobject = false }),
        std::make_pair(
            std::vector<assembler_options> {
                { .goff = true },
                {},
            },
            asm_option { .sysopt_xobject = true }),
        std::make_pair(
            std::vector<assembler_options> {
                { .system_id = "PARM" },
                { .goff = true },
            },
            asm_option { .system_id = "PARM", .sysopt_xobject = true }),
    };

    for (const auto& [input, expected] : cases)
    {
        asm_option result;
        for (const auto& opt : input)
            opt.apply(result);
        EXPECT_EQ(result, expected) << nlohmann::json(input).dump(2);
    }
}