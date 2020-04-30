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

#include "../common_testing.h"

// tests for OPSYN instruction

TEST(OPSYN, simple)
{
    std::string input(R"(
OP1 OPSYN LR
OP2 OPSYN OP1
  OP2 1,1
  OP1 1,1
)");
    analyzer a(input);
    a.analyze();
    a.collect_diags();
    ASSERT_EQ(a.diags().size(), (size_t)0);
}

TEST(OPSYN, undefined_operand)
{
    std::string input(R"(
OP2 OPSYN OP1
)");
    analyzer a(input);
    a.analyze();
    a.collect_diags();
    ASSERT_EQ(a.diags().size(), (size_t)1);
}

TEST(OPSYN, undefined_name)
{
    std::string input(R"(
OP2 OPSYN
)");
    analyzer a(input);
    a.analyze();
    a.collect_diags();
    ASSERT_EQ(a.diags().size(), (size_t)1);
}

TEST(OPSYN, missing_name)
{
    std::string input(R"(
&VAR SETC ''
&VAR OPSYN &VAR
)");
    analyzer a(input);
    a.analyze();
    a.collect_diags();
    ASSERT_EQ(a.diags().size(), (size_t)1);
}

TEST(OPSYN, delete_opcode)
{
    std::string input(R"(
LR OPSYN 
  LR 1,1
)");
    analyzer a(input);
    a.analyze();
    a.collect_diags();
    ASSERT_EQ(a.diags().size(), (size_t)1);
}

TEST(OPSYN, preserve_opcode)
{
    std::string input(R"(
OP1 OPSYN LR
LR OPSYN 
  OP1 1,1
)");
    analyzer a(input);
    a.analyze();
    a.collect_diags();
    ASSERT_EQ(a.diags().size(), (size_t)0);
}

TEST(OPSYN, non_CA_instruction_before_macro_def)
{
    std::string input(R"(
OPSYN_THIS OPSYN SAM31 
 MACRO
 M
 OPSYN_THIS
 MEND
 M
)");
    analyzer a(input);
    a.analyze();
    a.collect_diags();
    ASSERT_EQ(a.diags().size(), (size_t)0);
}

TEST(OPSYN, non_CA_instruction_before_macro_call)
{
    std::string input(R"(
 MACRO
 M
 OPSYN_THIS
 MEND
OPSYN_THIS OPSYN SAM31 
 M
)");
    analyzer a(input);
    a.analyze();
    a.collect_diags();
    ASSERT_EQ(a.diags().size(), (size_t)0);
}

TEST(OPSYN, non_CA_instruction_after_macro_call)
{
    std::string input(R"(
 MACRO
 M
 OPSYN_THIS
 MEND
 M
OPSYN_THIS OPSYN SAM31 
)");
    analyzer a(input);
    a.analyze();
    a.collect_diags();
    ASSERT_EQ(a.diags().size(), (size_t)1);
}

TEST(OPSYN, CA_instruction)
{
    std::string input(R"(
 MACRO
 M1
 AGO .A
.A MEND
AGO OPSYN ACTR
 MACRO
 M2
 AGO 13
 MEND
 M1
 M2
)");
    analyzer a(input);
    a.analyze();
    a.collect_diags();
    ASSERT_EQ(a.diags().size(), (size_t)0);
}

TEST(OPSYN, macro_definition)
{
    std::string input(R"(
MACROX OPSYN MACRO
MENDX OPSYN MEND

 MACROX
 M
 LR 1,1
 MENDX

 M
)");
    analyzer a(input);
    a.analyze();
    a.collect_diags();
    ASSERT_EQ(a.diags().size(), (size_t)0);
}

TEST(OPSYN, macro_redefinition)
{
    std::string input(R"(
 MACRO
 M
 LR 1,1
 MEND

M_ OPSYN M

 MACRO
 M
 M_
 MEND

 M
)");
    analyzer a(input);
    a.analyze();
    a.collect_diags();
    ASSERT_EQ(a.diags().size(), (size_t)0);
}

TEST(OPSYN, macro_mach_redefinition)
{
    std::string input(R"(
LR_ OPSYN LR

 MACRO
 LR &VAR
 LR_ 1,&VAR
 MEND

 LR 1
)");
    analyzer a(input);
    a.analyze();
    a.collect_diags();
    ASSERT_EQ(a.diags().size(), (size_t)0);
}

TEST(OPSYN, late_macro_definition)
{
    std::string input(R"(
LRX OPSYN LR
 MACRO
 LR
 LR 1,1
 MEND
 
 LRX  

)");
    analyzer a(input);
    a.analyze();
    a.collect_diags();
    ASSERT_EQ(a.diags().size(), (size_t)1);
}

TEST(OPSYN, removed_machine_instruction)
{
    std::string input(R"(
LR OPSYN
X OPSYN LR
)");
    analyzer a(input);
    a.analyze();
    a.collect_diags();
    ASSERT_EQ(a.diags().size(), (size_t)1);
}

class opsyn_parse_lib_prov : public parse_lib_provider
{
    std::unique_ptr<analyzer> a;

    std::string LIB =
        R"( MACRO
 LR
 MEND)";

    virtual parse_result parse_library(
        const std::string& library, context::hlasm_context& hlasm_ctx, const library_data data) override
    {
        std::string* content;
        if (library == "LR")
            content = &LIB;
        else
            return false;

        a = std::make_unique<analyzer>(*content, library, hlasm_ctx, *this, data);
        a->analyze();
        a->collect_diags();
        return true;
    }

    virtual bool has_library(const std::string&, context::hlasm_context&) const override { return false; }
};

TEST(OPSYN, macro_after_delete)
{
    std::string input(R"(
LR OPSYN
   LR
)");
    opsyn_parse_lib_prov mock;
    analyzer a(input, "", mock);
    a.analyze();
    a.collect_diags();
    ASSERT_EQ(a.diags().size(), (size_t)0);
}