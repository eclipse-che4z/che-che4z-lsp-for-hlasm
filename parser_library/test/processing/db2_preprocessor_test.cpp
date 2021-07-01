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

#include <unordered_map>

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
    size_t lineno = 0;

    EXPECT_TRUE(p->fill_buffer(text, lineno, buffer));
    EXPECT_GT(buffer.size(), 0);
    EXPECT_EQ(lineno, 0);

    EXPECT_TRUE(std::any_of(
        buffer.begin(), buffer.end(), [](const std::string& s) { return s.find("SQLSECT") != std::string::npos; }));

    EXPECT_FALSE(p->fill_buffer(text, lineno, buffer));
    EXPECT_EQ(lineno, 0);
}

TEST(db2_preprocessor, last_line)
{
    auto p = preprocessor::create(db2_preprocessor_options {}, [](std::string_view) { return std::nullopt; }, {});
    std::string_view text = "\n END ";
    std::deque<std::string> buffer;
    size_t lineno = 0;

    EXPECT_TRUE(p->fill_buffer(text, lineno, buffer));
    EXPECT_EQ(lineno, 0);
    buffer.clear();
    text.remove_prefix(1);
    ++lineno;

    std::string_view original_text = text;
    EXPECT_TRUE(p->fill_buffer(text, lineno, buffer));
    EXPECT_EQ(lineno, 1);
    EXPECT_GT(buffer.size(), 0);
    EXPECT_EQ(original_text, text); // END should remain in the text

    EXPECT_TRUE(std::any_of(
        buffer.begin(), buffer.end(), [](const std::string& s) { return s.find("***$$$ SQL WORKING STORAGE") == 0; }));

    buffer.clear();
    EXPECT_FALSE(p->fill_buffer(text, lineno, buffer)); // but should not be processed again
    EXPECT_EQ(lineno, 1);
    EXPECT_EQ(buffer.size(), 0);
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
    size_t lineno = 0;

    EXPECT_TRUE(p->fill_buffer(text, lineno, buffer));
    EXPECT_EQ(lineno, 0);
    buffer.clear();
    text.remove_prefix(1);
    ++lineno;

    std::string_view original_text = text;
    EXPECT_TRUE(p->fill_buffer(text, lineno, buffer));
    EXPECT_NE(original_text, text); // INCLUDE should be removed
    EXPECT_EQ(lineno, 2);
    EXPECT_GT(buffer.size(), 0);

    EXPECT_TRUE(std::any_of(buffer.begin(), buffer.end(), [](const std::string& s) { return s == "member content"; }));

    EXPECT_FALSE(p->fill_buffer(text, lineno, buffer));
    EXPECT_EQ(lineno, 2);
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
    size_t lineno = 0;

    EXPECT_TRUE(p->fill_buffer(text, lineno, buffer));
    EXPECT_EQ(lineno, 0);
    buffer.clear();
    text.remove_prefix(1);
    ++lineno;

    std::string_view original_text = text;
    EXPECT_TRUE(p->fill_buffer(text, lineno, buffer));
    EXPECT_EQ(lineno, 2);
    EXPECT_NE(original_text, text); // INCLUDE should be removed
    EXPECT_GT(buffer.size(), 0);

    EXPECT_FALSE(called);
    EXPECT_TRUE(
        std::any_of(buffer.begin(), buffer.end(), [](const std::string& s) { return s.find("***$$$ SQLCA") == 0; }));

    EXPECT_FALSE(p->fill_buffer(text, lineno, buffer));
    EXPECT_EQ(lineno, 2);
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
    size_t lineno = 0;

    EXPECT_TRUE(p->fill_buffer(text, lineno, buffer));
    EXPECT_EQ(lineno, 0);
    buffer.clear();
    text.remove_prefix(1);
    ++lineno;

    std::string_view original_text = text;
    EXPECT_TRUE(p->fill_buffer(text, lineno, buffer));
    EXPECT_EQ(lineno, 2);
    EXPECT_NE(original_text, text); // INCLUDE should be removed
    EXPECT_GT(buffer.size(), 0);

    EXPECT_FALSE(called);
    EXPECT_TRUE(
        std::any_of(buffer.begin(), buffer.end(), [](const std::string& s) { return s.find("***$$$ SQLDA") == 0; }));

    EXPECT_FALSE(p->fill_buffer(text, lineno, buffer));
    EXPECT_EQ(lineno, 2);
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
    size_t lineno = 0;

    EXPECT_TRUE(p->fill_buffer(text, lineno, buffer));
    EXPECT_EQ(lineno, 0);
    buffer.clear();
    text.remove_prefix(1);
    ++lineno;

    std::string_view original_text = text;
    EXPECT_TRUE(p->fill_buffer(text, lineno, buffer));
    EXPECT_EQ(lineno, 2);
    EXPECT_NE(original_text, text); // SQL should be removed
    ASSERT_GE(buffer.size(), 2);

    EXPECT_FALSE(called);
    EXPECT_EQ(buffer[0], "***$$$");
    EXPECT_EQ(buffer[1], "*EXEC SQL SELECT 1 INTO :A FROM SYSIBM.SYSDUMMY1");

    EXPECT_FALSE(p->fill_buffer(text, lineno, buffer));
    EXPECT_EQ(lineno, 2);
}

TEST(db2_preprocessor, with_label)
{
    auto p = preprocessor::create(db2_preprocessor_options {}, [](std::string_view s) { return std::nullopt; }, {});
    std::string_view text = "\nABC EXEC SQL WHATEVER";
    std::deque<std::string> buffer;
    size_t lineno = 0;

    EXPECT_TRUE(p->fill_buffer(text, lineno, buffer));
    EXPECT_EQ(lineno, 0);
    buffer.clear();
    text.remove_prefix(1);
    ++lineno;

    EXPECT_TRUE(p->fill_buffer(text, lineno, buffer));
    EXPECT_EQ(lineno, 2);
    ASSERT_GE(buffer.size(), 3);

    EXPECT_EQ(buffer[0], "ABC DS 0H");
    EXPECT_EQ(buffer[1], "***$$$");
    EXPECT_EQ(buffer[2], "*   EXEC SQL WHATEVER");

    EXPECT_FALSE(p->fill_buffer(text, lineno, buffer));
    EXPECT_EQ(lineno, 2);
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
    size_t lineno = 0;

    EXPECT_TRUE(p->fill_buffer(text, lineno, buffer));
    EXPECT_EQ(lineno, 1);
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
    size_t lineno = 0;

    EXPECT_TRUE(p->fill_buffer(text, lineno, buffer));
    EXPECT_EQ(lineno, 2);
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
    size_t lineno = 0;

    EXPECT_TRUE(p->fill_buffer(text, lineno, buffer));
    EXPECT_EQ(lineno, 1);
    EXPECT_TRUE(called);
}

TEST(db2_preprocessor, sqlsect_available)
{
    std::string input = R"(
 SQLSECT SAVE
 SQLSECT RESTORE
)";

    analyzer a(input, analyzer_options { db2_preprocessor_options {} });
    a.analyze();
    a.collect_diags();

    EXPECT_EQ(a.diags().size(), (size_t)0);
}

TEST(db2_preprocessor, aread_from_preprocessor)
{
    std::string input = R"(
         GBLC   &RES
         MACRO
         MAC
         GBLC   &RES
&RES     AREAD
         MEND
*
         MAC
         EXEC SQL SELECT 1 FROM SYSIBM.SYSDUMMY1
)";

    analyzer a(input, analyzer_options { db2_preprocessor_options {} });
    a.analyze();
    a.collect_diags();

    EXPECT_EQ(a.diags().size(), (size_t)0);
    EXPECT_EQ(get_var_value<std::string>(a.hlasm_ctx(), "RES"), std::string("***$$$").append(74, ' '));
}

TEST(db2_preprocessor, ignore_comments)
{
    std::string input = R"(
         GBLC   &RES
         MACRO
         MAC
         GBLC   &RES
&RES     AREAD
         MEND
*
         MAC
*        EXEC SQL SELECT 1 FROM SYSIBM.SYSDUMMY1
)";

    analyzer a(input, analyzer_options { db2_preprocessor_options {} });
    a.analyze();
    a.collect_diags();

    EXPECT_EQ(a.diags().size(), (size_t)0);

    const auto RES = get_var_value<std::string>(a.hlasm_ctx(), "RES");

    ASSERT_TRUE(RES.has_value());
    EXPECT_EQ(RES.value().find("*        EXEC SQL SELECT 1 FROM SYSIBM.SYSDUMMY1"), 0);
}

namespace {
class library_provider_with_uri final : public parse_lib_provider
{
    std::unordered_map<std::string, std::string> m_files;

    const std::pair<const std::string, std::string>* find_file(const std::string& f) const
    {
        if (auto it = m_files.find(f); it != m_files.end())
            return &*it;
        return nullptr;
    }

public:
    parse_result parse_library(const std::string& l, analyzing_context, library_data) override
    {
        assert(false);
        return false;
    };
    bool has_library(const std::string& l, const std::string&) const override { return find_file(l) != nullptr; };
    std::optional<std::string> get_library(const std::string& l, const std::string&, std::string* uri) const override
    {
        const auto* f = find_file(l);
        if (!f)
            return std::nullopt;

        if (uri)
            *uri = f->first;

        return f->second;
    }

    library_provider_with_uri(std::initializer_list<std::pair<std::string, std::string>> files)
        : m_files(files.begin(), files.end())
    {}
};
} // namespace

TEST(db2_preprocessor, continuation_in_buffer)
{
    library_provider_with_uri libs({
        { "MEMBER", R"(
&A SETA 1                                                              X
               comment to be ignored
)" },
    });
    std::string input = " EXEC SQL INCLUDE MEMBER ";

    analyzer a(input, analyzer_options { &libs, db2_preprocessor_options {} });
    a.analyze();
    a.collect_diags();

    EXPECT_EQ(a.diags().size(), (size_t)0);

    EXPECT_EQ(get_var_value<A_t>(a.hlasm_ctx(), "A"), 1);
    EXPECT_TRUE(a.hlasm_ctx().get_visited_files().count("MEMBER"));
}

TEST(db2_preprocessor, include_empty)
{
    library_provider_with_uri libs({
        { "MEMBER", "" },
    });
    std::string input = " EXEC SQL INCLUDE MEMBER ";

    analyzer a(input, analyzer_options { &libs, db2_preprocessor_options {} });
    a.analyze();
    a.collect_diags();

    EXPECT_EQ(a.diags().size(), (size_t)0);

    EXPECT_TRUE(a.hlasm_ctx().get_visited_files().count("MEMBER"));
}

TEST(db2_preprocessor, include_nonexistent)
{
    std::string input = " EXEC SQL INCLUDE MEMBER ";

    analyzer a(input, analyzer_options { db2_preprocessor_options {} });
    a.analyze();
    a.collect_diags();

    EXPECT_EQ(a.diags().size(), (size_t)1);
}

TEST(db2_preprocessor, ago_in_include)
{
    library_provider_with_uri libs({
        { "MEMBER", R"(
        AGO  .HERE
.HERE   ANOP
)" },
    });
    std::string input = " EXEC SQL INCLUDE MEMBER ";

    analyzer a(input, analyzer_options { &libs, db2_preprocessor_options {} });
    a.analyze();
    a.collect_diags();

    EXPECT_EQ(a.diags().size(), (size_t)0);

    EXPECT_TRUE(a.hlasm_ctx().get_visited_files().count("MEMBER"));
}
