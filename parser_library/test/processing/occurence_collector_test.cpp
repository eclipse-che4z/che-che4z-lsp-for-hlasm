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
#include "processing/statement_analyzers/occurence_collector.h"

using namespace hlasm_plugin::parser_library;


struct operand_occurence_analyzer_mock : public processing::statement_analyzer
{
    analyzer a;
    lsp::occurence_storage st;
    lsp::occurence_kind occ_kind;

    operand_occurence_analyzer_mock(const std::string& text, lsp::occurence_kind kind)
        : a(text)
        , occ_kind(kind)
    {
        a.register_stmt_analyzer(this);
        a.analyze();
    }

    void analyze(const context::hlasm_statement& statement,
        processing::statement_provider_kind,
        processing::processing_kind) override
    {
        const auto* res_stmt = statement.access_resolved();
        ASSERT_TRUE(res_stmt);
        processing::occurence_collector collector(occ_kind, *a.context().hlasm_ctx, st);
        const auto& operands = res_stmt->operands_ref().value;
        std::for_each(operands.begin(), operands.end(), [&](const auto& op) { op->apply(collector); });
    }

    context::id_index get_id(const std::string& s) { return a.context().hlasm_ctx->ids().add(s); }
};

auto tie_occurence(const lsp::symbol_occurence& lhs)
{
    return std::tie(*lhs.name,
        lhs.kind,
        lhs.occurence_range.start.line,
        lhs.occurence_range.start.column,
        lhs.occurence_range.end.line,
        lhs.occurence_range.end.column);
}

void sort_occurence_vector(std::vector<lsp::symbol_occurence>& v)
{
    std::sort(v.begin(), v.end(), [](const lsp::symbol_occurence& lhs, const lsp::symbol_occurence& rhs) {
        return tie_occurence(lhs) <= tie_occurence(rhs);
    });
}


TEST(occurence_collector, ord_mach_expr_simple)
{
    std::string input = " LR 1,SYM";
    operand_occurence_analyzer_mock oa(input, lsp::occurence_kind::ORD);

    ASSERT_EQ(oa.st.size(), 1U);
    EXPECT_EQ(oa.st[0].kind, lsp::occurence_kind::ORD);
    EXPECT_EQ(*oa.st[0].name, "SYM");
    EXPECT_EQ(oa.st[0].occurence_range, range({ 0, 6 }, { 0, 9 }));
}

TEST(occurence_collector, ord_mach_expr_operators)
{
    std::string input = " LR 1,R1+R2*R3-R4/R5";
    // std::string input = " LR 1,R1";
    operand_occurence_analyzer_mock oa(input, lsp::occurence_kind::ORD);

    std::vector<lsp::symbol_occurence> expected = {
        { lsp::occurence_kind::ORD, oa.get_id("R1"), { { 0, 6 }, { 0, 8 } } },
        { lsp::occurence_kind::ORD, oa.get_id("R2"), { { 0, 9 }, { 0, 11 } } },
        { lsp::occurence_kind::ORD, oa.get_id("R3"), { { 0, 12 }, { 0, 14 } } },
        { lsp::occurence_kind::ORD, oa.get_id("R4"), { { 0, 15 }, { 0, 17 } } },
        { lsp::occurence_kind::ORD, oa.get_id("R5"), { { 0, 18 }, { 0, 20 } } }
    };

    sort_occurence_vector(oa.st);
    EXPECT_EQ(oa.st, expected);
}

TEST(occurence_collector, ord_mach_expr_data_attr)
{
    std::string input = " LR 1,L'SYM";
    operand_occurence_analyzer_mock oa(input, lsp::occurence_kind::ORD);

    ASSERT_EQ(oa.st.size(), 1U);
    EXPECT_EQ(oa.st[0], lsp::symbol_occurence(lsp::occurence_kind::ORD, oa.get_id("SYM"), { { 0, 8 }, { 0, 11 } }));
}

TEST(occurence_collector, var_mach_instr)
{
    std::string input = " LR &VAR,1";
    operand_occurence_analyzer_mock oa(input, lsp::occurence_kind::VAR);

    ASSERT_EQ(oa.st.size(), 1U);
    EXPECT_EQ(oa.st[0], lsp::symbol_occurence(lsp::occurence_kind::VAR, oa.get_id("VAR"), { { 0, 4 }, { 0, 8 } }));
}

TEST(occurence_collector, var_created_set_sym)
{
    std::string input = " LR &(&V1.&V2),1";
    operand_occurence_analyzer_mock oa(input, lsp::occurence_kind::VAR);

    std::vector<lsp::symbol_occurence> expected = {
        { lsp::occurence_kind::VAR, oa.get_id("V1"), { { 0, 6 }, { 0, 9 } } },
        { lsp::occurence_kind::VAR, oa.get_id("V2"), { { 0, 10 }, { 0, 13 } } }
    };
    sort_occurence_vector(oa.st);
    EXPECT_EQ(oa.st, expected);
}

TEST(occurence_collector, var_ca_expr)
{
    std::string input = " AIF (&V EQ 47).SEQ";
    operand_occurence_analyzer_mock oa(input, lsp::occurence_kind::VAR);

    ASSERT_EQ(oa.st.size(), 1U);
    EXPECT_EQ(oa.st[0], lsp::symbol_occurence(lsp::occurence_kind::VAR, oa.get_id("V"), { { 0, 6 }, { 0, 8 } }));
}

TEST(occurence_collector, var_ca_expr_ca_string_dupl_factor)
{
    std::string input = "&V SETC (&D)'STR'";
    operand_occurence_analyzer_mock oa(input, lsp::occurence_kind::VAR);

    ASSERT_EQ(oa.st.size(), 1U);
    EXPECT_EQ(oa.st[0], lsp::symbol_occurence(lsp::occurence_kind::VAR, oa.get_id("D"), { { 0, 9 }, { 0, 11 } }));
}


TEST(occurence_collector, ord_ca_expr)
{
    std::string input = " AIF (B EQ 15).SEQ";
    operand_occurence_analyzer_mock oa(input, lsp::occurence_kind::ORD);

    ASSERT_EQ(oa.st.size(), 1U);
    EXPECT_EQ(oa.st[0], lsp::symbol_occurence(lsp::occurence_kind::ORD, oa.get_id("B"), { { 0, 6 }, { 0, 7 } }));
}

TEST(occurence_collector, ord_ca_expr_data_attr)
{
    std::string input = " AIF (L'B EQ 15).SEQ";
    operand_occurence_analyzer_mock oa(input, lsp::occurence_kind::ORD);

    ASSERT_EQ(oa.st.size(), 1U);
    EXPECT_EQ(oa.st[0], lsp::symbol_occurence(lsp::occurence_kind::ORD, oa.get_id("B"), { { 0, 8 }, { 0, 9 } }));
}

TEST(occurence_collector, var_ca_expr_data_attr)
{
    std::string input = " AIF (L'&V EQ 15).SEQ";
    operand_occurence_analyzer_mock oa(input, lsp::occurence_kind::VAR);

    ASSERT_EQ(oa.st.size(), 1U);
    EXPECT_EQ(oa.st[0], lsp::symbol_occurence(lsp::occurence_kind::VAR, oa.get_id("V"), { { 0, 8 }, { 0, 10 } }));
}

TEST(occurence_collector, seq_aif)
{
    std::string input = " AIF (&B).SEQ";
    operand_occurence_analyzer_mock oa(input, lsp::occurence_kind::SEQ);

    ASSERT_EQ(oa.st.size(), 1U);
    EXPECT_EQ(oa.st[0], lsp::symbol_occurence(lsp::occurence_kind::SEQ, oa.get_id("SEQ"), { { 0, 9 }, { 0, 13 } }));
}

TEST(occurence_collector, seq_ago)
{
    std::string input = " AGO .SEQ";
    operand_occurence_analyzer_mock oa(input, lsp::occurence_kind::SEQ);

    ASSERT_EQ(oa.st.size(), 1U);
    EXPECT_EQ(oa.st[0], lsp::symbol_occurence(lsp::occurence_kind::SEQ, oa.get_id("SEQ"), { { 0, 5 }, { 0, 9 } }));
}

TEST(occurence_collector, ord_dc_operand_modifiers)
{
    std::string input = " DC (S1)FDP(S2)L(S3)S(S4)E(S5)'2.25'";
    operand_occurence_analyzer_mock oa(input, lsp::occurence_kind::ORD);

    std::vector<lsp::symbol_occurence> expected = {
        { lsp::occurence_kind::ORD, oa.get_id("S1"), { { 0, 5 }, { 0, 7 } } },
        { lsp::occurence_kind::ORD, oa.get_id("S2"), { { 0, 12 }, { 0, 14 } } },
        { lsp::occurence_kind::ORD, oa.get_id("S3"), { { 0, 17 }, { 0, 19 } } },
        { lsp::occurence_kind::ORD, oa.get_id("S4"), { { 0, 22 }, { 0, 24 } } },
        { lsp::occurence_kind::ORD, oa.get_id("S5"), { { 0, 27 }, { 0, 29 } } }
    };

    sort_occurence_vector(oa.st);
    EXPECT_EQ(oa.st, expected);
}

TEST(occurence_collector, ord_dc_operand_nominal_value)
{
    std::string input = " DC A(S1,S2(S3))";
    operand_occurence_analyzer_mock oa(input, lsp::occurence_kind::ORD);

    std::vector<lsp::symbol_occurence> expected = {
        { lsp::occurence_kind::ORD, oa.get_id("S1"), { { 0, 6 }, { 0, 8 } } },
        { lsp::occurence_kind::ORD, oa.get_id("S2"), { { 0, 9 }, { 0, 11 } } },
        { lsp::occurence_kind::ORD, oa.get_id("S3"), { { 0, 12 }, { 0, 14 } } }
    };

    sort_occurence_vector(oa.st);
    EXPECT_EQ(oa.st, expected);
}
