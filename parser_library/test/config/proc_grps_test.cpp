/*
 * Copyright (c) 2021 Broadcom.
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

#include "config/proc_grps.h"
#include "nlohmann/json.hpp"

using namespace hlasm_plugin::parser_library::config;

TEST(proc_grps, library_read)
{
    const auto cases = {
        std::make_pair(R"("lib")"_json, library { "lib", {}, false }),
        std::make_pair(R"({"path":"lib"})"_json, library { "lib", {}, false }),
        std::make_pair(R"({"path":"lib","optional":false})"_json, library { "lib", {}, false }),
        std::make_pair(R"({"path":"lib","optional":true})"_json, library { "lib", {}, true }),
    };

    for (const auto& [input, expected] : cases)
    {
        const auto l = input.get<library>();
        EXPECT_EQ(l.path, expected.path);
        EXPECT_EQ(l.optional, expected.optional);
        EXPECT_EQ(l.macro_extensions, expected.macro_extensions);
    }
}

TEST(proc_grps, library_write)
{
    const library l = { "lib", {}, true };
    const auto expected = R"({"path":"lib","optional":true})"_json;

    EXPECT_EQ(nlohmann::json(l), expected);
}

static void compare_proc_grps(const proc_grps& pg, const proc_grps& expected)
{
    ASSERT_EQ(pg.pgroups.size(), expected.pgroups.size());
    for (size_t i = 0; i < pg.pgroups.size(); ++i)
    {
        EXPECT_EQ(pg.pgroups[i].name, expected.pgroups[i].name);
        EXPECT_EQ(pg.pgroups[i].asm_options.profile, expected.pgroups[i].asm_options.profile);
        EXPECT_EQ(pg.pgroups[i].asm_options.sysparm, expected.pgroups[i].asm_options.sysparm);
        EXPECT_EQ(pg.pgroups[i].preprocessor, expected.pgroups[i].preprocessor);
        ASSERT_EQ(pg.pgroups[i].libs.size(), expected.pgroups[i].libs.size());
        for (size_t j = 0; j < pg.pgroups[i].libs.size(); ++j)
        {
            EXPECT_EQ(pg.pgroups[i].libs[j].optional, expected.pgroups[i].libs[j].optional);
            EXPECT_EQ(pg.pgroups[i].libs[j].path, expected.pgroups[i].libs[j].path);
            EXPECT_EQ(pg.pgroups[i].libs[j].macro_extensions, expected.pgroups[i].libs[j].macro_extensions);
        }
    }
}

TEST(proc_grps, full_content_read)
{
    const auto cases = {
        std::make_pair(R"({"pgroups":[]})"_json, proc_grps {}),
        std::make_pair(R"({"pgroups":[{"name":"P1", "libs":["lib1", {"path": "lib2", "optional":true}]}]})"_json,
            proc_grps { { { "P1", { { "lib1", {}, false }, { "lib2", {}, true } } } } }),
        std::make_pair(
            R"({"pgroups":[{"name":"P1", "libs":["lib1", {"path": "lib2", "optional":true}]},{"name":"P2", "libs":["lib2_1", {"path": "lib2_2", "optional":true}]}]})"_json,
            proc_grps { { { "P1", { { "lib1", {}, false }, { "lib2", {}, true } } },
                { "P2", { { "lib2_1", {}, false }, { "lib2_2", {}, true } } } } }),
        std::make_pair(
            R"({"pgroups":[{"name":"P1", "libs":["lib1", {"path": "lib2", "optional":true}],"asm_options":{"SYSPARM":"PARAM","PROFILE":"PROFMAC"}},{"name":"P2", "libs":["lib2_1", {"path": "lib2_2", "optional":true}]}]})"_json,
            proc_grps { { { "P1", { { "lib1", {}, false }, { "lib2", {}, true } }, { "PARAM", "PROFMAC" } },
                { "P2", { { "lib2_1", {}, false }, { "lib2_2", {}, true } } } } }),
        std::make_pair(
            R"({"pgroups":[{"name":"P1", "libs":["lib1", {"path": "lib2", "optional":true,"macro_extensions":["mac"]}],"asm_options":{"SYSPARM":"PARAM","PROFILE":"PROFMAC"}},{"name":"P2", "libs":["lib2_1", {"path": "lib2_2", "optional":true}]}],"macro_extensions":["asmmac"]})"_json,
            proc_grps { { { "P1", { { "lib1", {}, false }, { "lib2", { "mac" }, true } }, { "PARAM", "PROFMAC" } },
                            { "P2", { { "lib2_1", {}, false }, { "lib2_2", {}, true } } } },
                { "asmmac" } }),
        std::make_pair(
            R"({"pgroups":[{"name":"P1", "libs":[]}]})"_json, proc_grps { { { "P1", {}, {}, std::monostate {} } } }),
        std::make_pair(R"({"pgroups":[{"name":"P1", "libs":[], "preprocessor":"DB2"}]})"_json,
            proc_grps { { { "P1", {}, {}, db2_preprocessor {} } } }),
        std::make_pair(R"({"pgroups":[{"name":"P1", "libs":[], "preprocessor":{"name":"DB2"}}]})"_json,
            proc_grps { { { "P1", {}, {}, db2_preprocessor {} } } }),
        std::make_pair(
            R"({"pgroups":[{"name":"P1", "libs":[], "preprocessor":{"name":"DB2","options":{"version":"AAA"}}}]})"_json,
            proc_grps { { { "P1", {}, {}, db2_preprocessor { "AAA" } } } }),
        std::make_pair(R"({"pgroups":[{"name":"P1", "libs":[], "preprocessor":{"name":"CICS"}}]})"_json,
            proc_grps { { { "P1", {}, {}, cics_preprocessor {} } } }),
        std::make_pair(
            R"({"pgroups":[{"name":"P1", "libs":[], "preprocessor":{"name":"CICS","options":["NOPROLOG"]}}]})"_json,
            proc_grps { { { "P1", {}, {}, cics_preprocessor { false } } } }),
        std::make_pair(
            R"({"pgroups":[{"name":"P1", "libs":[], "preprocessor":{"name":"CICS","options":["NOEPILOG","NOPROLOG"]}}]})"_json,
            proc_grps { { { "P1", {}, {}, cics_preprocessor { false, false } } } }),
        std::make_pair(
            R"({"pgroups":[{"name":"P1", "libs":[], "preprocessor":{"name":"CICS","options":["LEASM"]}}]})"_json,
            proc_grps { { { "P1", {}, {}, cics_preprocessor { true, true, true } } } }),
    };

    for (const auto& [input, expected] : cases)
        compare_proc_grps(input.get<proc_grps>(), expected);
}

TEST(proc_grps, full_content_write)
{
    const auto cases = {
        std::make_pair(R"({"pgroups":[]})"_json, proc_grps {}),
        std::make_pair(
            R"({"pgroups":[{"name":"P1", "libs":[{"path":"lib1","optional":false}, {"path": "lib2", "optional":true}]}]})"_json,
            proc_grps { { { "P1", { { "lib1", {}, false }, { "lib2", {}, true } } } } }),
        std::make_pair(
            R"({"pgroups":[{"name":"P1", "libs":[{"path":"lib1","optional":false}, {"path": "lib2", "optional":true}]},{"name":"P2", "libs":[{"path":"lib2_1","optional":false}, {"path": "lib2_2", "optional":true}]}]})"_json,
            proc_grps { { { "P1", { { "lib1", {}, false }, { "lib2", {}, true } } },
                { "P2", { { "lib2_1", {}, false }, { "lib2_2", {}, true } } } } }),
        std::make_pair(
            R"({"pgroups":[{"name":"P1", "libs":[{"path":"lib1","optional":false}, {"path": "lib2", "optional":true}],"asm_options":{"SYSPARM":"PARAM","PROFILE":"PROFMAC"}},{"name":"P2", "libs":[{"path":"lib2_1","optional":false}, {"path": "lib2_2", "optional":true}]}]})"_json,
            proc_grps { { { "P1", { { "lib1", {}, false }, { "lib2", {}, true } }, { "PARAM", "PROFMAC" } },
                { "P2", { { "lib2_1", {}, false }, { "lib2_2", {}, true } } } } }),
        std::make_pair(
            R"({"pgroups":[{"name":"P1", "libs":[{"path":"lib1","optional":false}, {"path": "lib2", "optional":true,"macro_extensions":["mac"]}],"asm_options":{"SYSPARM":"PARAM","PROFILE":"PROFMAC"}},{"name":"P2", "libs":[{"path":"lib2_1","optional":false}, {"path": "lib2_2", "optional":true}]}],"macro_extensions":["asmmac"]})"_json,
            proc_grps { { { "P1", { { "lib1", {}, false }, { "lib2", { "mac" }, true } }, { "PARAM", "PROFMAC" } },
                            { "P2", { { "lib2_1", {}, false }, { "lib2_2", {}, true } } } },
                { "asmmac" } }),
        std::make_pair(
            R"({"pgroups":[{"name":"P1", "libs":[]}]})"_json, proc_grps { { { "P1", {}, {}, std::monostate {} } } }),
        std::make_pair(R"({"pgroups":[{"name":"P1", "libs":[], "preprocessor":"DB2"}]})"_json,
            proc_grps { { { "P1", {}, {}, db2_preprocessor {} } } }),
        std::make_pair(
            R"({"pgroups":[{"name":"P1", "libs":[], "preprocessor":{"name":"DB2","options":{"version":"AAA"}}}]})"_json,
            proc_grps { { { "P1", {}, {}, db2_preprocessor { "AAA" } } } }),
        std::make_pair(R"({"pgroups":[{"name":"P1", "libs":[], "preprocessor":"CICS"}]})"_json,
            proc_grps { { { "P1", {}, {}, cics_preprocessor {} } } }),
        std::make_pair(
            R"({"pgroups":[{"name":"P1", "libs":[], "preprocessor":{"name":"CICS", "options":["NOPROLOG","NOEPILOG","LEASM"]}}]})"_json,
            proc_grps { { { "P1", {}, {}, cics_preprocessor { false, false, true } } } }),
    };

    for (const auto& [expected, input] : cases)
        EXPECT_EQ(nlohmann::json(input), expected);
}

TEST(proc_grps, invalid)
{
    const auto cases = {
        R"({})"_json,
        R"({"pgroups":[{}]})"_json,
        R"({"pgroups":[{"name":""}]})"_json,
        R"({"pgroups":[{"libs":[]}]})"_json,
        R"({"pgroups":[{"name":"","libs":[{}]}]})"_json,
        R"({"pgroups":[{"name":"","libs":[{}]}],"macro_extensions":{}})"_json,
        R"({"pgroups":[{"name":"","libs":[{}]}],"preprocessor":"invalid"})"_json,
        R"({"pgroups":[{"name":"","libs":[{}]}],"preprocessor":{}})"_json,
        R"({"pgroups":[{"name":"","libs":[{}]}],"preprocessor":{"name":"invalid"}})"_json,
        R"({"pgroups":[{"name":"","libs":[{}]}],"preprocessor":{"name":"DB2","options":""}})"_json,
        R"({"pgroups":[{"name":"","libs":[{}]}],"preprocessor":{"name":"DB2","options":{"version":1}}})"_json,
        R"({"pgroups":[{"name":"","libs":[{}]}],"preprocessor":{"name":"DB2","options":{"version":{}}}})"_json,
    };

    for (const auto& input : cases)
        EXPECT_THROW(input.get<proc_grps>(), nlohmann::json::exception);
}

TEST(proc_grps, preprocessor_options_validate)
{
    const auto cases = {
        std::make_pair(preprocessor_options { std::monostate {} }, true),
        std::make_pair(preprocessor_options { db2_preprocessor {} }, true),
        std::make_pair(preprocessor_options { cics_preprocessor {} }, true),
        std::make_pair(preprocessor_options { db2_preprocessor { "" } }, true),
        std::make_pair(preprocessor_options { db2_preprocessor { "aaa" } }, true),
        std::make_pair(preprocessor_options { db2_preprocessor { std::string(64, 'A') } }, true),
        std::make_pair(preprocessor_options { db2_preprocessor { std::string(65, 'A') } }, false),
        std::make_pair(preprocessor_options { db2_preprocessor { std::string(256, 'A') } }, false),
    };

    for (const auto& [input, expected] : cases)
        EXPECT_EQ(input.valid(), expected);
}
