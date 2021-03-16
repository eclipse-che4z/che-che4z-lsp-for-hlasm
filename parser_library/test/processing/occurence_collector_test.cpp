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
        processing::statement_provider_kind prov_kind,
        processing::processing_kind proc_kind) override
    {
        if (statement.access_resolved())
        {
            processing::occurence_collector collector(occ_kind, *a.context().hlasm_ctx, st);
            const auto& operands = statement.access_resolved()->operands_ref().value;
            std::for_each(operands.begin(), operands.end(), [&](const auto& op) { op->apply(collector); });
        }
        else
        {
            //const auto& operands = statement.access_deferred()->deferred_ref().
        }
    }

    context::id_index get_id(const std::string& s) { return a.context().hlasm_ctx->ids().add(s); }
};

#define EXPECT_VECTORS_EQ_UNORDERED(actual, expected)                                                                  \
    do                                                                                                                 \
    {                                                                                                                  \
        ASSERT_EQ(actual.size(), expected.size());                                                                     \
        for (const auto& i : actual)                                                                                   \
        {                                                                                                              \
            auto it = std::find(expected.begin(), expected.end(), i);                                                  \
            ASSERT_NE(it, expected.end()) << "Test returned unexpected value:" << i;                                   \
        }                                                                                                              \
    } while (false)


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

    EXPECT_VECTORS_EQ_UNORDERED(oa.st, expected);
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

TEST(occurence_collector, var_mach_instr_created_set_sym)
{
    std::string input = " LR &(&V1.&V2),1";
    operand_occurence_analyzer_mock oa(input, lsp::occurence_kind::VAR);

    std::vector<lsp::symbol_occurence> expected = {
        { lsp::occurence_kind::VAR, oa.get_id("V1"), { { 0, 6 }, { 0, 9 } } },
        { lsp::occurence_kind::VAR, oa.get_id("V2"), { { 0, 10 }, { 0, 13 } } }
    };

    EXPECT_VECTORS_EQ_UNORDERED(oa.st, expected);
}

TEST(occurence_collector, var_expr_simple)
{
    std::string input = " AIF (&V EQ 47).SEQ";
    operand_occurence_analyzer_mock oa(input, lsp::occurence_kind::VAR);

    ASSERT_EQ(oa.st.size(), 1U);
    EXPECT_EQ(oa.st[0], lsp::symbol_occurence(lsp::occurence_kind::VAR, oa.get_id("V"), { { 0, 6 }, { 0, 8 } }));
}






