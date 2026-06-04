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

#include "../common_testing.h"
#include "expressions/mach_expr_term.h"
#include "semantics/operand_impls.h"
#include "semantics/variable_symbol.h"

using namespace hlasm_plugin::parser_library::semantics;
using namespace hlasm_plugin::parser_library;

bool access_op(operand_type type, operand* o)
{
    auto asm_op = o->access_asm();
    if ((type == operand_type::ASM) != (asm_op != nullptr)) // equivalence negation
        return false;
    auto ca_op = o->access_ca();
    if ((type == operand_type::CA) != (ca_op != nullptr))
        return false;
    auto dat_op = o->access_data_def();
    if ((type == operand_type::DAT) != (dat_op != nullptr))
        return false;
    auto mac_op = o->access_mac();
    if ((type == operand_type::MAC) != (mac_op != nullptr))
        return false;
    auto mach_op = o->access_mach();
    if ((type == operand_type::MACH) != (mach_op != nullptr))
        return false;
    auto model_op = o->access_model();
    if ((type == operand_type::MODEL) != (model_op != nullptr))
        return false;

    return true;
}

bool access_asm_op(asm_kind kind, assembler_operand* o)
{
    auto expr_op = o->access_expr();
    if ((kind == asm_kind::EXPR) != (expr_op != nullptr))
        return false;
    auto base_op = o->access_base_end();
    if ((kind == asm_kind::BASE_END) != (base_op != nullptr))
        return false;
    auto complex_op = o->access_complex();
    if ((kind == asm_kind::COMPLEX) != (complex_op != nullptr))
        return false;
    auto string_op = o->access_string();
    if ((kind == asm_kind::STRING) != (string_op != nullptr))
        return false;

    operand* op = o;
    return access_op(operand_type::ASM, op);
}

bool access_ca_op(ca_kind kind, ca_operand* o)
{
    auto var_op = o->access_var();
    if ((kind == ca_kind::VAR) != (var_op != nullptr))
        return false;
    auto expr_op = o->access_expr();
    if ((kind == ca_kind::EXPR) != (expr_op != nullptr))
        return false;
    auto seq_op = o->access_seq();
    if ((kind == ca_kind::SEQ) != (seq_op != nullptr))
        return false;
    auto branch_op = o->access_branch();
    if ((kind == ca_kind::BRANCH) != (branch_op != nullptr))
        return false;

    operand* op = o;
    return access_op(operand_type::CA, op);
}

bool access_mac_op(macro_operand* o)
{
    operand* op = o;
    return access_op(operand_type::MAC, op);
}

TEST(operand, access_operand)
{
    // model operand
    model_operand mo({}, {}, range());
    EXPECT_TRUE(access_op(operand_type::MODEL, &mo));

    // machine operand
    machine_operand emo(std::make_unique<mach_expr_constant>(0, range()), nullptr, nullptr, range());
    EXPECT_TRUE(access_op(operand_type::MACH, &emo));

    // assembler operand
    // expr
    expr_assembler_operand eao(nullptr, range());
    EXPECT_TRUE(access_asm_op(asm_kind::EXPR, &eao));
    // base end
    using_instr_assembler_operand uao(nullptr, nullptr, "", "", range());
    EXPECT_TRUE(access_asm_op(asm_kind::BASE_END, &uao));
    // complex
    complex_assembler_operand cao("", {}, range());
    EXPECT_TRUE(access_asm_op(asm_kind::COMPLEX, &cao));
    // string
    string_assembler_operand sao("", range());
    EXPECT_TRUE(access_asm_op(asm_kind::STRING, &sao));

    // datadef operand
    data_def_operand_inline ddo(expressions::data_definition {}, range());
    EXPECT_TRUE(access_op(operand_type::DAT, &ddo));

    // ca operand
    // var
    var_ca_operand vco(nullptr, range());
    EXPECT_TRUE(access_ca_op(ca_kind::VAR, &vco));
    // expr
    expr_ca_operand eco(nullptr, range());
    EXPECT_TRUE(access_ca_op(ca_kind::EXPR, &eco));
    // seq
    seq_ca_operand sco({}, range());
    EXPECT_TRUE(access_ca_op(ca_kind::SEQ, &sco));
    // branch
    branch_ca_operand bco({}, nullptr, range());
    EXPECT_TRUE(access_ca_op(ca_kind::BRANCH, &bco));

    // macro operand
    // chain
    macro_operand moc({}, range());
    EXPECT_TRUE(access_mac_op(&moc));
}

TEST(operand, resolve_model)
{
    std::string input =
        R"(
A       DS     A
&A(1)   SETC   '0,A'
        LARL   &A((1 AND 1))
)";
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());
}
