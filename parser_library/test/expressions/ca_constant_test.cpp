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

#include "gmock/gmock.h"

#include "expr_mocks.h"
#include "expressions/conditional_assembly/terms/ca_constant.h"
#include "expressions/evaluation_context.h"

using namespace hlasm_plugin::parser_library::expressions;
using namespace hlasm_plugin::parser_library::semantics;
using namespace hlasm_plugin::parser_library;

TEST(ca_constant, undefined_attributes)
{
    lib_prov_mock lib;
    evaluation_context eval_ctx {
        analyzing_context { std::make_shared<context::hlasm_context>(), std::make_shared<lsp::lsp_context>() }, lib
    };

    ca_constant c(1, range());

    ASSERT_EQ(c.get_undefined_attributed_symbols(eval_ctx).size(), 0U);
}

class collectable_mock : public diagnosable_op_impl
{
    void collect_diags() const override {}

public:
    void operator()(diagnostic_op d) { add_diagnostic(d); }
};

TEST(ca_constant, self_def_term_invalid_input)
{
    {
        collectable_mock m;
        diagnostic_adder add_diags(m, range());
        ca_constant::self_defining_term("", add_diags);
        ASSERT_TRUE(add_diags.diagnostics_present);
        EXPECT_EQ(m.diags().front().code, "CE015");
    }
    {
        collectable_mock m;
        diagnostic_adder add_diags(m, range());
        ca_constant::self_defining_term("Q'dc'", add_diags);
        ASSERT_TRUE(add_diags.diagnostics_present);
        EXPECT_EQ(m.diags().front().code, "CE015");
    }
}

TEST(ca_constant, self_def_term_valid_input)
{
    {
        diagnostic_adder add_diags;
        auto res = ca_constant::self_defining_term("B'10'", add_diags);
        ASSERT_FALSE(add_diags.diagnostics_present);
        EXPECT_EQ(res, 2);
    }
    {
        diagnostic_adder add_diags;
        auto res = ca_constant::self_defining_term("C'1'", add_diags);
        ASSERT_FALSE(add_diags.diagnostics_present);
        EXPECT_EQ(res, 241);
    }
    {
        diagnostic_adder add_diags;
        auto res = ca_constant::self_defining_term("X'f'", add_diags);
        ASSERT_FALSE(add_diags.diagnostics_present);
        EXPECT_EQ(res, 15);
    }
}
