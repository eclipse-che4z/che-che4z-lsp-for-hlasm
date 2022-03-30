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
#include "../mock_parse_lib_provider.h"
#include "preprocessor_options.h"
#include "processing/preprocessor.h"
// test db2 preprocessor emulator

using namespace hlasm_plugin::parser_library::processing;

TEST(db2_preprocessor, first_line)
{
    auto p = preprocessor::create(
        db2_preprocessor_options {}, [](std::string_view) { return std::nullopt; }, nullptr);
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
    auto p = preprocessor::create(
        db2_preprocessor_options {}, [](std::string_view) { return std::nullopt; }, nullptr);
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
    auto p = preprocessor::create(
        db2_preprocessor_options {},
        [](std::string_view s) {
            EXPECT_EQ(s, "MEMBER");
            return "member content";
        },
        nullptr);
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
    auto p = preprocessor::create(
        db2_preprocessor_options {},
        [&called](std::string_view) {
            called = true;
            return std::nullopt;
        },
        nullptr);
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
    auto p = preprocessor::create(
        db2_preprocessor_options {},
        [&called](std::string_view) {
            called = true;
            return std::nullopt;
        },
        nullptr);
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
    auto p = preprocessor::create(
        db2_preprocessor_options {},
        [&called](std::string_view) {
            called = true;
            return std::nullopt;
        },
        nullptr);
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
    auto p = preprocessor::create(
        db2_preprocessor_options {}, [](std::string_view) { return std::nullopt; }, nullptr);
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
    diagnostic_op_consumer_container diags;
    auto p = preprocessor::create(
        db2_preprocessor_options {}, [](std::string_view) { return std::nullopt; }, &diags);

    std::string_view text = " EXEC SQL INCLUDE MISSING";
    size_t lineno = 0;

    EXPECT_TRUE(p->generate_replacement(text, lineno));
    EXPECT_EQ(lineno, 1);

    ASSERT_EQ(diags.diags.size(), 1U);
    EXPECT_EQ(diags.diags[0].code, "DB002");
}

TEST(db2_preprocessor, bad_continuation)
{
    diagnostic_op_consumer_container diags;
    auto p = preprocessor::create(
        db2_preprocessor_options {}, [](std::string_view) { return std::nullopt; }, &diags);

    std::string_view text = R"( EXEC SQL PRETENT SQL STATEMENT                                        X
badcontinuation)";
    size_t lineno = 0;

    EXPECT_TRUE(p->generate_replacement(text, lineno));
    EXPECT_EQ(lineno, 2);

    ASSERT_EQ(diags.diags.size(), 1U);
    EXPECT_EQ(diags.diags[0].code, "DB001");
}

TEST(db2_preprocessor, no_nested_include)
{
    diagnostic_op_consumer_container diags;
    auto p = preprocessor::create(
        db2_preprocessor_options {},
        [](std::string_view s) {
            EXPECT_EQ(s, "MEMBER");
            return " EXEC SQL INCLUDE MEMBER";
        },
        &diags);
    std::string_view text = " EXEC SQL INCLUDE MEMBER ";
    size_t lineno = 0;

    EXPECT_TRUE(p->generate_replacement(text, lineno));
    EXPECT_EQ(lineno, 1);

    ASSERT_EQ(diags.diags.size(), 1U);
    EXPECT_EQ(diags.diags[0].code, "DB003");
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
         EXEC SQL WHEnEVEr SQLERROR GOTO ERROR_RTN
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
         EXEC SQL WHENEVER SQLERROR GOTO ERROR_RTN1
         EXEC SQL whenever SQLERROR GOTO ERROR_RTN2
)";

    analyzer a(input, analyzer_options { db2_preprocessor_options {} });
    a.analyze();
    a.collect_diags();

    std::array expected {
        std::string { "***$$$" },
        std::string { "*        EXEC SQL WHENEVER SQLERROR GOTO ERROR_RTN1" },
        std::string { "***$$$" },
        std::string { "***$$$" },
        std::string { "*        EXEC SQL whenever SQLERROR GOTO ERROR_RTN2" },
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

TEST(db2_preprocessor, continuation_in_buffer)
{
    mock_parse_lib_provider libs({
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
    mock_parse_lib_provider libs({
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
    mock_parse_lib_provider libs({
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
    mock_parse_lib_provider libs({
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
    mock_parse_lib_provider libs({
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
    mock_parse_lib_provider libs({
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
    mock_parse_lib_provider libs({
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
        mock_parse_lib_provider libs(t.deps);
        analyzer a(t.opencode, analyzer_options { &libs, db2_preprocessor_options {} });
        a.analyze();
        a.collect_diags();

        ASSERT_EQ(a.diags().size(), (size_t)1);
        EXPECT_EQ(a.diags()[0].diag_range.start.line, t.lineno);
    }
}

TEST(db2_preprocessor, multiline_exec_sql)
{
    std::string input = R"(
        EXEC  SQL                                                      X
               wHENeVER      SQLERROR GOTO ERROR_RTN
)";

    analyzer a(input, analyzer_options { db2_preprocessor_options {} });
    a.analyze();
    a.collect_diags();

    EXPECT_EQ(a.diags().size(), (size_t)0);
}

TEST(db2_preprocessor, end_sqldsect_injection)
{
    std::string input = R"( END)";

    analyzer a(input, analyzer_options { db2_preprocessor_options {} });
    a.analyze();
    a.collect_diags();

    EXPECT_EQ(a.diags().size(), (size_t)0);
}

TEST(db2_preprocessor, sql_types)
{
    diagnostic_op_consumer_container diags;
    auto p = preprocessor::create(
        db2_preprocessor_options {}, [](std::string_view) { return std::nullopt; }, &diags);
    std::string_view text = R"(
RE SQL TYPE IS RESULT_SET_LOCATOR VARYING
RO SQL TYPE IS ROWID
TU SQL TYPE IS TABLE LIKE A AS LOCATOR
TQ SQL TYPE IS TABLE LIKE 'A''B' AS LOCATOR
XB SQL TYPE IS XML AS BLOB 10
XC SQL TYPE IS XML AS CLOB 10K
XD SQL TYPE IS XML AS DBCLOB 10M
BL SQL TYPE IS BINARY LARGE OBJECT 10K
CL SQL TYPE IS CHARACTER LARGE OBJECT 10M
DL SQL TYPE IS DBCLOB 1G
BLOC SQL TYPE IS BLOB_LOCATOR
CLOC SQL TYPE IS CLOB_LOCATOR
DLOC SQL TYPE IS DBCLOB_LOCATOR
BFILE SQL TYPE IS BLOB_FILE
CFILE SQL TYPE IS CLOB_FILE
DFILE SQL TYPE IS DBCLOB_FILE
)";
    size_t lineno = 0;

    EXPECT_TRUE(p->generate_replacement(text, lineno).has_value());
    EXPECT_EQ(lineno, 0);
    text.remove_prefix(1);
    ++lineno;

    std::vector<std::string> expected = {
        R"(
***$$$
*RE SQL TYPE IS RESULT_SET_LOCATOR VARYING
***$$$
RE       DS    FL4
)",
        R"(
***$$$
*RO SQL TYPE IS ROWID
***$$$
RO       DS    H,CL40
)",
        R"(
***$$$
*TU SQL TYPE IS TABLE LIKE A AS LOCATOR
***$$$
TU       DS    FL4
)",
        R"(
***$$$
*TQ SQL TYPE IS TABLE LIKE 'A''B' AS LOCATOR
***$$$
TQ       DS    FL4
)",
        R"(
***$$$
*XB SQL TYPE IS XML AS BLOB 10
***$$$
XB       DS   0FL4
XB_LENGTH DS FL4
XB_DATA DS CL10
)",
        R"(
***$$$
*XC SQL TYPE IS XML AS CLOB 10K
***$$$
XC       DS   0FL4
XC_LENGTH DS FL4
XC_DATA DS CL10240
)",
        R"(
***$$$
*XD SQL TYPE IS XML AS DBCLOB 10M
***$$$
XD       DS   0FL4
XD_LENGTH DS FL4
XD_DATA DS GL65534
 ORG   *+(10420226)
)",
        R"(
***$$$
*BL SQL TYPE IS BINARY LARGE OBJECT 10K
***$$$
BL       DS   0FL4
BL_LENGTH DS FL4
BL_DATA DS CL10240
)",
        R"(
***$$$
*CL SQL TYPE IS CHARACTER LARGE OBJECT 10M
***$$$
CL       DS   0FL4
CL_LENGTH DS FL4
CL_DATA DS CL65535
 ORG   *+(10420225)
)",
        R"(
***$$$
*DL SQL TYPE IS DBCLOB 1G
***$$$
DL       DS   0FL4
DL_LENGTH DS FL4
DL_DATA DS GL65534
 ORG   *+(1073676289)
)",
        R"(
***$$$
*BLOC SQL TYPE IS BLOB_LOCATOR
***$$$
BLOC     DS    FL4
)",
        R"(
***$$$
*CLOC SQL TYPE IS CLOB_LOCATOR
***$$$
CLOC     DS    FL4
)",
        R"(
***$$$
*DLOC SQL TYPE IS DBCLOB_LOCATOR
***$$$
DLOC     DS    FL4
)",
        R"(
***$$$
*BFILE SQL TYPE IS BLOB_FILE
***$$$
BFILE    DS   0FL4
BFILE_NAME_LENGTH DS FL4
BFILE_DATA_LENGTH DS FL4
BFILE_FILE_OPTIONS DS FL4
BFILE_NAME DS CL255
)",
        R"(
***$$$
*CFILE SQL TYPE IS CLOB_FILE
***$$$
CFILE    DS   0FL4
CFILE_NAME_LENGTH DS FL4
CFILE_DATA_LENGTH DS FL4
CFILE_FILE_OPTIONS DS FL4
CFILE_NAME DS CL255
)",
        R"(
***$$$
*DFILE SQL TYPE IS DBCLOB_FILE
***$$$
DFILE    DS   0FL4
DFILE_NAME_LENGTH DS FL4
DFILE_DATA_LENGTH DS FL4
DFILE_FILE_OPTIONS DS FL4
DFILE_NAME DS CL255
)"
    };

    size_t result_id = 0;
    while (!text.empty())
    {
        ASSERT_LT(result_id, expected.size());

        auto result = p->generate_replacement(text, lineno);
        ASSERT_TRUE(result.has_value());

        std::string_view e = expected[result_id];
        e.remove_prefix(1);

        EXPECT_EQ(result, e);

        ++result_id;
    }
    EXPECT_EQ(result_id, expected.size());
    EXPECT_EQ(diags.diags.size(), 0);
}

TEST(db2_preprocessor, sql_types_with_space)
{
    std::string input = R"(
RE SQL TYPE IS RESULT_SET_LOCATOR VARYING   
RO SQL TYPE IS ROWID                        
TU SQL TYPE IS TABLE LIKE A AS LOCATOR      
TQ SQL TYPE IS TABLE LIKE 'A''B' AS LOCATOR 
XB SQL TYPE IS XML AS BLOB 10               
XC SQL TYPE IS XML AS CLOB 10K              
XD SQL TYPE IS XML AS DBCLOB 10M            
BL SQL TYPE IS BINARY LARGE OBJECT 10K      
CL SQL TYPE IS CHARACTER LARGE OBJECT 10M   
DL SQL TYPE IS DBCLOB 1G                    
BLOC SQL TYPE IS BLOB_LOCATOR               
CLOC SQL TYPE IS CLOB_LOCATOR               
DLOC SQL TYPE IS DBCLOB_LOCATOR             
BFILE SQL TYPE IS BLOB_FILE                 
CFILE SQL TYPE IS CLOB_FILE                 
DFILE SQL TYPE IS DBCLOB_FILE               
)";

    analyzer a(input, analyzer_options { db2_preprocessor_options {} });
    a.analyze();
    a.collect_diags();

    EXPECT_TRUE(a.diags().empty());
}

TEST(db2_preprocessor, sql_types_with_comment)
{
    std::string input = R"(
RE SQL TYPE IS RESULT_SET_LOCATOR VARYING   comment
RO SQL TYPE IS ROWID                        comment
TU SQL TYPE IS TABLE LIKE A AS LOCATOR      comment
TQ SQL TYPE IS TABLE LIKE 'A''B' AS LOCATOR comment
XB SQL TYPE IS XML AS BLOB 10               comment
XC SQL TYPE IS XML AS CLOB 10K              comment
XD SQL TYPE IS XML AS DBCLOB 10M            comment
BL SQL TYPE IS BINARY LARGE OBJECT 10K      comment
CL SQL TYPE IS CHARACTER LARGE OBJECT 10M   comment
DL SQL TYPE IS DBCLOB 1G                    comment
BLOC SQL TYPE IS BLOB_LOCATOR               comment
CLOC SQL TYPE IS CLOB_LOCATOR               comment
DLOC SQL TYPE IS DBCLOB_LOCATOR             comment
BFILE SQL TYPE IS BLOB_FILE                 comment
CFILE SQL TYPE IS CLOB_FILE                 comment
DFILE SQL TYPE IS DBCLOB_FILE               comment
)";

    analyzer a(input, analyzer_options { db2_preprocessor_options {} });
    a.analyze();
    a.collect_diags();

    EXPECT_TRUE(a.diags().empty());
}

TEST(db2_preprocessor, sql_type_fails)
{
    for (std::string_view text : {
             "A SQL TYPE IS ",
             "A SQL TYPE IS UNKNOWN",
             "A SQL TYPE IS BLOB",
             "A SQL TYPE IS TABLE LIKE AAA AS LOCATORR",
         })
    {
        diagnostic_op_consumer_container diags;
        auto p = preprocessor::create(
            db2_preprocessor_options {}, [](std::string_view) { return std::nullopt; }, &diags);

        size_t lineno = 0;

        EXPECT_TRUE(p->generate_replacement(text, lineno));

        ASSERT_EQ(diags.diags.size(), 1U);
        EXPECT_EQ(diags.diags[0].code, "DB004");
    }
}

TEST(db2_preprocessor, sql_type_warn_on_continuation)
{
    std::string_view text = "A SQL TYPE IS TABLE LIKE AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\n"
                            "                AS LOCATOR";
    diagnostic_op_consumer_container diags;
    auto p = preprocessor::create(
        db2_preprocessor_options {}, [](std::string_view) { return std::nullopt; }, &diags);

    size_t lineno = 0;

    EXPECT_TRUE(p->generate_replacement(text, lineno));

    ASSERT_EQ(diags.diags.size(), 1U);
    EXPECT_EQ(diags.diags[0].code, "DB005");
}

TEST(db2_preprocessor, no_codegen_for_unacceptable_sql_statement)
{
    std::string input = R"(
    EXEC SQL BEGIN DECLARE SECTION
    EXEC SQL END DECLARE SECTION
)";

    analyzer a(input, analyzer_options(db2_preprocessor_options()));
    a.analyze();
    a.collect_diags();

    EXPECT_TRUE(a.diags().empty()); // TODO(optional): original warns
}

TEST(db2_preprocessor, package_info_missing)
{
    std::string input = R"(
    LARL 0,SQLVERSP
    END
)";

    analyzer a(input, analyzer_options(db2_preprocessor_options()));
    a.analyze();
    a.collect_diags();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "E010" }));
}

TEST(db2_preprocessor, package_info_short)
{
    std::string input = R"(
    LARL 0,SQLVERSP
    LARL 0,SQLVERD1
    END
)";

    analyzer a(input, analyzer_options(db2_preprocessor_options("AAA")));
    a.analyze();
    a.collect_diags();

    EXPECT_TRUE(a.diags().empty());
}

TEST(db2_preprocessor, package_info_long)
{
    std::string input = R"(
    LARL 0,SQLVERSP
    LARL 0,SQLVERS
    LARL 0,SQLVERD1
    LARL 0,SQLVERD2
    END
)";

    analyzer a(input, analyzer_options(db2_preprocessor_options(std::string(48, 'A'))));
    a.analyze();
    a.collect_diags();

    EXPECT_TRUE(a.diags().empty());
}
