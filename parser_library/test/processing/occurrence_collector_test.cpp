/*
 * Copyright (c) 2019 Broadcom.
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

#include "../gtest_stringers.h"
#include "analyzer.h"
#include "processing/statement.h"
#include "processing/statement_analyzers/occurrence_collector.h"
#include "processing/statement_analyzers/statement_analyzer.h"

using namespace hlasm_plugin::parser_library;


struct operand_occurrence_analyzer_mock : public processing::statement_analyzer
{
    analyzer a;
    std::vector<lsp::symbol_occurrence> st;
    lsp::occurrence_kind occ_kind;

    operand_occurrence_analyzer_mock(const std::string& text, lsp::occurrence_kind kind)
        : a(text)
        , occ_kind(kind)
    {
        a.register_stmt_analyzer(this);
        a.analyze();
    }

    bool analyze(const context::hlasm_statement& statement,
        processing::statement_provider_kind,
        processing::processing_kind,
        bool evaluated_model) override
    {
        const auto* res_stmt = statement.access_resolved();
        EXPECT_TRUE(res_stmt);
        assert(res_stmt);
        processing::occurrence_collector collector(occ_kind, *a.context().hlasm_ctx, st, evaluated_model);
        std::ranges::for_each(res_stmt->operands_ref().value, [&collector](const auto& op) { op->apply(collector); });

        return false;
    }

    void analyze_aread_line(const hlasm_plugin::utils::resource::resource_location&, size_t, std::string_view) override
    {}
};

auto tie_occurrence(const lsp::symbol_occurrence& lhs)
{
    return std::make_tuple(lhs.name.to_string_view(),
        lhs.kind,
        lhs.occurrence_range.start.line,
        lhs.occurrence_range.start.column,
        lhs.occurrence_range.end.line,
        lhs.occurrence_range.end.column);
}

void sort_occurrence_vector(std::vector<lsp::symbol_occurrence>& v) { std::ranges::sort(v, {}, tie_occurrence); }


TEST(occurrence_collector, ord_mach_expr_simple)
{
    std::string input = " LR 1,SYM";
    operand_occurrence_analyzer_mock oa(input, lsp::occurrence_kind::ORD);

    ASSERT_EQ(oa.st.size(), 1U);
    EXPECT_EQ(oa.st[0].kind, lsp::occurrence_kind::ORD);
    EXPECT_EQ(oa.st[0].name.to_string_view(), "SYM");
    EXPECT_EQ(oa.st[0].occurrence_range, range({ 0, 6 }, { 0, 9 }));
}

TEST(occurrence_collector, ord_mach_expr_operators)
{
    std::string input = " LR 1,R1+R2*R3-R4/R5";
    // std::string input = " LR 1,R1";
    operand_occurrence_analyzer_mock oa(input, lsp::occurrence_kind::ORD);

    std::vector<lsp::symbol_occurrence> expected = {
        { lsp::occurrence_kind::ORD, context::id_index("R1"), { { 0, 6 }, { 0, 8 } }, false },
        { lsp::occurrence_kind::ORD, context::id_index("R2"), { { 0, 9 }, { 0, 11 } }, false },
        { lsp::occurrence_kind::ORD, context::id_index("R3"), { { 0, 12 }, { 0, 14 } }, false },
        { lsp::occurrence_kind::ORD, context::id_index("R4"), { { 0, 15 }, { 0, 17 } }, false },
        { lsp::occurrence_kind::ORD, context::id_index("R5"), { { 0, 18 }, { 0, 20 } }, false }
    };

    sort_occurrence_vector(oa.st);
    EXPECT_EQ(oa.st, expected);
}

TEST(occurrence_collector, ord_mach_expr_data_attr)
{
    std::string input = " LR 1,L'SYM";
    operand_occurrence_analyzer_mock oa(input, lsp::occurrence_kind::ORD);

    ASSERT_EQ(oa.st.size(), 1U);
    EXPECT_EQ(oa.st[0],
        lsp::symbol_occurrence(lsp::occurrence_kind::ORD, context::id_index("SYM"), { { 0, 8 }, { 0, 11 } }, false));
}

TEST(occurrence_collector, var_mach_instr)
{
    std::string input = " LR &VAR,1";
    operand_occurrence_analyzer_mock oa(input, lsp::occurrence_kind::VAR);

    ASSERT_EQ(oa.st.size(), 1U);
    EXPECT_EQ(oa.st[0],
        lsp::symbol_occurrence(lsp::occurrence_kind::VAR, context::id_index("VAR"), { { 0, 4 }, { 0, 8 } }, false));
}

TEST(occurrence_collector, var_created_set_sym)
{
    std::string input = " LR &(&V1.&V2),1";
    operand_occurrence_analyzer_mock oa(input, lsp::occurrence_kind::VAR);

    std::vector<lsp::symbol_occurrence> expected = {
        { lsp::occurrence_kind::VAR, context::id_index("V1"), { { 0, 6 }, { 0, 9 } }, false },
        { lsp::occurrence_kind::VAR, context::id_index("V2"), { { 0, 10 }, { 0, 13 } }, false }
    };
    sort_occurrence_vector(oa.st);
    EXPECT_EQ(oa.st, expected);
}

TEST(occurrence_collector, var_ca_expr)
{
    std::string input = " AIF (&V EQ 47).SEQ";
    operand_occurrence_analyzer_mock oa(input, lsp::occurrence_kind::VAR);

    ASSERT_EQ(oa.st.size(), 1U);
    EXPECT_EQ(oa.st[0],
        lsp::symbol_occurrence(lsp::occurrence_kind::VAR, context::id_index("V"), { { 0, 6 }, { 0, 8 } }, false));
}

TEST(occurrence_collector, var_ca_expr_in_string)
{
    std::string input = " AIF ('&V' EQ ' ').SEQ";
    operand_occurrence_analyzer_mock oa(input, lsp::occurrence_kind::VAR);

    ASSERT_EQ(oa.st.size(), 1U);
    EXPECT_EQ(oa.st[0],
        lsp::symbol_occurrence(lsp::occurrence_kind::VAR, context::id_index("V"), { { 0, 7 }, { 0, 9 } }, false));
}

TEST(occurrence_collector, var_ca_expr_ca_string_dupl_factor)
{
    std::string input = "&V SETC (&D)'STR'";
    operand_occurrence_analyzer_mock oa(input, lsp::occurrence_kind::VAR);

    ASSERT_EQ(oa.st.size(), 1U);
    EXPECT_EQ(oa.st[0],
        lsp::symbol_occurrence(lsp::occurrence_kind::VAR, context::id_index("D"), { { 0, 9 }, { 0, 11 } }, false));
}


TEST(occurrence_collector, ord_ca_expr)
{
    std::string input = " AIF (B EQ 15).SEQ";
    operand_occurrence_analyzer_mock oa(input, lsp::occurrence_kind::ORD);

    ASSERT_EQ(oa.st.size(), 1U);
    EXPECT_EQ(oa.st[0],
        lsp::symbol_occurrence(lsp::occurrence_kind::ORD, context::id_index("B"), { { 0, 6 }, { 0, 7 } }, false));
}

TEST(occurrence_collector, ord_ca_expr_data_attr)
{
    std::string input = " AIF (L'B EQ 15).SEQ";
    operand_occurrence_analyzer_mock oa(input, lsp::occurrence_kind::ORD);

    ASSERT_EQ(oa.st.size(), 1U);
    EXPECT_EQ(oa.st[0],
        lsp::symbol_occurrence(lsp::occurrence_kind::ORD, context::id_index("B"), { { 0, 8 }, { 0, 9 } }, false));
}

TEST(occurrence_collector, var_ca_expr_data_attr)
{
    std::string input = " AIF (L'&V EQ 15).SEQ";
    operand_occurrence_analyzer_mock oa(input, lsp::occurrence_kind::VAR);

    ASSERT_EQ(oa.st.size(), 1U);
    EXPECT_EQ(oa.st[0],
        lsp::symbol_occurrence(lsp::occurrence_kind::VAR, context::id_index("V"), { { 0, 8 }, { 0, 10 } }, false));
}

TEST(occurrence_collector, seq_aif)
{
    std::string input = " AIF (&B).SEQ";
    operand_occurrence_analyzer_mock oa(input, lsp::occurrence_kind::SEQ);

    ASSERT_EQ(oa.st.size(), 1U);
    EXPECT_EQ(oa.st[0],
        lsp::symbol_occurrence(lsp::occurrence_kind::SEQ, context::id_index("SEQ"), { { 0, 9 }, { 0, 13 } }, false));
}

TEST(occurrence_collector, seq_ago)
{
    std::string input = " AGO .SEQ";
    operand_occurrence_analyzer_mock oa(input, lsp::occurrence_kind::SEQ);

    ASSERT_EQ(oa.st.size(), 1U);
    EXPECT_EQ(oa.st[0],
        lsp::symbol_occurrence(lsp::occurrence_kind::SEQ, context::id_index("SEQ"), { { 0, 5 }, { 0, 9 } }, false));
}

TEST(occurrence_collector, ord_dc_operand_modifiers)
{
    std::string input = " DC (S1)FDP(S2)L(S3)S(S4)E(S5)'2.25'";
    operand_occurrence_analyzer_mock oa(input, lsp::occurrence_kind::ORD);

    std::vector<lsp::symbol_occurrence> expected = {
        { lsp::occurrence_kind::ORD, context::id_index("S1"), { { 0, 5 }, { 0, 7 } }, false },
        { lsp::occurrence_kind::ORD, context::id_index("S2"), { { 0, 12 }, { 0, 14 } }, false },
        { lsp::occurrence_kind::ORD, context::id_index("S3"), { { 0, 17 }, { 0, 19 } }, false },
        { lsp::occurrence_kind::ORD, context::id_index("S4"), { { 0, 22 }, { 0, 24 } }, false },
        { lsp::occurrence_kind::ORD, context::id_index("S5"), { { 0, 27 }, { 0, 29 } }, false }
    };

    sort_occurrence_vector(oa.st);
    EXPECT_EQ(oa.st, expected);
}

TEST(occurrence_collector, ord_dc_operand_nominal_value)
{
    std::string input = " DC A(S1,S2(S3))";
    operand_occurrence_analyzer_mock oa(input, lsp::occurrence_kind::ORD);

    std::vector<lsp::symbol_occurrence> expected = {
        { lsp::occurrence_kind::ORD, context::id_index("S1"), { { 0, 6 }, { 0, 8 } }, false },
        { lsp::occurrence_kind::ORD, context::id_index("S2"), { { 0, 9 }, { 0, 11 } }, false },
        { lsp::occurrence_kind::ORD, context::id_index("S3"), { { 0, 12 }, { 0, 14 } }, false }
    };

    sort_occurrence_vector(oa.st);
    EXPECT_EQ(oa.st, expected);
}

TEST(occurrence_collector, ord_literal)
{
    std::string input = R"(
    LARL 0,=A(X,X)
X   DC   A(X)
&A  SETA L'=A(X)
)";
    operand_occurrence_analyzer_mock oa(input, lsp::occurrence_kind::ORD);

    // operands only
    std::vector<lsp::symbol_occurrence> expected = {
        { lsp::occurrence_kind::ORD, context::id_index("X"), { { 1, 14 }, { 1, 15 } }, false },
        { lsp::occurrence_kind::ORD, context::id_index("X"), { { 1, 16 }, { 1, 17 } }, false },
        { lsp::occurrence_kind::ORD, context::id_index("X"), { { 2, 11 }, { 2, 12 } }, false },
        { lsp::occurrence_kind::ORD, context::id_index("X"), { { 3, 14 }, { 3, 15 } }, false },
    };

    sort_occurrence_vector(oa.st);
    EXPECT_EQ(oa.st, expected);
}
