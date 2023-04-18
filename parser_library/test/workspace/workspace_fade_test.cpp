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

#include <algorithm>
#include <initializer_list>
#include <regex>
#include <string>
#include <utility>
#include <vector>

#include "gtest/gtest.h"

#include "../common_testing.h"
#include "consume_diagnostics_mock.h"
#include "empty_configs.h"
#include "fade_messages.h"
#include "lib_config.h"
#include "utils/resource_location.h"
#include "workspaces/file_manager_impl.h"
#include "workspaces/workspace.h"

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::workspaces;
using namespace hlasm_plugin::utils::resource;
using namespace hlasm_plugin::utils;

namespace {
const resource_location src1_loc("src1.hlasm");
const resource_location src2_loc("src2.hlasm");
const resource_location pgm_conf_loc(".hlasmplugin/pgm_conf.json");
const resource_location proc_grps_loc(".hlasmplugin/proc_grps.json");
const resource_location cpybook_loc("libs/CPYBOOK");
const resource_location mac_loc("libs/mac");

class file_manager_extended : public file_manager_impl
{
public:
    value_task<list_directory_result> list_directory_files(
        const hlasm_plugin::utils::resource::resource_location&) const override
    {
        return value_task<list_directory_result>::from_value({
            {
                { "CPYBOOK", cpybook_loc },
                { "MAC", mac_loc },
            },
            hlasm_plugin::utils::path::list_directory_rc::done,
        });
    }
};
} // namespace

namespace {
struct test_params
{
    std::vector<std::string> text_to_insert;
    std::vector<fade_message_s> expected_fade_messages;
    std::vector<std::string> diag_message_codes;
};

class fade_fixture_base : public diagnosable_impl, public ::testing::TestWithParam<test_params>
{
public:
    file_manager_extended file_manager;
    lib_config config;
    shared_json global_settings = make_empty_shared_json();
    workspace ws;
    std::vector<fade_message_s> fms;

    fade_fixture_base()
        : ws(workspace(file_manager, config, global_settings))
    {}

    void SetUp() override
    {
        file_manager.did_open_file(pgm_conf_loc, 1, pgm_conf);
        file_manager.did_open_file(proc_grps_loc, 1, proc_grps);
    }

    void collect_diags() const override
    {
        diags().clear();
        collect_diags_from_child(ws);
    }

    std::vector<diagnostic_s> collect_and_get_diags()
    {
        collect_diags();
        return diags();
    }

    void open_src_files_and_collect_fms(std::initializer_list<std::pair<resource_location, std::string>> files)
    {
        ws.open().run();

        for (const auto& [rl, text] : files)
        {
            file_manager.did_open_file(rl, 1, text);
            run_if_valid(ws.did_open_file(rl));
        }
        parse_all_files(ws);

        collect_fms();
    }

private:
    std::string pgm_conf = R"({
  "pgms": [
    {
      "program": "src?.hlasm",
      "pgroup": "P1"
    }
  ]
})";

    std::string proc_grps = R"({
  "pgroups": [
    {
      "name": "P1",
      "libs": ["libs"],
      "preprocessor": ["DB2"]
    }
  ]
})";

    void collect_fms()
    {
        fms.clear();
        ws.retrieve_fade_messages(fms);
    }
};

class opencode_general_fixture : public fade_fixture_base
{};

INSTANTIATE_TEST_SUITE_P(fade,
    opencode_general_fixture,
    ::testing::Values(
        test_params {
            { "0" },
            {},
        },
        test_params {
            { "1" },
            {
                fade_message_s::inactive_statement("src1.hlasm", range(position(3, 0), position(5, 80))),
            },
        }));

} // namespace

TEST_P(opencode_general_fixture, opencode)
{
    static const std::string src_template = R"(
         CSECT
         AIF ($x EQ 1).SKIP
&A       SETA 5
&C       SETC '12345678901234567890123456789012345678901234567890123456X
               789012345678901234567890'
.SKIP    ANOP

         END)";

    open_src_files_and_collect_fms({
        { src1_loc, std::regex_replace(src_template, std::regex("\\$x"), GetParam().text_to_insert[0]) },
    });

    EXPECT_TRUE(contains_message_codes(collect_and_get_diags(), GetParam().diag_message_codes));
    EXPECT_TRUE(matches_fade_messages(fms, GetParam().expected_fade_messages));
}

namespace {
class opencode_macros_fixture : public fade_fixture_base
{};

INSTANTIATE_TEST_SUITE_P(fade,
    opencode_macros_fixture,
    ::testing::Values(
        test_params {
            { "         MAC 0" },
            {},
        },
        test_params {
            { "         MAC 1" },
            {
                fade_message_s::inactive_statement("src1.hlasm", range(position(5, 0), position(5, 80))),
            },
        },
        test_params {
            { "*        MAC 1" },
            {
                fade_message_s::unused_macro("src1.hlasm", range(position(3, 0), position(3, 80))),
            },
        }));
} // namespace

TEST_P(opencode_macros_fixture, macros_opencode)
{
    static const std::string src_template = R"(
         CSECT
         MACRO
         MAC  &P
         AIF (&P EQ 1).SKIP
         ANOP
.SKIP    ANOP
         MEND

$x

         END)";

    open_src_files_and_collect_fms({
        { src1_loc, std::regex_replace(src_template, std::regex("\\$x"), GetParam().text_to_insert[0]) },
    });

    EXPECT_TRUE(contains_message_codes(collect_and_get_diags(), GetParam().diag_message_codes));
    EXPECT_TRUE(matches_fade_messages(fms, GetParam().expected_fade_messages));
}

TEST_P(opencode_macros_fixture, macros_opencode_no_section)
{
    static const std::string src_template = R"(
         MACRO
         MAC  &P
         AIF (&P EQ 1).SKIP
         ANOP
.SKIP    ANOP
         MEND

$x
)";

    open_src_files_and_collect_fms({
        { src1_loc, std::regex_replace(src_template, std::regex("\\$x"), GetParam().text_to_insert[0]) },
    });

    EXPECT_TRUE(contains_message_codes(collect_and_get_diags(), GetParam().diag_message_codes));
    EXPECT_TRUE(fms.empty());
}

namespace {
class opencode_deferred_fixture : public fade_fixture_base
{};

INSTANTIATE_TEST_SUITE_P(fade,
    opencode_deferred_fixture,
    ::testing::Values(test_params {
        {},
        {
            fade_message_s::inactive_statement("src1.hlasm", range(position(3, 0), position(3, 80))),
            fade_message_s::inactive_statement("src1.hlasm", range(position(5, 0), position(5, 80))),
            fade_message_s::inactive_statement("src1.hlasm", range(position(7, 0), position(7, 80))),
        },
    }));
} // namespace

TEST_P(opencode_deferred_fixture, opencode_deferred)
{
    static const std::string src = R"(
         CSECT
         AIF (L'X EQ 4).SKIP1
         SAM31
.SKIP1   ANOP

         AGO .SKIP2
X        DS F
.SKIP2   ANOP

         END
)";

    open_src_files_and_collect_fms({
        { src1_loc, src },
    });

    EXPECT_TRUE(contains_message_codes(collect_and_get_diags(), GetParam().diag_message_codes));
    EXPECT_TRUE(matches_fade_messages(fms, GetParam().expected_fade_messages));
}

namespace {
class opencode_macros_inner_fixture : public fade_fixture_base
{};

INSTANTIATE_TEST_SUITE_P(fade,
    opencode_macros_inner_fixture,
    ::testing::Values(
        test_params {
            { "         MAC 0" },
            {},
        },
        test_params {
            { "         MAC 1" },
            {
                fade_message_s::unused_macro("src1.hlasm", range(position(5, 0), position(5, 80))),
                fade_message_s::inactive_statement("src1.hlasm", range(position(9, 0), position(9, 80))),
            },
        },
        test_params {
            { "*        MAC 1" },
            {
                fade_message_s::unused_macro("src1.hlasm", range(position(3, 0), position(3, 80))),
            },
        }));
} // namespace

TEST_P(opencode_macros_inner_fixture, macros_opencode_inner)
{
    static const std::string src_template = R"(
         CSECT
         MACRO
         MAC  &P
         MACRO
         MAC_INNER
         ANOP
         MEND
         AIF (&P EQ 1).SKIP
         MAC_INNER
.SKIP    ANOP
         MEND

$x

         END)";

    open_src_files_and_collect_fms({
        { src1_loc, std::regex_replace(src_template, std::regex("\\$x"), GetParam().text_to_insert[0]) },
    });

    EXPECT_TRUE(contains_message_codes(collect_and_get_diags(), GetParam().diag_message_codes));
    EXPECT_TRUE(matches_fade_messages(fms, GetParam().expected_fade_messages));
}

namespace {
class opencode_multiple_macros_inner_fixture : public fade_fixture_base
{};

INSTANTIATE_TEST_SUITE_P(fade,
    opencode_multiple_macros_inner_fixture,
    ::testing::Values(
        test_params {
            {
                R"(
        OPEN
        INNER)",
            },
            {
                fade_message_s::unused_macro("src1.hlasm", range(position(10, 0), position(10, 80))),
            },
        },
        test_params {
            {
                R"(
*       OPEN
        INNER)",
            },
            {
                fade_message_s::unused_macro("src1.hlasm", range(position(2, 0), position(2, 80))),
            },
        },
        test_params {
            {
                R"(
        OPEN
*       INNER)",
            },
            {
                fade_message_s::unused_macro("src1.hlasm", range(position(4, 0), position(4, 80))),
                fade_message_s::unused_macro("src1.hlasm", range(position(10, 0), position(10, 80))),
            },
        },
        test_params {
            {
                R"(
*       OPEN
*       INNER)",
            },
            {
                fade_message_s::unused_macro("src1.hlasm", range(position(2, 0), position(2, 80))),
                fade_message_s::unused_macro("src1.hlasm", range(position(10, 0), position(10, 80))),
            },
        }));
} // namespace

TEST_P(opencode_multiple_macros_inner_fixture, multiple_macros_opencode_inner)
{
    static const std::string src_template = R"(         CSECT
         MACRO
         OPEN
         MACRO
         INNER
         ANOP
         MEND
         MEND

         MACRO
         INNER
         ANOP
         MEND

$x

         END)";

    open_src_files_and_collect_fms({
        { src1_loc, std::regex_replace(src_template, std::regex("\\$x"), GetParam().text_to_insert[0]) },
    });

    EXPECT_TRUE(contains_message_codes(collect_and_get_diags(), GetParam().diag_message_codes));
    EXPECT_TRUE(matches_fade_messages(fms, GetParam().expected_fade_messages));
}

namespace {
class opencode_macros_external_fixture : public fade_fixture_base
{};

INSTANTIATE_TEST_SUITE_P(fade,
    opencode_macros_external_fixture,
    ::testing::Values(
        test_params {
            { "         MAC 0" },
            {},
        },
        test_params {
            { "         MAC 1" },
            {
                fade_message_s::inactive_statement("libs/mac", range(position(3, 0), position(3, 80))),
            },
        },
        test_params {
            { "*        MAC 1" },
            {},
        }));
} // namespace

TEST_P(opencode_macros_external_fixture, macros_external)
{
    std::string mac = R"(         MACRO
         MAC  &P
         AIF (&P EQ 1).SKIP
         ANOP
.SKIP    ANOP
         MEND

* SOME MEANINGFUL REMARKS)";

    static const std::string src_template = R"(
         CSECT
$x

         END)";

    open_src_files_and_collect_fms({
        { mac_loc, std::move(mac) },
        { src1_loc, std::regex_replace(src_template, std::regex("\\$x"), GetParam().text_to_insert[0]) },
    });

    EXPECT_TRUE(contains_message_codes(collect_and_get_diags(), GetParam().diag_message_codes));
    EXPECT_TRUE(matches_fade_messages(fms, GetParam().expected_fade_messages));
}

namespace {
class opencode_macros_external_nested_fixture : public fade_fixture_base
{};

INSTANTIATE_TEST_SUITE_P(fade,
    opencode_macros_external_nested_fixture,
    ::testing::Values(
        test_params {
            {
                "         NEST",
                "         MAC 0",
            },
            {},
        },
        test_params {
            {
                "         NEST",
                "         MAC 1",
            },
            {
                fade_message_s::inactive_statement("libs/mac", range(position(3, 0), position(3, 80))),
            },
        },
        test_params {
            {
                "*        NEST",
                "         MAC 0",
            },
            {
                fade_message_s::unused_macro("libs/mac", range(position(7, 0), position(7, 80))),
            },
        },
        test_params {
            {
                "*        NEST",
                "         MAC 1",
            },
            {
                fade_message_s::inactive_statement("libs/mac", range(position(3, 0), position(3, 80))),
                fade_message_s::unused_macro("libs/mac", range(position(7, 0), position(7, 80))),
            },
        },
        test_params {
            {
                "         NEST",
                "*        MAC 1",
            },
            {},
        },
        test_params {
            {
                "*        NEST",
                "*        MAC 1",
            },
            {},
        }));
} // namespace

TEST_P(opencode_macros_external_nested_fixture, macros_external_nested)
{
    std::string mac_template = R"(         MACRO
         MAC  &P
         AIF (&P EQ 1).SKIP
         ANOP
.SKIP    ANOP

         MACRO
         NEST
         ANOP
         MEND

$y

         MEND

* SOME MEANINGFUL REMARKS)";

    static const std::string src_template = R"(
         CSECT
$x

         END)";

    open_src_files_and_collect_fms({
        { mac_loc, std::regex_replace(mac_template, std::regex("\\$y"), GetParam().text_to_insert[0]) },
        { src1_loc, std::regex_replace(src_template, std::regex("\\$x"), GetParam().text_to_insert[1]) },
    });

    EXPECT_TRUE(contains_message_codes(collect_and_get_diags(), GetParam().diag_message_codes));
    EXPECT_TRUE(matches_fade_messages(fms, GetParam().expected_fade_messages));
}

namespace {
class multiple_opencodes_and_macros_fixture : public fade_fixture_base
{};

INSTANTIATE_TEST_SUITE_P(fade,
    multiple_opencodes_and_macros_fixture,
    ::testing::Values(
        test_params {
            {
                "         MAC1 0",
                "*        MAC3 0",
            },
            {
                fade_message_s::unused_macro("libs/CPYBOOK", range(position(10, 0), position(10, 80))),
                fade_message_s::unused_macro("libs/CPYBOOK", range(position(20, 0), position(20, 80))),
            },
        },
        test_params {
            {
                "         MAC1 1",
                "*        MAC3 1",
            },
            {
                fade_message_s::inactive_statement("libs/CPYBOOK", range(position(3, 0), position(3, 80))),
                fade_message_s::unused_macro("libs/CPYBOOK", range(position(10, 0), position(10, 80))),
                fade_message_s::unused_macro("libs/CPYBOOK", range(position(20, 0), position(20, 80))),
            },
        },
        test_params {
            {
                "*        MAC1 0",
                "         MAC3 0",
            },
            {
                fade_message_s::unused_macro("libs/CPYBOOK", range(position(1, 0), position(1, 80))),
                fade_message_s::unused_macro("libs/CPYBOOK", range(position(10, 0), position(10, 80))),
            },
        },
        test_params {
            {
                "*        MAC1 1",
                "         MAC3 1",
            },
            {
                fade_message_s::unused_macro("libs/CPYBOOK", range(position(1, 0), position(1, 80))),
                fade_message_s::unused_macro("libs/CPYBOOK", range(position(10, 0), position(10, 80))),
                fade_message_s::inactive_statement("libs/CPYBOOK", range(position(22, 0), position(22, 80))),
            },
        },
        test_params {
            {
                "         MAC1 0",
                "         MAC3 0",
            },
            {
                fade_message_s::unused_macro("libs/CPYBOOK", range(position(10, 0), position(10, 80))),
            },
        },
        test_params {
            {
                "         MAC1 1",
                "         MAC3 0",
            },
            {
                fade_message_s::inactive_statement("libs/CPYBOOK", range(position(3, 0), position(3, 80))),
                fade_message_s::unused_macro("libs/CPYBOOK", range(position(10, 0), position(10, 80))),
            },
        },
        test_params {
            {
                "         MAC1 0",
                "         MAC3 1",
            },
            {
                fade_message_s::unused_macro("libs/CPYBOOK", range(position(10, 0), position(10, 80))),
                fade_message_s::inactive_statement("libs/CPYBOOK", range(position(22, 0), position(22, 80))),
            },
        },
        test_params {
            {
                "         MAC1 1",
                "         MAC3 1",
            },
            {
                fade_message_s::inactive_statement("libs/CPYBOOK", range(position(3, 0), position(3, 80))),
                fade_message_s::unused_macro("libs/CPYBOOK", range(position(10, 0), position(10, 80))),
                fade_message_s::inactive_statement("libs/CPYBOOK", range(position(22, 0), position(22, 80))),
            },
        },
        test_params {
            {
                "         MAC3 0",
                "         MAC3 0",
            },
            {
                fade_message_s::unused_macro("libs/CPYBOOK", range(position(1, 0), position(1, 80))),
                fade_message_s::unused_macro("libs/CPYBOOK", range(position(10, 0), position(10, 80))),
            },
        },
        test_params {
            {
                "         MAC3 1",
                "         MAC3 0",
            },
            {
                fade_message_s::unused_macro("libs/CPYBOOK", range(position(1, 0), position(1, 80))),
                fade_message_s::unused_macro("libs/CPYBOOK", range(position(10, 0), position(10, 80))),
            },
        },
        test_params {
            {
                "         MAC3 0",
                "         MAC3 1",
            },
            {
                fade_message_s::unused_macro("libs/CPYBOOK", range(position(1, 0), position(1, 80))),
                fade_message_s::unused_macro("libs/CPYBOOK", range(position(10, 0), position(10, 80))),
            },
        },
        test_params {
            {
                "         MAC3 1",
                "         MAC3 1",
            },
            {
                fade_message_s::unused_macro("libs/CPYBOOK", range(position(1, 0), position(1, 80))),
                fade_message_s::unused_macro("libs/CPYBOOK", range(position(10, 0), position(10, 80))),
                fade_message_s::inactive_statement("libs/CPYBOOK", range(position(22, 0), position(22, 80))),
            },
        }));
} // namespace

TEST_P(multiple_opencodes_and_macros_fixture, macros_external)
{
    std::string cpybook = R"(         MACRO
         MAC1  &P 
         AIF (&P EQ 1).SKIP1
         ANOP
.SKIP1   ANOP
         MEND

* * SOME MEANINGFUL REMARKS

         MACRO
         MAC_UNUSED  &P
         AIF (&P EQ 1).SKIP2
         ANOP
.SKIP2   ANOP
         MEND

         GBLC &A
         GBLC &B

         MACRO
         MAC3  &P
         AIF (&P EQ 1).SKIP3
         ANOP
.SKIP3   ANOP
         MEND
)";

    static const std::string src_template = R"(
         CSECT
         COPY CPYBOOK
$x

         END)";

    open_src_files_and_collect_fms({
        { cpybook_loc, std::move(cpybook) },
        { src1_loc, std::regex_replace(src_template, std::regex("\\$x"), GetParam().text_to_insert[0]) },
        { src2_loc, std::regex_replace(src_template, std::regex("\\$x"), GetParam().text_to_insert[1]) },
    });

    EXPECT_TRUE(contains_message_codes(collect_and_get_diags(), GetParam().diag_message_codes));
    EXPECT_TRUE(matches_fade_messages(fms, GetParam().expected_fade_messages));
}

namespace {
class cpybooks_fixture : public fade_fixture_base
{};

INSTANTIATE_TEST_SUITE_P(fade,
    cpybooks_fixture,
    ::testing::Values(
        test_params {
            { "0", "0" },
            {},
        },
        test_params {
            { "0", "1" },
            {},
        },
        test_params {
            { "1", "0" },
            {},
        },
        test_params {
            { "1", "1" },
            { fade_message_s::inactive_statement("libs/CPYBOOK", range(position(2, 0), position(2, 80))) },
        }));
} // namespace

TEST_P(cpybooks_fixture, cpybook)
{
    std::string cpybook = R"(
         AIF (&VAR EQ 1).SKIP
LABEL    L 1,1
.SKIP    ANOP)";

    static const std::string src_template = R"(
         CSECT
&VAR     SETA  $x
         COPY CPYBOOK
         END)";

    open_src_files_and_collect_fms({
        { cpybook_loc, std::move(cpybook) },
        { src1_loc, std::regex_replace(src_template, std::regex("\\$x"), GetParam().text_to_insert[0]) },
        { src2_loc, std::regex_replace(src_template, std::regex("\\$x"), GetParam().text_to_insert[1]) },
    });

    EXPECT_TRUE(contains_message_codes(collect_and_get_diags(), GetParam().diag_message_codes));
    EXPECT_TRUE(matches_fade_messages(fms, GetParam().expected_fade_messages));
}

namespace {
class opencode_nested_fixture : public fade_fixture_base
{};

INSTANTIATE_TEST_SUITE_P(fade,
    opencode_nested_fixture,
    ::testing::Values(
        test_params {
            { "         MAC 0,0" },
            {},
        },
        test_params {
            { "         MAC 0,1" },
            {
                fade_message_s::inactive_statement("libs/CPYBOOK", range(position(2, 0), position(2, 80))),
                fade_message_s::inactive_statement("libs/mac", range(position(4, 0), position(4, 80))),
            },
        },
        test_params {
            { "         MAC 1,0" },
            {
                fade_message_s::inactive_statement("libs/CPYBOOK", range(position(1, 0), position(2, 80))),
                fade_message_s::inactive_statement("libs/mac", range(position(3, 0), position(3, 80))),
            },
        },
        test_params {
            { "         MAC 1,1" },
            {
                fade_message_s::inactive_statement("libs/CPYBOOK", range(position(1, 0), position(2, 80))),
                fade_message_s::inactive_statement("libs/mac", range(position(3, 0), position(3, 80))),
            },
        },
        test_params {
            { "*        MAC 1,1" },
            {},
            {
                "E010",
                "E058", // Diags related to missing members in mac and cpybook
            },
        }));
} // namespace

TEST_P(opencode_nested_fixture, nested)
{
    std::string cpybook = R"(
         AIF (&P2 EQ 1).SKIP2
LABEL    L 1,1)";

    std::string mac = R"(         MACRO
         MAC  &P1,&P2
         AIF (&P1 EQ 1).SKIP
         COPY CPYBOOK
.SKIP    ANOP
.SKIP2   ANOP
         MEND)";

    std::string src_template = R"(
         CSECT
$x

         END)";

    open_src_files_and_collect_fms({
        { mac_loc, std::move(mac) },
        { cpybook_loc, std::move(cpybook) },
        { src1_loc, std::regex_replace(src_template, std::regex("\\$x"), GetParam().text_to_insert[0]) },
    });

    EXPECT_TRUE(contains_message_codes(collect_and_get_diags(), GetParam().diag_message_codes));
    EXPECT_TRUE(matches_fade_messages(fms, GetParam().expected_fade_messages));
}

namespace {
class opencode_nested_cycle_fixture : public fade_fixture_base
{};

INSTANTIATE_TEST_SUITE_P(fade,
    opencode_nested_cycle_fixture,
    ::testing::Values(
        test_params {
            { "         MAC 0,0" },
            {},
            {
                "E062", // Diags related to recursive copybook
            },
        },
        test_params {
            { "         MAC 0,1" },
            {},
            {
                "E062", // Diags related to recursive copybook
            },
        },
        test_params {
            { "         MAC 1,0" },
            {},
            {
                "E062", // Diags related to recursive copybook
            },
        },
        test_params {
            { "         MAC 1,1" },
            {},
            {
                "E062", // Diags related to recursive copybook
            },
        },
        test_params {
            { "*        MAC 1,1" },
            {},
            {
                "E010",
                "E058", // Diags related to missing members in mac and cpybook
            },
        }));
} // namespace

TEST_P(opencode_nested_cycle_fixture, nested_cycle)
{
    std::string cpybook = R"(
         AIF (&P2 EQ 1).SKIP2
LABEL    L 1,1
         COPY MAC)";

    std::string mac = R"(         MACRO
         MAC  &P1,&P2
         AIF (&P1 EQ 1).SKIP
         COPY CPYBOOK
.SKIP    ANOP
.SKIP2   ANOP
         MEND)";

    std::string src_template = R"(
         CSECT
$x

         END)";

    open_src_files_and_collect_fms({
        { mac_loc, std::move(mac) },
        { cpybook_loc, std::move(cpybook) },
        { src1_loc, std::regex_replace(src_template, std::regex("\\$x"), GetParam().text_to_insert[0]) },
    });

    EXPECT_TRUE(contains_message_codes(collect_and_get_diags(), GetParam().diag_message_codes));
    // No check for inactive statements -> expectation is to not crash
}

namespace {
class lookeahead_fixture : public fade_fixture_base
{};

INSTANTIATE_TEST_SUITE_P(fade,
    lookeahead_fixture,
    ::testing::Values(
        test_params {
            { "0" },
            {
                fade_message_s::inactive_statement("src1.hlasm", range(position(4, 0), position(4, 80))),
                fade_message_s::inactive_statement("src1.hlasm", range(position(6, 0), position(6, 80))),
            },
        },
        test_params {
            { "1" },
            {
                fade_message_s::inactive_statement("src1.hlasm", range(position(4, 0), position(4, 80))),
                fade_message_s::inactive_statement("libs/mac", range(position(4, 0), position(5, 80))),
            },
        }));
} // namespace

TEST_P(lookeahead_fixture, nested_lookahead)
{
    std::string mac = R"(         MACRO
         MAC &P,&SYMBOL

         AIF   (&P EQ 1).SKIP
         AIF   (1 EQ 0).NOEXIST
&LEN     SETA  L'&SYMBOL
.SKIP ANOP

         MEXIT
         MEND
)";

    std::string src_template = R"(
         CSECT
         MAC $x,SYM
         AGO .JUMP
         ANOP
.JUMP    ANOP

SYM      DS XL8
         END   )";

    open_src_files_and_collect_fms({
        { mac_loc, std::move(mac) },
        { src1_loc, std::regex_replace(src_template, std::regex("\\$x"), GetParam().text_to_insert[0]) },
    });

    EXPECT_TRUE(contains_message_codes(collect_and_get_diags(), GetParam().diag_message_codes));
    EXPECT_TRUE(matches_fade_messages(fms, GetParam().expected_fade_messages));
}

namespace {
class aread_and_lookahead_fixture : public fade_fixture_base
{};

INSTANTIATE_TEST_SUITE_P(fade,
    aread_and_lookahead_fixture,
    ::testing::Values(test_params {
        {},
        {
            fade_message_s::inactive_statement("src1.hlasm", range(position(17, 0), position(17, 80))),
        },
    }));
} // namespace

TEST_P(aread_and_lookahead_fixture, test)
{
    static const std::string src = R"(
         CSECT
         MACRO
         MAC   &LINES2READ
         AIF (&LINES2READ EQ 0).SKIP
&LINENO  SETA 0
.LOOP    ANOP
&LINENO  SETA &LINENO+1
&TEXT    AREAD  NOSTMT
         AIF (&LINENO LT &LINES2READ).LOOP
.SKIP    MEND

&LEN     SETA  L'SYM
         MAC 3
 SELECT * FROM TABLE
 SELECT * FROM TABLE                        WHERE 'SYSTEM' = 'SOL'  AND
 'PLANET' = EARTH

SYM      DS XL8
         END 
)";

    open_src_files_and_collect_fms({
        { src1_loc, src },
    });

    EXPECT_TRUE(contains_message_codes(collect_and_get_diags(), GetParam().diag_message_codes));
    EXPECT_TRUE(matches_fade_messages(fms, GetParam().expected_fade_messages));
}

namespace {
class file_manager_impl_test : public file_manager_impl
{
    value_task<std::optional<std::string>> get_file_content(const resource_location& rl) override
    {
        if (rl.get_uri().ends_with("proc_grps.json"))
            return value_task<std::optional<std::string>>::from_value(proc_grps);

        if (rl.get_uri().ends_with("pgm_conf.json"))
            return value_task<std::optional<std::string>>::from_value(pgm_conf);

        return value_task<std::optional<std::string>>::from_value(std::nullopt);
    }

    value_task<list_directory_result> list_directory_files(const resource_location& directory) const override
    {
        list_directory_result result;

        for (const auto& [file_rl, _] : get_files())
        {
            auto rel_path = file_rl.lexically_relative(directory);
            if (rel_path.empty() || rel_path.lexically_out_of_scope())
                continue;

            if (const auto& filename = rel_path.get_uri(); filename.find('/') == std::string::npos)
                result.first.emplace_back(std::make_pair(filename, file_rl));
        }

        result.second = hlasm_plugin::utils::path::list_directory_rc::done;

        return value_task<list_directory_result>::from_value(std::move(result));
    }

private:
    inline static const std::string proc_grps = R"({
  "pgroups": [
    {
      "name": "GRP1",
      "libs": [""]
    }
  ]
})";

    inline static const std::string pgm_conf = R"({
  "pgms": [
    {
      "program": "A.hlasm",
      "pgroup": "GRP1",
      "asm_options": {
        "SYSPARM": "A"
      }
    },
    {
      "program": "B.hlasm",
      "pgroup": "GRP1",
      "asm_options": {
        "SYSPARM": "B"
      }
    },
    {
      "program": "C.hlasm",
      "pgroup": "GRP1",
      "asm_options": {
        "SYSPARM": "C"
      }
    }
  ]
})";
};

class fade_helper
{
public:
    struct files_details
    {
        resource_location rl;
        bool is_cpybook;
        workspaces::file_content_state open_file_res;
    };

    fade_helper(const std::vector<files_details>& files_to_open)
    {
        for (const auto& [rl, is_cpybook, _] : files_to_open)
            m_fm.did_open_file(rl, 1, is_cpybook ? cpybook : source_template);

        ws.open().run();
        for (const auto& [rl, _, open_file_res] : files_to_open)
            run_if_valid(ws.did_open_file(rl, open_file_res));
        parse_all_files(ws);
    }

    std::vector<fade_message_s>& fade_messages()
    {
        m_fmsgs.clear();
        ws.retrieve_fade_messages(m_fmsgs);
        return m_fmsgs;
    }

    void did_close_file(resource_location rl)
    {
        run_if_valid(ws.did_close_file(rl));
        parse_all_files(ws);
    }
    void did_open_file(resource_location rl)
    {
        m_fm.did_open_file(rl, 1, source_template);
        run_if_valid(ws.did_open_file(rl));
        parse_all_files(ws);
    }


private:
    const static inline std::string source_template = R"(
         CSECT
         COPY  C)";

    const static inline std::string cpybook = R"(
         CSECT
         AGO   .SKIP
         ANOP
.SKIP    AIF   ('&SYSPARM' EQ 'A').A
         AIF   ('&SYSPARM' EQ 'B').B
         MNOTE 'C'
         AGO   .END

.A       ANOP
         MNOTE 'A'
         AGO   .END
.B       ANOP
         MNOTE 'B'
         AGO   .END

.END     ANOP
)";

    file_manager_impl_test m_fm;
    const lib_config m_empty_config;
    const shared_json m_global_settings = make_empty_shared_json();
    workspace ws = workspace(m_fm, m_empty_config, m_global_settings);
    std::vector<fade_message_s> m_fmsgs;
};
} // namespace

TEST(fade, cpybook_as_pgm)
{
    static const resource_location srcA_loc("A.hlasm");
    static const resource_location srcB_loc("B.hlasm");
    static const resource_location srcC_loc("C.hlasm");

    fade_helper fh(std::vector<fade_helper::files_details>({
        fade_helper::files_details { srcA_loc, false, workspaces::file_content_state::changed_content },
        fade_helper::files_details { srcB_loc, false, workspaces::file_content_state::changed_content },
        fade_helper::files_details { srcC_loc, true, workspaces::file_content_state::changed_lsp },
    }));

    EXPECT_TRUE(matches_fade_messages(fh.fade_messages(),
        std::vector<fade_message_s>({
            fade_message_s::inactive_statement("C.hlasm", range(position(3, 0), position(3, 80))),
            fade_message_s::inactive_statement("C.hlasm", range(position(8, 0), position(8, 80))),
            fade_message_s::inactive_statement("C.hlasm", range(position(15, 0), position(15, 80))),
        })));

    // Toggle open and close of a file a few times
    fh.did_close_file(srcA_loc);
    EXPECT_TRUE(matches_fade_messages(fh.fade_messages(),
        std::vector<fade_message_s>({
            fade_message_s::inactive_statement("C.hlasm", range(position(3, 0), position(3, 80))),
            fade_message_s::inactive_statement("C.hlasm", range(position(8, 0), position(11, 80))),
            fade_message_s::inactive_statement("C.hlasm", range(position(15, 0), position(15, 80))),
        })));

    fh.did_open_file(srcA_loc);
    EXPECT_TRUE(matches_fade_messages(fh.fade_messages(),
        std::vector<fade_message_s>({
            fade_message_s::inactive_statement("C.hlasm", range(position(3, 0), position(3, 80))),
            fade_message_s::inactive_statement("C.hlasm", range(position(8, 0), position(8, 80))),
            fade_message_s::inactive_statement("C.hlasm", range(position(15, 0), position(15, 80))),
        })));

    fh.did_close_file(srcA_loc);
    EXPECT_TRUE(matches_fade_messages(fh.fade_messages(),
        std::vector<fade_message_s>({
            fade_message_s::inactive_statement("C.hlasm", range(position(3, 0), position(3, 80))),
            fade_message_s::inactive_statement("C.hlasm", range(position(8, 0), position(11, 80))),
            fade_message_s::inactive_statement("C.hlasm", range(position(15, 0), position(15, 80))),
        })));

    fh.did_close_file(srcB_loc);
    EXPECT_TRUE(matches_fade_messages(fh.fade_messages(),
        std::vector<fade_message_s>({
            fade_message_s::inactive_statement("C.hlasm", range(position(3, 0), position(3, 80))),
            fade_message_s::inactive_statement("C.hlasm", range(position(8, 0), position(15, 80))),
        })));

    fh.did_close_file(srcC_loc);
    EXPECT_EQ(fh.fade_messages().size(), static_cast<size_t>(0));
}

TEST(fade, preprocessor)
{
    workspace_manager ws_mngr;
    diag_consumer_mock consumer;
    ws_mngr.register_diagnostics_consumer(&consumer);

    ws_mngr.add_workspace("workspace", "test/library/test_wks");
    std::string pgm_conf = R"({
  "pgms": [
    {
      "program": "file*",
      "pgroup": "P1"
    }
  ]
})";

    std::string proc_grps = R"({
  "pgroups": [
    {
      "name": "P1",
      "preprocessor":[{
          "name": "CICS",
          "options": [
            "NOEPILOG",
            "NOPROLOG"
          ]
        }],
      "libs": []
    }
  ]
})";
    std::string f1 = R"(
         USING *,12
         MACRO
         DFHECALL
         MEND

A        EXEC CICS ABEND ABCODE('1234')

         DFHECALL
         END)";

    ws_mngr.did_open_file("test/library/test_wks/.hlasmplugin/pgm_conf.json", 1, pgm_conf.c_str(), pgm_conf.size());
    ws_mngr.did_open_file("test/library/test_wks/.hlasmplugin/proc_grps.json", 1, proc_grps.c_str(), proc_grps.size());
    ws_mngr.did_open_file("test/library/test_wks/file_1", 1, f1.c_str(), f1.size());
    EXPECT_FALSE(ws_mngr.idle_handler());
    EXPECT_EQ(consumer.diags.diagnostics_size(), static_cast<size_t>(0));
    ASSERT_EQ(consumer.fms.size(), static_cast<size_t>(1));
    EXPECT_EQ(std::string(consumer.fms.message(0).file_uri()), "test/library/test_wks/file_1");
    EXPECT_EQ(consumer.fms.message(0).get_range(), range(position(6, 9), position(6, 18)));

    std::vector<document_change> changes;
    ws_mngr.did_change_file("test/library/test_wks/file_1", 2, changes.data(), 0);
    EXPECT_FALSE(ws_mngr.idle_handler());
    EXPECT_EQ(consumer.diags.diagnostics_size(), static_cast<size_t>(0));
    ASSERT_EQ(consumer.fms.size(), static_cast<size_t>(1));
    EXPECT_EQ(std::string(consumer.fms.message(0).file_uri()), "test/library/test_wks/file_1");
    EXPECT_EQ(consumer.fms.message(0).get_range(), range(position(6, 9), position(6, 18)));

    std::string new_f1_text = "A         EXEC  CICS   ABEND ABCODE('1234')\n";
    changes.push_back(document_change({ { 6, 0 }, { 6, 43 } }, new_f1_text.c_str(), new_f1_text.size()));
    ws_mngr.did_change_file("test/library/test_wks/file_1", 3, changes.data(), 1);
    EXPECT_FALSE(ws_mngr.idle_handler());
    EXPECT_EQ(consumer.diags.diagnostics_size(), static_cast<size_t>(0));
    ASSERT_EQ(consumer.fms.size(), static_cast<size_t>(1));
    EXPECT_EQ(std::string(consumer.fms.message(0).file_uri()), "test/library/test_wks/file_1");
    EXPECT_EQ(consumer.fms.message(0).get_range(), range(position(6, 10), position(6, 20)));

    std::string f2 = "";
    ws_mngr.did_open_file("test/library/test_wks/diff_file_2", 1, f2.c_str(), f2.size());
    EXPECT_FALSE(ws_mngr.idle_handler());
    EXPECT_EQ(consumer.diags.diagnostics_size(), static_cast<size_t>(0));
    ASSERT_EQ(consumer.fms.size(), static_cast<size_t>(1));
    EXPECT_EQ(std::string(consumer.fms.message(0).file_uri()), "test/library/test_wks/file_1");
    EXPECT_EQ(consumer.fms.message(0).get_range(), range(position(6, 10), position(6, 20)));

    new_f1_text = "*A         EXEC  CICS   ABEND ABCODE('1234')\n";
    changes.clear();
    changes.push_back(document_change({ { 6, 0 }, { 6, 44 } }, new_f1_text.c_str(), new_f1_text.size()));
    ws_mngr.did_change_file("test/library/test_wks/file_1", 4, changes.data(), 1);
    EXPECT_FALSE(ws_mngr.idle_handler());
    EXPECT_GE(consumer.diags.diagnostics_size(), static_cast<size_t>(0));
    EXPECT_EQ(consumer.fms.size(), static_cast<size_t>(0));

    new_f1_text = R"(         
         MACRO
         DFHECALL
         MEND

         DFHECALL
         END)";
    changes.clear();
    changes.push_back(document_change(new_f1_text.c_str(), new_f1_text.size()));
    ws_mngr.did_change_file("test/library/test_wks/file_1", 5, changes.data(), 1);
    EXPECT_FALSE(ws_mngr.idle_handler());
    EXPECT_EQ(consumer.diags.diagnostics_size(), static_cast<size_t>(0));
    EXPECT_EQ(consumer.fms.size(), static_cast<size_t>(0));
}
