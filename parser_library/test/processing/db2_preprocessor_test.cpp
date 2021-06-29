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

#include "../common_testing.h"
#include "preprocessor_options.h"
#include "processing/preprocessor.h"

// test db2 preprocessor emulator

using namespace hlasm_plugin::parser_library::processing;

TEST(db2_preprocessor, first_line)
{
    auto p = preprocessor::create(db2_preprocessor_options {}, [](std::string_view) { return std::nullopt; }, {});
    std::string_view text = "";
    std::deque<std::string> buffer;

    EXPECT_TRUE(p->fill_buffer(text, 0, buffer));
    EXPECT_GT(buffer.size(), 0);

    EXPECT_TRUE(std::any_of(
        buffer.begin(), buffer.end(), [](const std::string& s) { return s.find("SQLSECT") != std::string::npos; }));

    EXPECT_FALSE(p->fill_buffer(text, 1, buffer));
}

TEST(db2_preprocessor, last_line)
{
    auto p = preprocessor::create(db2_preprocessor_options {}, [](std::string_view) { return std::nullopt; }, {});
    std::string_view text = "\n END ";
    std::deque<std::string> buffer;

    EXPECT_TRUE(p->fill_buffer(text, 0, buffer));
    buffer.clear();
    text.remove_prefix(1);

    EXPECT_TRUE(p->fill_buffer(text, 1, buffer));
    EXPECT_GT(buffer.size(), 0);

    EXPECT_TRUE(std::any_of(
        buffer.begin(), buffer.end(), [](const std::string& s) { return s.find("***$$$ SQL WORKING STORAGE") == 0; }));

    EXPECT_FALSE(p->fill_buffer(text, 2, buffer));
}

TEST(db2_preprocessor, include)
{
    auto p = preprocessor::create(db2_preprocessor_options {},
        [](std::string_view s) {
            EXPECT_EQ(s, "MEMBER");
            return "member content";
        },
        {});
    std::string_view text = "\n EXEC SQL INCLUDE MEMBER ";
    std::deque<std::string> buffer;

    EXPECT_TRUE(p->fill_buffer(text, 0, buffer));
    buffer.clear();
    text.remove_prefix(1);

    EXPECT_TRUE(p->fill_buffer(text, 1, buffer));
    EXPECT_GT(buffer.size(), 0);

    EXPECT_TRUE(std::any_of(buffer.begin(), buffer.end(), [](const std::string& s) { return s == "member content"; }));

    EXPECT_FALSE(p->fill_buffer(text, 2, buffer));
}

TEST(db2_preprocessor, include_sqlca)
{
    bool called = false;
    auto p = preprocessor::create(db2_preprocessor_options {},
        [&called](std::string_view s) {
            called = true;
            return std::nullopt;
        },
        {});
    std::string_view text = "\n EXEC SQL INCLUDE SQLCA ";
    std::deque<std::string> buffer;

    EXPECT_TRUE(p->fill_buffer(text, 0, buffer));
    buffer.clear();
    text.remove_prefix(1);

    EXPECT_TRUE(p->fill_buffer(text, 1, buffer));
    EXPECT_GT(buffer.size(), 0);

    EXPECT_FALSE(called);
    EXPECT_TRUE(
        std::any_of(buffer.begin(), buffer.end(), [](const std::string& s) { return s.find("***$$$ SQLCA") == 0; }));

    EXPECT_FALSE(p->fill_buffer(text, 2, buffer));
}

TEST(db2_preprocessor, include_sqlda)
{
    bool called = false;
    auto p = preprocessor::create(db2_preprocessor_options {},
        [&called](std::string_view s) {
            called = true;
            return std::nullopt;
        },
        {});
    std::string_view text = "\n EXEC SQL INCLUDE SQLDA ";
    std::deque<std::string> buffer;

    EXPECT_TRUE(p->fill_buffer(text, 0, buffer));
    buffer.clear();
    text.remove_prefix(1);

    EXPECT_TRUE(p->fill_buffer(text, 1, buffer));
    EXPECT_GT(buffer.size(), 0);

    EXPECT_FALSE(called);
    EXPECT_TRUE(
        std::any_of(buffer.begin(), buffer.end(), [](const std::string& s) { return s.find("***$$$ SQLDA") == 0; }));

    EXPECT_FALSE(p->fill_buffer(text, 2, buffer));
}

TEST(db2_preprocessor, sql_like)
{
    bool called = false;
    auto p = preprocessor::create(db2_preprocessor_options {},
        [&called](std::string_view s) {
            called = true;
            return std::nullopt;
        },
        {});
    std::string_view text = "\n EXEC SQL SELECT 1 INTO :A FROM SYSIBM.SYSDUMMY1";
    std::deque<std::string> buffer;

    EXPECT_TRUE(p->fill_buffer(text, 0, buffer));
    buffer.clear();
    text.remove_prefix(1);

    EXPECT_TRUE(p->fill_buffer(text, 1, buffer));
    ASSERT_GE(buffer.size(), 2);

    EXPECT_FALSE(called);
    EXPECT_EQ(buffer[0], "***$$$");
    EXPECT_EQ(buffer[1], "*EXEC SQL SELECT 1 INTO :A FROM SYSIBM.SYSDUMMY1");

    EXPECT_FALSE(p->fill_buffer(text, 2, buffer));
}

TEST(db2_preprocessor, with_label)
{
    auto p = preprocessor::create(db2_preprocessor_options {}, [](std::string_view s) { return std::nullopt; }, {});
    std::string_view text = "\nABC EXEC SQL WHATEVER";
    std::deque<std::string> buffer;

    EXPECT_TRUE(p->fill_buffer(text, 0, buffer));
    buffer.clear();
    text.remove_prefix(1);

    EXPECT_TRUE(p->fill_buffer(text, 1, buffer));
    ASSERT_GE(buffer.size(), 3);

    EXPECT_EQ(buffer[0], "ABC DS 0H");
    EXPECT_EQ(buffer[1], "***$$$");
    EXPECT_EQ(buffer[2], "*   EXEC SQL WHATEVER");

    EXPECT_FALSE(p->fill_buffer(text, 2, buffer));
}

TEST(db2_preprocessor, missing_member)
{
    bool called = false;
    auto p = preprocessor::create(
        db2_preprocessor_options {},
        [](std::string_view s) { return std::nullopt; },
        [&called](diagnostic_op d) {
            called = true;
            EXPECT_EQ(d.code, "P0002");
        });

    std::string_view text = " EXEC SQL INCLUDE MISSING";
    std::deque<std::string> buffer;

    EXPECT_TRUE(p->fill_buffer(text, 0, buffer));
    EXPECT_TRUE(called);
}

TEST(db2_preprocessor, bad_continuation)
{
    bool called = false;
    auto p = preprocessor::create(
        db2_preprocessor_options {},
        [](std::string_view s) { return std::nullopt; },
        [&called](diagnostic_op d) {
            called = true;
            EXPECT_EQ(d.code, "P0001");
        });

    std::string_view text = R"( EXEC SQL PRETENT SQL STATEMENT                                        X
badcontinuation)";
    std::deque<std::string> buffer;

    EXPECT_TRUE(p->fill_buffer(text, 0, buffer));
    EXPECT_TRUE(called);
}

TEST(db2_preprocessor, no_nested_include)
{
    bool called = false;
    auto p = preprocessor::create(
        db2_preprocessor_options {},
        [](std::string_view s) {
            EXPECT_EQ(s, "MEMBER");
            return " EXEC SQL INCLUDE MEMBER";
        },
        [&called](diagnostic_op d) {
            called = true;
            EXPECT_EQ(d.code, "P0003");
        });
    std::string_view text = " EXEC SQL INCLUDE MEMBER ";
    std::deque<std::string> buffer;

    EXPECT_TRUE(p->fill_buffer(text, 0, buffer));
    EXPECT_TRUE(called);
}
