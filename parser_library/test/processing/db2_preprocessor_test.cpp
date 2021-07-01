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
    size_t lineno = 0;

    auto result = p->generate_replacement(text, lineno);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(lineno, 0);

    EXPECT_NE(result.value().find("SQLSECT"), std::string::npos);

    EXPECT_FALSE(p->generate_replacement(text, lineno).has_value());
    EXPECT_EQ(lineno, 0);
}

TEST(db2_preprocessor, last_line)
{
    auto p = preprocessor::create(db2_preprocessor_options {}, [](std::string_view) { return std::nullopt; }, {});
    std::string_view text = "\n END ";
    size_t lineno = 0;

    EXPECT_TRUE(p->generate_replacement(text, lineno).has_value());
    EXPECT_EQ(lineno, 0);
    text.remove_prefix(1);
    ++lineno;

    std::string_view original_text = text;
    auto result = p->generate_replacement(text, lineno);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(lineno, 1);
    EXPECT_EQ(original_text, text); // END should remain in the text

    EXPECT_EQ(result.value().find("***$$$ SQL WORKING STORAGE"), 0);

    EXPECT_FALSE(p->generate_replacement(text, lineno).has_value()); // but should not be processed again
    EXPECT_EQ(lineno, 1);
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
    size_t lineno = 0;

    EXPECT_TRUE(p->generate_replacement(text, lineno).has_value());
    EXPECT_EQ(lineno, 0);
    text.remove_prefix(1);
    ++lineno;

    std::string_view original_text = text;
    auto result = p->generate_replacement(text, lineno);
    ASSERT_TRUE(result.has_value());
    EXPECT_NE(original_text, text); // INCLUDE should be removed
    EXPECT_EQ(lineno, 2);

    EXPECT_NE(result.value().find("member content\n"), std::string::npos);

    EXPECT_FALSE(p->generate_replacement(text, lineno).has_value());
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
    size_t lineno = 0;

    EXPECT_TRUE(p->generate_replacement(text, lineno).has_value());
    EXPECT_EQ(lineno, 0);
    text.remove_prefix(1);
    ++lineno;

    std::string_view original_text = text;
    auto result = p->generate_replacement(text, lineno);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(lineno, 2);
    EXPECT_NE(original_text, text); // INCLUDE should be removed

    EXPECT_FALSE(called);
    EXPECT_NE(result.value().find("***$$$ SQLCA"), std::string::npos);

    EXPECT_FALSE(p->generate_replacement(text, lineno).has_value());
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
    size_t lineno = 0;

    EXPECT_TRUE(p->generate_replacement(text, lineno).has_value());
    EXPECT_EQ(lineno, 0);
    text.remove_prefix(1);
    ++lineno;

    std::string_view original_text = text;
    auto result = p->generate_replacement(text, lineno);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(lineno, 2);
    EXPECT_NE(original_text, text); // INCLUDE should be removed

    EXPECT_FALSE(called);
    EXPECT_NE(result.value().find("***$$$ SQLDA"), std::string::npos);

    EXPECT_FALSE(p->generate_replacement(text, lineno).has_value());
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
    size_t lineno = 0;

    EXPECT_TRUE(p->generate_replacement(text, lineno).has_value());
    EXPECT_EQ(lineno, 0);
    text.remove_prefix(1);
    ++lineno;

    std::string_view original_text = text;
    auto result = p->generate_replacement(text, lineno);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(lineno, 2);
    EXPECT_NE(original_text, text); // SQL should be removed

    EXPECT_FALSE(called);
    EXPECT_EQ(result.value().find("***$$$\n*EXEC SQL SELECT 1 INTO :A FROM SYSIBM.SYSDUMMY1\n"), 0);

    EXPECT_FALSE(p->generate_replacement(text, lineno).has_value());
    EXPECT_EQ(lineno, 2);
}

TEST(db2_preprocessor, with_label)
{
    auto p = preprocessor::create(db2_preprocessor_options {}, [](std::string_view s) { return std::nullopt; }, {});
    std::string_view text = "\nABC EXEC SQL WHATEVER";
    size_t lineno = 0;

    EXPECT_TRUE(p->generate_replacement(text, lineno).has_value());
    EXPECT_EQ(lineno, 0);
    text.remove_prefix(1);
    ++lineno;

    auto result = p->generate_replacement(text, lineno);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(lineno, 2);

    EXPECT_EQ(result.value().find("ABC DS 0H\n***$$$\n*   EXEC SQL WHATEVER"), 0);

    EXPECT_FALSE(p->generate_replacement(text, lineno).has_value());
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

    EXPECT_TRUE(p->generate_replacement(text, lineno));
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

    EXPECT_TRUE(p->generate_replacement(text, lineno));
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

    EXPECT_TRUE(p->generate_replacement(text, lineno));
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

TEST(db2_preprocessor, aread_from_two_preprocessor_outputs)
{
    std::string input = R"(
         GBLC   &RES0,&RES1,&RES2,&RES3,&RES4,&RES5
         MACRO
         MAC
         GBLC   &RES0,&RES1,&RES2,&RES3,&RES4,&RES5
&RES0    AREAD
&RES1    AREAD
&RES2    AREAD
&RES3    AREAD
&RES4    AREAD
&RES5    AREAD
         MEND
*
         MAC
         EXEC SQL SELECT 1 FROM SYSIBM.SYSDUMMY1
         EXEC SQL SELECT 2 FROM SYSIBM.SYSDUMMY1
)";

    analyzer a(input, analyzer_options { db2_preprocessor_options {} });
    a.analyze();
    a.collect_diags();

    std::array expected {
        std::string { "***$$$" },
        std::string { "*        EXEC SQL SELECT 1 FROM SYSIBM.SYSDUMMY1" },
        std::string { "***$$$" },
        std::string { "***$$$" },
        std::string { "*        EXEC SQL SELECT 2 FROM SYSIBM.SYSDUMMY1" },
        std::string { "***$$$" },
    };
    for (auto& s : expected)
        s.resize(80, ' ');

    EXPECT_EQ(a.diags().size(), (size_t)0);
    EXPECT_EQ(get_var_value<std::string>(a.hlasm_ctx(), "RES0"), expected[0]);
    EXPECT_EQ(get_var_value<std::string>(a.hlasm_ctx(), "RES1"), expected[1]);
    EXPECT_EQ(get_var_value<std::string>(a.hlasm_ctx(), "RES2"), expected[2]);
    EXPECT_EQ(get_var_value<std::string>(a.hlasm_ctx(), "RES3"), expected[3]);
    EXPECT_EQ(get_var_value<std::string>(a.hlasm_ctx(), "RES4"), expected[4]);
    EXPECT_EQ(get_var_value<std::string>(a.hlasm_ctx(), "RES5"), expected[5]);
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
    parse_result parse_library(const std::string& l, analyzing_context ctx, library_data data) override
    {
        const auto* f = find_file(l);
        if (!f)
            return false;

        analyzer a(f->second, analyzer_options { l, this, std::move(ctx), data });
        a.analyze();
        return true;
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

    template<typename T>
    library_provider_with_uri(T&& c)
        : m_files(c.begin(), c.end())
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
&A      SETA 1
)" },
    });
    std::string input = " EXEC SQL INCLUDE MEMBER ";

    analyzer a(input, analyzer_options { &libs, db2_preprocessor_options {} });
    a.analyze();
    a.collect_diags();

    EXPECT_EQ(a.diags().size(), (size_t)0);

    EXPECT_TRUE(a.hlasm_ctx().get_visited_files().count("MEMBER"));
    EXPECT_EQ(get_var_value<A_t>(a.hlasm_ctx(), "A"), 1);
}

TEST(db2_preprocessor, ago_into_include)
{
    library_provider_with_uri libs({
        { "MEMBER", R"(
.HERE   ANOP
&A      SETA 1
)" },
    });
    std::string input = R"(
 AGO .HERE
 EXEC SQL INCLUDE MEMBER
&B      SETA 2
)";

    analyzer a(input, analyzer_options { &libs, db2_preprocessor_options {} });
    a.analyze();
    a.collect_diags();

    EXPECT_EQ(a.diags().size(), (size_t)0);

    EXPECT_TRUE(a.hlasm_ctx().get_visited_files().count("MEMBER"));
    EXPECT_EQ(get_var_value<A_t>(a.hlasm_ctx(), "A"), 1);
    EXPECT_EQ(get_var_value<A_t>(a.hlasm_ctx(), "B"), 2);
}

TEST(db2_preprocessor, ago_from_include)
{
    library_provider_with_uri libs({
        { "MEMBER", R"(
&A      SETA 1
 AGO .HERE
)" },
    });
    std::string input = R"(
 EXEC SQL INCLUDE MEMBER
.HERE   ANOP
&B      SETA 2
)";

    analyzer a(input, analyzer_options { &libs, db2_preprocessor_options {} });
    a.analyze();
    a.collect_diags();

    EXPECT_EQ(a.diags().size(), (size_t)0);

    EXPECT_TRUE(a.hlasm_ctx().get_visited_files().count("MEMBER"));
    EXPECT_EQ(get_var_value<A_t>(a.hlasm_ctx(), "A"), 1);
    EXPECT_EQ(get_var_value<A_t>(a.hlasm_ctx(), "B"), 2);
}

TEST(db2_preprocessor, ago_around_include)
{
    library_provider_with_uri libs({
        { "MEMBER", R"(
&A      SETA &A+1
)" },
    });
    std::string input = R"(
        LCLA  &A
        LCLB  &STOP
.HERE   ANOP
        EXEC  SQL INCLUDE MEMBER
        AIF   (&STOP).END
&STOP   SETB  1
        AGO   .HERE
.END    ANOP
)";

    analyzer a(input, analyzer_options { &libs, db2_preprocessor_options {} });
    a.analyze();
    a.collect_diags();

    EXPECT_EQ(a.diags().size(), (size_t)0);

    EXPECT_TRUE(a.hlasm_ctx().get_visited_files().count("MEMBER"));
    EXPECT_EQ(get_var_value<A_t>(a.hlasm_ctx(), "A"), 2);
}

TEST(db2_preprocessor, copy_in_include)
{
    library_provider_with_uri libs({
        { "COPY1", "&A1 SETA 1" },
        { "COPY2", "&A2 SETA 2" },
        { "MEMBER", R"(
        COPY COPY1
&A3     SETA 3
        COPY COPY2
)" },
    });
    std::string input = R"(
&A4     SETA 4
        EXEC  SQL INCLUDE MEMBER
&A5     SETA 5
)";

    analyzer a(input, analyzer_options { &libs, db2_preprocessor_options {} });
    a.analyze();
    a.collect_diags();

    EXPECT_EQ(a.diags().size(), (size_t)0);

    EXPECT_TRUE(a.hlasm_ctx().get_visited_files().count("COPY1"));
    EXPECT_TRUE(a.hlasm_ctx().get_visited_files().count("COPY2"));
    EXPECT_TRUE(a.hlasm_ctx().get_visited_files().count("MEMBER"));

    EXPECT_EQ(get_var_value<A_t>(a.hlasm_ctx(), "A1"), 1);
    EXPECT_EQ(get_var_value<A_t>(a.hlasm_ctx(), "A2"), 2);
    EXPECT_EQ(get_var_value<A_t>(a.hlasm_ctx(), "A3"), 3);
    EXPECT_EQ(get_var_value<A_t>(a.hlasm_ctx(), "A4"), 4);
    EXPECT_EQ(get_var_value<A_t>(a.hlasm_ctx(), "A5"), 5);
}

TEST(db2_preprocessor, check_diagnostic_lines)
{
    struct testcase
    {
        size_t lineno;
        std::string opencode;
        std::vector<std::pair<std::string, std::string>> deps;
    };
    const auto testcases = {
        testcase { 0, "A", {} },
        testcase {
            3 + 1, // keep in mind ***$$$, ...
            R"(
    EXEC  SQL INCLUDE MEMBER
)",
            {
                { "MEMBER", "\nA" },
            },
        },
        testcase {
            2,
            R"(
    EXEC  SQL INCLUDE MEMBER
A
)",
            {
                { "MEMBER", "B DS 0H" },
            },
        },
        testcase {
            4,
            R"(
    AGO   .HERE
    EXEC  SQL INCLUDE MEMBER
.HERE     ANOP ,
A
)",
            {
                { "MEMBER", "A DS 0H" },
            },
        },
        testcase {
            3,
            R"(
    EXEC  SQL INCLUDE MEMBER
.HERE     ANOP ,
A
)",
            {
                { "MEMBER", " AGO .HERE" },
            },
        },
        testcase {
            3 + 3,
            R"(
    EXEC  SQL INCLUDE MEMBER
)",
            {
                { "MEMBER", R"(
    AGO   .HERE
.HERE ANOP
A
)" },
            },
        },
        testcase {
            3,
            R"(
    AGO   .HERE
    EXEC  SQL INCLUDE MEMBER
A
)",
            {
                { "MEMBER", ".HERE ANOP" },
            },
        },
        testcase {
            1,
            R"( EXEC  SQL INCLUDE MEMBER
A
)",
            {
                { "MEMBER", ".HERE ANOP" },
            },
        },
        testcase {
            5,
            R"(.HERE   ANOP
            AIF    (D'ONEPASS).STOP
ONEPASS     DC     0C
            AGO    .HERE
.STOP       ANOP
A
)",
            {},
        },
    };

    for (const auto& t : testcases)
    {
        library_provider_with_uri libs(t.deps);
        analyzer a(t.opencode, analyzer_options { &libs, db2_preprocessor_options {} });
        a.analyze();
        a.collect_diags();

        ASSERT_EQ(a.diags().size(), (size_t)1);
        EXPECT_EQ(a.diags()[0].diag_range.start.line, t.lineno);
    }
}
