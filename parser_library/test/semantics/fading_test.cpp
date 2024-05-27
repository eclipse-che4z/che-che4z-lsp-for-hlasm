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
#include <memory>
#include <vector>

#include "gtest/gtest.h"

#include "analyzer.h"
#include "fade_messages.h"
#include "preprocessor_options.h"

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::semantics;
using namespace hlasm_plugin::utils::resource;

namespace {
const auto source_file_loc = resource_location("file_name");
}

TEST(fading, endevor)
{
    const std::string contents = R"(
-INC  MEMBER bla bla
++INCLUDE MEM)";

    std::vector<fade_message> expected_fmsg {
        fade_message::preprocessor_statement("file_name", range(position(1, 0), position(1, 4))),
        fade_message::preprocessor_statement("file_name", range(position(2, 0), position(2, 9))),
    };

    auto fms = std::make_shared<std::vector<fade_message>>();

    analyzer a(contents, analyzer_options { source_file_loc, endevor_preprocessor_options(), fms });
    a.analyze();


    EXPECT_TRUE(std::ranges::is_permutation(*fms, expected_fmsg, [](const auto& fmsg, const auto& expected_fmsg) {
        return fmsg.code == expected_fmsg.code && fmsg.r == expected_fmsg.r && fmsg.uri == expected_fmsg.uri;
    }));
}

TEST(fading, cics)
{
    const std::string contents = R"(
A   EXEC CICS ABEND ABCODE('1234')                                      00000000
    EXEC CICS ABEND                                                    X
                 ABCODE('1234')
 EXEC CICS                                                             X12345678
    ABEND ABCODE('1234')

B   L 0,DFHRESP ( NORMAL ) bla                                         X00000002
               bla                                                     XYZ
               bla
                                                                       X00000004
               L 1,DFHRESP(NORMAL))";

    std::vector<fade_message> expected_fmsg {
        fade_message::preprocessor_statement("file_name", range(position(1, 4), position(1, 13))),
        fade_message::preprocessor_statement("file_name", range(position(2, 4), position(2, 13))),
        fade_message::preprocessor_statement("file_name", range(position(4, 1), position(4, 10))),
    };

    auto fms = std::make_shared<std::vector<fade_message>>();

    analyzer a(contents, analyzer_options { source_file_loc, cics_preprocessor_options(), fms });
    a.analyze();

    EXPECT_TRUE(std::ranges::is_permutation(*fms, expected_fmsg, [](const auto& fmsg, const auto& expected_fmsg) {
        return fmsg.code == expected_fmsg.code && fmsg.r == expected_fmsg.r && fmsg.uri == expected_fmsg.uri;
    }));
}

TEST(fading, db2_preprocessor_statement_include)
{
    const std::string contents = R"(
AAA  EXEC  SQL   INCLUDE  SQLCA -- REMARK                               00000001
                                                         EXEC SQL --REMX00000020
               INCLUDE SQLCA         -- rem  rem2                       00000300
                  EXEC      SQL                                        X00004000
               SELECT                                                  X
               1       --rem                                           X00050000
                   INTO :B                                             X
               FROM                                                    X
               SYSIBM.SYSDUMMY1

B SQL  TYPE   IS RESULT_SET_LOCATOR VARYING   comment comment2          00000060
C   SQL TYPE                                                           X00000700
                  IS RESULT_SET_LOCATOR VARYING   comment comment2)";

    std::vector<fade_message> expected_fmsg {
        fade_message::preprocessor_statement("file_name", range(position(1, 5), position(1, 14))),
        fade_message::preprocessor_statement("file_name", range(position(2, 57), position(2, 65))),
        fade_message::preprocessor_statement("file_name", range(position(4, 18), position(4, 31))),
        fade_message::preprocessor_statement("file_name", range(position(11, 2), position(11, 11))),
        fade_message::preprocessor_statement("file_name", range(position(12, 4), position(12, 12))),
    };

    auto fms = std::make_shared<std::vector<fade_message>>();

    analyzer a(contents, analyzer_options { source_file_loc, db2_preprocessor_options(), fms });
    a.analyze();

    EXPECT_TRUE(std::ranges::is_permutation(*fms, expected_fmsg, [](const auto& fmsg, const auto& expected_fmsg) {
        return fmsg.code == expected_fmsg.code && fmsg.r == expected_fmsg.r && fmsg.uri == expected_fmsg.uri;
    }));
}
