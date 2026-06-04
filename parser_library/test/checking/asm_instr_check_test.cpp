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

#include "checking/diagnostic_collector.h"
#include "checking/instr_operand.h"
#include "checking/instruction_checker.h"

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::checking;

class instruction_test : public testing::Test
{
public:
    instruction_test()
    {
        for (auto i = 0; i < 66000; i++)
            test_entry_false.push_back(&empty_val);

        test_adata_true_one.push_back(&op_val_287);
        test_adata_true_one.push_back(&empty_val);
        test_adata_true_one.push_back(&op_val_min_567);
        test_adata_true_one.push_back(&zero_val);
        test_adata_true_one.push_back(&adata_string_val);

        test_adata_true_two.push_back(&zero_val);
        test_adata_true_two.push_back(&empty_val);
        test_adata_true_two.push_back(&empty_val);
        test_adata_true_two.push_back(&empty_val);
        test_adata_true_two.push_back(&empty_val);
        // acontrol instruction

        auto acontrol_compat_vector = std::vector<std::unique_ptr<asm_operand>> {};
        acontrol_compat_vector.push_back(std::make_unique<one_operand>("CASE"));
        acontrol_compat_vector.push_back(std::make_unique<one_operand>("NOMC"));
        acontrol_compat_vector.push_back(std::make_unique<one_operand>("NOTRS"));
        acontrol_compat_vector.push_back(std::make_unique<one_operand>("NOTRANSDT"));
        acontrol_compat_vector.push_back(std::make_unique<one_operand>("NOTRANSDT"));
        acontrol_operand_compat = complex_operand("COMPAT", std::move(acontrol_compat_vector));

        auto acontrol_flag_vector = std::vector<std::unique_ptr<asm_operand>> {};
        acontrol_flag_vector.push_back(std::make_unique<one_operand>(2));
        acontrol_flag_vector.push_back(std::make_unique<one_operand>("AL"));
        acontrol_flag_vector.push_back(std::make_unique<one_operand>("NOCONT"));
        acontrol_flag_vector.push_back(std::make_unique<one_operand>("IMPLEN"));
        acontrol_flag_vector.push_back(std::make_unique<one_operand>("NOUSING0"));
        acontrol_operand_flag = complex_operand("FLAG", std::move(acontrol_flag_vector));

        auto acontrol_typecheck_vector = std::vector<std::unique_ptr<asm_operand>> {};
        acontrol_typecheck_vector.push_back(std::make_unique<one_operand>("MAGNITUDE"));
        acontrol_typecheck_vector.push_back(std::make_unique<one_operand>("NOREGISTER"));
        acontrol_typecheck_vector.push_back(std::make_unique<one_operand>("REGISTER"));
        acontrol_operand_typecheck = complex_operand("TYPECHECK", std::move(acontrol_typecheck_vector));

        auto acontrol_optable_vector = std::vector<std::unique_ptr<asm_operand>> {};
        acontrol_optable_vector.push_back(std::make_unique<one_operand>("ZS6"));
        acontrol_optable_vector.push_back(std::make_unique<one_operand>("LIST"));
        acontrol_operand_optable = complex_operand("OPTABLE", std::move(acontrol_optable_vector));

        test_acontrol_true.push_back(&acontrol_afpr);
        test_acontrol_true.push_back(&acontrol_operand_compat);
        test_acontrol_true.push_back(&op_str_nocompat);
        test_acontrol_true.push_back(&acontrol_operand_flag);
        test_acontrol_true.push_back(&op_str_lmac);
        test_acontrol_true.push_back(&acontrol_operand_typecheck);
        test_acontrol_true.push_back(&op_str_ra2);
        test_acontrol_true.push_back(&acontrol_operand_optable);
        test_acontrol_true.push_back(&op_str_notc);

        // ainsert instruction

        test_ainsert_true_one.push_back(&ainsert_one);
        test_ainsert_true_one.push_back(&ainsert_back);

        test_ainsert_true_two.push_back(&ainsert_lorem);
        test_ainsert_true_two.push_back(&ainsert_front);

        test_ainsert_false.push_back(&ainsert_apo);
        test_ainsert_false.push_back(&ainsert_front);

        // alias instruction

        test_alias_true_one.push_back(&op_str_alias_one);
        test_alias_true_two.push_back(&op_str_alias_two);
        test_alias_false.push_back(&op_str_alias_three);

        // amode instruction
        test_amode_true.push_back(&amode_any31);

        // cattr instruction

        auto cattr_rmode_vector = std::vector<std::unique_ptr<asm_operand>> {};
        cattr_rmode_vector.push_back(std::make_unique<one_operand>("ANY"));
        cattr_operand_rmode = complex_operand("RMODE", std::move(cattr_rmode_vector));

        auto cattr_align_vector = std::vector<std::unique_ptr<asm_operand>> {};
        cattr_align_vector.push_back(std::make_unique<one_operand>(12));
        cattr_operand_align = complex_operand("ALIGN", std::move(cattr_align_vector));

        auto cattr_fill_vector = std::vector<std::unique_ptr<asm_operand>> {};
        cattr_fill_vector.push_back(std::make_unique<one_operand>(128));
        cattr_operand_fill = complex_operand("FILL", std::move(cattr_fill_vector));

        auto cattr_part_vector = std::vector<std::unique_ptr<asm_operand>> {};
        cattr_part_vector.push_back(std::make_unique<one_operand>("test_name"));
        cattr_operand_part = complex_operand("PART", std::move(cattr_part_vector));

        auto cattr_priority_vector = std::vector<std::unique_ptr<asm_operand>> {};
        cattr_priority_vector.push_back(std::make_unique<one_operand>(256));
        cattr_operand_priority = complex_operand("PRIORITY", std::move(cattr_priority_vector));

        test_cattr_true.push_back(&op_str_defload);
        test_cattr_true.push_back(&cattr_operand_rmode);
        test_cattr_true.push_back(&op_str_movable);
        test_cattr_true.push_back(&cattr_operand_align);
        test_cattr_true.push_back(&op_str_notreus);
        test_cattr_true.push_back(&cattr_operand_fill);
        test_cattr_true.push_back(&op_str_refr);
        test_cattr_true.push_back(&cattr_operand_part);
        test_cattr_true.push_back(&op_str_reus);
        test_cattr_true.push_back(&cattr_operand_priority);
        test_cattr_true.push_back(&op_str_reus);

        // expression_instruction

        test_expression_true.push_back(&op_val_8);

        // cnop


        test_cnop_true.push_back(&op_val_6);
        test_cnop_true.push_back(&op_val_8);
        test_cnop_one_false.push_back(&op_val_13);
        test_cnop_one_false.push_back(&op_val_15);
        test_cnop_two_false.push_back(&op_val_128);
        test_cnop_two_false.push_back(&op_val_128);

        // copy

        test_copy_true.push_back(&copy_first);

        // data
        test_data_true_one.push_back(&data_first_val);
        test_data_true_one.push_back(&data_second_val);
        test_data_true_one.push_back(&data_third_val);
        test_data_true_one.push_back(&data_fourth_val);

        // drop

        test_drop_true_one.push_back(&op_val_7);
        test_drop_true_one.push_back(&op_val_11);
        test_drop_true_one.push_back(&first_val);
        test_drop_true_two.push_back(&second_val);

        // end
        auto end_lang_one_vector = std::vector<std::unique_ptr<asm_operand>> {};
        end_lang_one_vector.push_back(std::make_unique<one_operand>("MYCOMPILER"));
        end_lang_one_vector.push_back(std::make_unique<one_operand>("0101", 101));
        end_lang_one_vector.push_back(std::make_unique<one_operand>("00273", 273));
        end_lang_operand_first = complex_operand("", std::move(end_lang_one_vector));

        auto end_lang_two_vector = std::vector<std::unique_ptr<asm_operand>> {};
        end_lang_two_vector.push_back(std::make_unique<one_operand>("MYCOMPILER"));
        end_lang_two_vector.push_back(std::make_unique<one_operand>("0101", 101));
        end_lang_two_vector.push_back(std::make_unique<one_operand>("00273", 273));
        end_lang_operand_second = complex_operand("", std::move(end_lang_two_vector));

        auto end_lang_false_vector = std::vector<std::unique_ptr<asm_operand>> {};
        end_lang_false_vector.push_back(std::make_unique<one_operand>("MYOWNCOMPILER"));
        end_lang_false_vector.push_back(std::make_unique<one_operand>("01010", 1010));
        end_lang_false_vector.push_back(std::make_unique<one_operand>("002737", 2737));
        end_lang_operand_false = complex_operand("", std::move(end_lang_false_vector));

        test_end_true_one.push_back(&end_one);
        test_end_true_one.push_back(&end_lang_operand_first);
        test_end_true_two.push_back(&empty_val);
        test_end_true_two.push_back(&end_lang_operand_second);
        test_end_false.push_back(&end_two);
        test_end_false.push_back(&end_lang_operand_false);

        test_equ_true_two.push_back(&op_val_16);
        test_equ_true_two.push_back(&op_val_8);
        test_equ_true_two.push_back(&op_val_128);
        test_equ_true_two.push_back(&empty_val);
        test_equ_true_two.push_back(&equ_CR32);
        // exitctl

        test_exitctl_true.push_back(&exitctl_one);
        test_exitctl_true.push_back(&zero_val);
        test_exitctl_true.push_back(&op_val_min_1024);
        test_exitctl_true.push_back(&exitctl_two);
        test_exitctl_false_one.push_back(&exitctl_three);
        test_exitctl_false_one.push_back(&op_val_15);

        // extrn

        auto extrn_vector = std::vector<std::unique_ptr<asm_operand>> {};
        extrn_vector.push_back(std::make_unique<one_operand>("test_symbol_one"));
        extrn_vector.push_back(std::make_unique<one_operand>("test_symbol_two"));
        extrn_part_operand = complex_operand("PART", std::move(extrn_vector));

        test_extrn_true_one.push_back(&extrn_test_str_one);
        test_extrn_true_one.push_back(&extrn_test_str_two);
        test_extrn_true_two.push_back(&extrn_part_operand);

        // ictl

        test_ictl_true_one.push_back(&op_val_1);
        test_ictl_true_one.push_back(&op_val_71);
        test_ictl_true_one.push_back(&op_val_16);
        test_ictl_true_two.push_back(&op_val_5);
        test_ictl_false_one.push_back(&op_val_16);
        test_ictl_false_one.push_back(&op_val_40);
        test_ictl_false_one.push_back(&op_val_16);
        test_ictl_false_two.push_back(&ictl_test_str);

        // iseq

        test_iseq_true.push_back(&op_val_40);
        test_iseq_true.push_back(&op_val_40);
        test_iseq_false.push_back(&op_val_40);
        test_iseq_false.push_back(&op_val_35);

        // mnote

        test_mnote_true_one.push_back(&op_val_128);
        test_mnote_true_one.push_back(&mnote_error_message);
        test_mnote_true_two.push_back(&mnote_asterisk);
        test_mnote_true_two.push_back(&mnote_error_message);
        test_mnote_true_three.push_back(&mnote_error_message);
        test_mnote_false_one.push_back(&op_val_256);
        test_mnote_false_one.push_back(&mnote_error_message);
        test_mnote_false_two.push_back(&mnote_asterisk);
        test_mnote_false_two.push_back(&mnote_error_message_wrong);

        // opsyn

        test_opsyn_true.push_back(&opsyn_first);

        // org

        test_org_true_one.push_back(&op_val_1024);
        test_org_true_one.push_back(&op_val_2048);
        test_org_true_one.push_back(&op_val_16);
        test_org_true_two.push_back(&op_val_16);
        test_org_true_two.push_back(&empty_val);
        test_org_true_two.push_back(&op_val_16);
        test_org_false.push_back(&op_val_1024);
        test_org_false.push_back(&op_val_1025);
        test_org_false.push_back(&op_val_16);

        // stack instructions

        test_stack_true_one.push_back(&op_str_print);
        test_stack_true_one.push_back(&op_str_using);
        test_stack_true_one.push_back(&op_str_acontrol);
        test_stack_true_one.push_back(&op_str_noprint);
        test_stack_true_two.push_back(&op_str_using);
        test_stack_false_one.push_back(&op_str_using);
        test_stack_false_one.push_back(&op_str_print);
        test_stack_false_one.push_back(&op_str_acontrol);
        test_stack_false_one.push_back(&op_str_print);
        test_stack_false_one.push_back(&op_str_noprint);
        test_stack_false_two.push_back(&empty_val);
        test_stack_false_two.push_back(&op_str_noprint);

        // print

        test_print_true.push_back(&op_str_off);
        test_print_true.push_back(&op_str_nogen);
        test_print_true.push_back(&op_str_data);
        test_print_true.push_back(&op_str_mcall);
        test_print_true.push_back(&op_str_nomcall);
        test_print_true.push_back(&op_str_msource);
        test_print_true.push_back(&op_str_nouhead);
        test_print_true.push_back(&op_str_nouhead);
        test_print_true.push_back(&op_str_noprint);

        // punch

        test_punch_true.push_back(&lorem_one);
        test_punch_false_one.push_back(&lorem_two);
        test_punch_false_two.push_back(&lorem_three);

        // rmode

        test_rmode_true_one.push_back(&rmode_64);
        test_rmode_true_two.push_back(&rmode_any);

        // using

        auto using_true_vector = std::vector<std::unique_ptr<asm_operand>> {};
        using_true_vector.push_back(std::make_unique<one_operand>(2048));
        using_true_vector.push_back(std::make_unique<one_operand>(4096));
        using_first_true_operand = complex_operand("", std::move(using_true_vector));

        auto using_false_vector = std::vector<std::unique_ptr<asm_operand>> {};
        using_false_vector.push_back(std::make_unique<one_operand>(1024));
        using_false_vector.push_back(std::make_unique<one_operand>(1024));
        using_first_false_operand = complex_operand("", std::move(using_false_vector));

        test_using_true_one.push_back(&op_val_256);
        test_using_true_one.push_back(&op_val_7);
        test_using_true_two.push_back(&op_val_2048);
        test_using_true_two.push_back(&op_val_15);
        test_using_true_two.push_back(&op_val_12);
        test_using_true_two.push_back(&op_val_12);
        test_using_true_two.push_back(&zero_val);
        test_using_true_three.push_back(&using_first_true_operand);
        test_using_true_three.push_back(&op_val_5);
        test_using_false_one.push_back(&op_val_5);
        test_using_false_one.push_back(&op_val_min_1);
        test_using_false_one.push_back(&op_val_min_1);
        test_using_false_two.push_back(&using_first_false_operand);
        test_using_false_two.push_back(&op_val_8);

        // xattr

        auto xattr_attributes_vector = std::vector<std::unique_ptr<asm_operand>> {};
        xattr_attributes_vector.push_back(std::make_unique<one_operand>("test_label"));
        xattr_attributes_operand = complex_operand("ATTR", std::move(xattr_attributes_vector));

        auto xattr_linkage_true_vector = std::vector<std::unique_ptr<asm_operand>> {};
        xattr_linkage_true_vector.push_back(std::make_unique<one_operand>("XPLINK"));
        xattr_linkage_true_operand = complex_operand("LINKAGE", std::move(xattr_linkage_true_vector));

        auto xattr_psect_vector = std::vector<std::unique_ptr<asm_operand>> {};
        xattr_psect_vector.push_back(std::make_unique<one_operand>("test_name"));
        xattr_psect_operand = complex_operand("PSECT", std::move(xattr_psect_vector));

        auto xattr_reference_true_vector = std::vector<std::unique_ptr<asm_operand>> {};
        xattr_reference_true_vector.push_back(std::make_unique<one_operand>("DIRECT"));
        xattr_reference_true_vector.push_back(std::make_unique<one_operand>("CODE"));
        xattr_reference_true_operand = complex_operand("REFERENCE", std::move(xattr_reference_true_vector));

        auto xattr_scope_vector = std::vector<std::unique_ptr<asm_operand>> {};
        xattr_scope_vector.push_back(std::make_unique<one_operand>("X"));
        xattr_scope_operand = complex_operand("SCOPE", std::move(xattr_scope_vector));

        auto xattr_reference_false_vector = std::vector<std::unique_ptr<asm_operand>> {};
        xattr_reference_false_vector.push_back(std::make_unique<one_operand>("DATA"));
        xattr_reference_false_vector.push_back(std::make_unique<one_operand>("CODE"));
        xattr_reference_false_operand = complex_operand("REFERENCE", std::move(xattr_reference_false_vector));

        auto xattr_linkage_false_vector = std::vector<std::unique_ptr<asm_operand>> {};
        xattr_linkage_false_vector.push_back(std::make_unique<one_operand>("OS"));
        xattr_linkage_false_vector.push_back(std::make_unique<one_operand>("CODE"));
        xattr_linkage_false_operand = complex_operand("LINKAGE", std::move(xattr_linkage_false_vector));

        test_xattr_true.push_back(&xattr_attributes_operand);
        test_xattr_true.push_back(&xattr_linkage_true_operand);
        test_xattr_true.push_back(&xattr_psect_operand);
        test_xattr_true.push_back(&xattr_reference_true_operand);
        test_xattr_true.push_back(&xattr_scope_operand);
        test_xattr_false_one.push_back(&xattr_reference_false_operand);
        test_xattr_false_two.push_back(&xattr_linkage_false_operand);

        // process instruction

        auto process_codepage_true_vector = std::vector<std::unique_ptr<asm_operand>> {};
        process_codepage_true_vector.push_back(std::make_unique<one_operand>("X'47C'"));
        process_codepage_true_operand = complex_operand("CP", std::move(process_codepage_true_vector));

        auto process_codepage_false_vector = std::vector<std::unique_ptr<asm_operand>> {};
        process_codepage_false_vector.push_back(std::make_unique<one_operand>(63978));
        process_codepage_false_operand = complex_operand("CP", std::move(process_codepage_false_vector));

        auto process_compat_true_vector = std::vector<std::unique_ptr<asm_operand>> {};
        process_compat_true_vector.push_back(std::make_unique<one_operand>("TRANSDT"));
        process_compat_true_vector.push_back(std::make_unique<one_operand>("NOCASE"));
        process_compat_true_vector.push_back(std::make_unique<one_operand>("LITTYPE"));
        process_compat_true_vector.push_back(std::make_unique<one_operand>("NOSYSLIST"));
        process_compat_true_vector.push_back(std::make_unique<one_operand>("TRANSDT"));
        process_compat_true_vector.push_back(std::make_unique<one_operand>("CASE"));
        process_compat_true_operand = complex_operand("CPAT", std::move(process_compat_true_vector));

        auto msg_true_vector = std::vector<std::unique_ptr<asm_operand>> {};
        msg_true_vector.push_back(std::make_unique<one_operand>(4));
        auto msg_true_operand = std::make_unique<complex_operand>("MSG", std::move(msg_true_vector));

        auto mnote_true_vector = std::vector<std::unique_ptr<asm_operand>> {};
        mnote_true_vector.push_back(std::make_unique<one_operand>(6));
        auto mnote_true_operand = std::make_unique<complex_operand>("MNOTE", std::move(mnote_true_vector));

        auto maxxers_true_vector = std::vector<std::unique_ptr<asm_operand>> {};
        maxxers_true_vector.push_back(std::make_unique<one_operand>(256));
        auto maxxers_true_operand = std::make_unique<complex_operand>("MAXXERS", std::move(maxxers_true_vector));

        auto process_fail_true_vector = std::vector<std::unique_ptr<asm_operand>> {};
        process_fail_true_vector.push_back(std::make_unique<one_operand>("NOMSG"));
        process_fail_true_vector.push_back(std::move(maxxers_true_operand));
        process_fail_true_vector.push_back(std::make_unique<one_operand>("NOMNOTE"));
        process_fail_true_vector.push_back(std::move(mnote_true_operand));
        process_fail_true_vector.push_back(std::move(msg_true_operand));
        auto process_fail_true_operand = std::make_unique<complex_operand>("FAIL", std::move(process_fail_true_vector));

        auto maxxers_false_vector = std::vector<std::unique_ptr<asm_operand>> {};
        maxxers_false_vector.push_back(std::make_unique<one_operand>(16));
        auto maxxers_false_operand = std::make_unique<complex_operand>("MAXXERS", std::move(maxxers_false_vector));

        auto process_fail_false_vector = std::vector<std::unique_ptr<asm_operand>> {};
        process_fail_false_vector.push_back(std::move(maxxers_false_operand));
        process_fail_false_operand = complex_operand("FAIL", std::move(process_fail_false_vector));

        auto process_flag_true_vector = std::vector<std::unique_ptr<asm_operand>> {};
        process_flag_true_vector.push_back(std::make_unique<one_operand>(128));
        process_flag_true_vector.push_back(std::make_unique<one_operand>("EXLITW"));
        process_flag_true_vector.push_back(std::make_unique<one_operand>("NOSUBSTR"));
        process_flag_true_vector.push_back(std::make_unique<one_operand>("PAGE0"));
        process_flag_true_vector.push_back(std::make_unique<one_operand>("NOCONT"));
        process_flag_true_operand = complex_operand("FLAG", std::move(process_flag_true_vector));

        auto process_flag_false_vector = std::vector<std::unique_ptr<asm_operand>> {};
        process_flag_false_vector.push_back(std::make_unique<one_operand>(256));
        process_flag_false_operand = complex_operand("FLAG", std::move(process_flag_false_vector));

        auto process_info_true_vector = std::vector<std::unique_ptr<asm_operand>> {};
        process_info_true_vector.push_back(std::make_unique<one_operand>(20180918));
        auto process_info_true_operand = std::make_unique<complex_operand>("INFO", std::move(process_info_true_vector));

        auto process_info_false_vector = std::vector<std::unique_ptr<asm_operand>> {};
        process_info_false_vector.push_back(std::make_unique<one_operand>(20180931));
        process_info_false_operand = complex_operand("INFO", std::move(process_info_false_vector));

        auto process_machine_true_vector = std::vector<std::unique_ptr<asm_operand>> {};
        process_machine_true_vector.push_back(std::make_unique<one_operand>("ZSERIES-7"));
        process_machine_true_vector.push_back(std::make_unique<one_operand>("LIST"));
        process_machine_true_operand = complex_operand("MAC", std::move(process_machine_true_vector));

        auto process_mxref_true_vector = std::vector<std::unique_ptr<asm_operand>> {};
        process_mxref_true_vector.push_back(std::make_unique<one_operand>("SOURCE"));
        process_mxref_true_operand = complex_operand("MX", std::move(process_mxref_true_vector));

        auto process_mxref_false_vector = std::vector<std::unique_ptr<asm_operand>> {};
        process_mxref_false_vector.push_back(std::make_unique<one_operand>("SOURCE"));
        process_mxref_false_vector.push_back(std::make_unique<one_operand>("FULL"));
        process_mxref_false_operand = complex_operand("MX", std::move(process_mxref_false_vector));

        auto process_optable_true_vector = std::vector<std::unique_ptr<asm_operand>> {};
        process_optable_true_vector.push_back(std::make_unique<one_operand>("ESA"));
        process_optable_true_vector.push_back(std::make_unique<one_operand>("NOLIST"));
        auto process_optable_true_operand =
            std::make_unique<complex_operand>("OP", std::move(process_optable_true_vector));

        auto process_pcontrol_true_vector = std::vector<std::unique_ptr<asm_operand>> {};
        process_pcontrol_true_vector.push_back(std::make_unique<one_operand>("MCALL"));
        process_pcontrol_true_vector.push_back(std::make_unique<one_operand>("ON"));
        process_pcontrol_true_vector.push_back(std::make_unique<one_operand>("OFF"));
        process_pcontrol_true_vector.push_back(std::make_unique<one_operand>("NOUHEAD"));
        process_pcontrol_true_vector.push_back(std::make_unique<one_operand>("NOGEN"));
        process_pcontrol_true_vector.push_back(std::make_unique<one_operand>("UHEAD"));
        auto process_pcontrol_true_operand =
            std::make_unique<complex_operand>("PC", std::move(process_pcontrol_true_vector));

        auto process_profile_true_vector = std::vector<std::unique_ptr<asm_operand>> {};
        process_profile_true_vector.push_back(std::make_unique<one_operand>("test_name"));
        process_profile_true_operand = complex_operand("PROF", std::move(process_profile_true_vector));

        auto process_sectalgn_true_vector = std::vector<std::unique_ptr<asm_operand>> {};
        process_sectalgn_true_vector.push_back(std::make_unique<one_operand>(8));
        auto process_sectalgn_true_operand =
            std::make_unique<complex_operand>("SECTALGN", std::move(process_sectalgn_true_vector));

        auto process_sectalgn_false_vector = std::vector<std::unique_ptr<asm_operand>> {};
        process_sectalgn_false_vector.push_back(std::make_unique<one_operand>(8192));
        process_sectalgn_false_operand = complex_operand("SECTALGN", std::move(process_sectalgn_false_vector));

        auto process_suprwarn_true_vector = std::vector<std::unique_ptr<asm_operand>> {};
        process_suprwarn_true_vector.push_back(std::make_unique<one_operand>(066));
        process_suprwarn_true_vector.push_back(std::make_unique<one_operand>(318));
        process_suprwarn_true_operand = complex_operand("NOSUP", std::move(process_suprwarn_true_vector));

        auto process_typecheck_true_vector = std::vector<std::unique_ptr<asm_operand>> {};
        process_typecheck_true_vector.push_back(std::make_unique<one_operand>("MAG"));
        process_typecheck_true_vector.push_back(std::make_unique<one_operand>("NOREG"));
        process_typecheck_true_operand = complex_operand("TYPECHECK", std::move(process_typecheck_true_vector));

        auto process_typecheck_false_vector = std::vector<std::unique_ptr<asm_operand>> {};
        process_typecheck_false_vector.push_back(std::make_unique<one_operand>("NOMAP"));
        process_typecheck_false_operand = complex_operand("TYPECHECK", std::move(process_typecheck_false_vector));

        auto process_warn_true_vector = std::vector<std::unique_ptr<asm_operand>> {};
        process_warn_true_vector.push_back(std::make_unique<one_operand>(15));
        auto process_warn_true_operand = std::make_unique<complex_operand>("WARN", std::move(process_warn_true_vector));

        auto process_limit_true_vector = std::vector<std::unique_ptr<asm_operand>> {};
        process_limit_true_vector.push_back(std::make_unique<one_operand>("X'F00'"));
        auto process_limit_true_operand =
            std::make_unique<complex_operand>("LIMIT", std::move(process_limit_true_vector));

        auto process_using_true_vector = std::vector<std::unique_ptr<asm_operand>> {};
        process_using_true_vector.push_back(std::move(process_warn_true_operand));
        process_using_true_vector.push_back(std::make_unique<one_operand>("NOMAP"));
        process_using_true_vector.push_back(std::move(process_limit_true_operand));
        process_using_true_vector.push_back(std::make_unique<one_operand>("NOWARN"));
        process_using_true_operand = complex_operand("USING", std::move(process_using_true_vector));

        auto process_limit_false_vector = std::vector<std::unique_ptr<asm_operand>> {};
        process_limit_false_vector.push_back(std::make_unique<one_operand>("X'FK0'"));
        auto process_limit_false_operand =
            std::make_unique<complex_operand>("LIMIT", std::move(process_limit_false_vector));

        auto process_using_false_vector = std::vector<std::unique_ptr<asm_operand>> {};
        process_using_false_vector.push_back(std::move(process_limit_false_operand));
        process_using_false_operand = complex_operand("USING", std::move(process_using_false_vector));

        auto process_xref_true_one_vector = std::vector<std::unique_ptr<asm_operand>> {};
        process_xref_true_one_vector.push_back(std::make_unique<one_operand>("FULL"));
        process_xref_true_one_operand = complex_operand("XREF", std::move(process_xref_true_one_vector));

        auto process_xref_true_two_vector = std::vector<std::unique_ptr<asm_operand>> {};
        process_xref_true_two_vector.push_back(std::make_unique<one_operand>("SHORT"));
        process_xref_true_two_vector.push_back(std::make_unique<one_operand>("UNREFS"));
        auto process_xref_true_two_operand =
            std::make_unique<complex_operand>("XREF", std::move(process_xref_true_two_vector));

        auto process_xref_false_vector = std::vector<std::unique_ptr<asm_operand>> {};
        process_xref_false_vector.push_back(std::make_unique<one_operand>("FULL"));
        process_xref_false_vector.push_back(std::make_unique<one_operand>("UNREFS"));
        process_xref_false_operand_p = complex_operand("XREF", std::move(process_xref_false_vector));

        auto test_process_true_overriden = std::vector<std::unique_ptr<asm_operand>> {};
        test_process_true_overriden.push_back(std::make_unique<one_operand>("NOFAIL"));
        test_process_true_overriden.push_back(std::move(process_xref_true_two_operand));
        test_process_true_overriden.push_back(std::make_unique<one_operand>("FOLD"));
        test_process_true_overriden.push_back(std::move(process_sectalgn_true_operand));
        test_process_true_overriden.push_back(std::make_unique<one_operand>("NOCOMPAT"));
        test_process_true_overriden.push_back(std::make_unique<one_operand>("NOSUPRWARN"));
        test_process_true_overriden.push_back(std::move(process_pcontrol_true_operand));
        test_process_true_overriden.push_back(std::make_unique<one_operand>("NORA2"));
        test_process_true_overriden.push_back(std::make_unique<one_operand>("ERASE"));
        test_process_true_overriden.push_back(std::move(process_info_true_operand));
        test_process_true_overriden.push_back(std::make_unique<one_operand>("NOLIBMAC"));
        test_process_true_overriden.push_back(std::move(process_optable_true_operand));
        test_process_true_overriden.push_back(std::make_unique<one_operand>("NORENT"));

        test_process_true_three_override = complex_operand("OVERRIDE", std::move(test_process_true_overriden));
        test_process_true_three.push_back(&test_process_true_three_override);

        test_process_true_one.push_back(&op_str_noalign);
        test_process_true_one.push_back(&op_str_batch);
        test_process_true_one.push_back(&op_str_dbcs);
        test_process_true_one.push_back(&op_str_dx);
        test_process_true_one.push_back(&op_str_noesd);
        test_process_true_one.push_back(&op_str_ilma);
        test_process_true_one.push_back(&op_str_info);
        test_process_true_one.push_back(&op_str_nomx);
        test_process_true_one.push_back(&op_str_nopc);
        test_process_true_one.push_back(&op_str_disk);
        test_process_true_one.push_back(&op_str_noprof);
        test_process_true_one.push_back(&op_str_rld);
        test_process_true_one.push_back(&op_str_rx);
        test_process_true_one.push_back(&op_str_seg);
        test_process_true_one.push_back(&op_str_notest);
        test_process_true_one.push_back(&op_str_thr);
        test_process_true_one.push_back(&op_str_notc);
        test_process_true_one.push_back(&op_str_nous);
        test_process_true_one.push_back(&op_str_noworkfile);
        test_process_true_one.push_back(&op_str_noxref);

        test_process_true_two.push_back(&process_using_true_operand);
        test_process_true_two.push_back(&process_profile_true_operand);
        test_process_true_two.push_back(&process_codepage_true_operand);
        test_process_true_two.push_back(&process_machine_true_operand);
        test_process_true_two.push_back(&process_compat_true_operand);
        test_process_true_two.push_back(&process_flag_true_operand);
        test_process_true_two.push_back(&process_machine_true_operand);
        test_process_true_two.push_back(&process_mxref_true_operand);
        test_process_true_two.push_back(&process_suprwarn_true_operand);
        test_process_true_two.push_back(&process_typecheck_true_operand);
        test_process_true_two.push_back(&process_xref_true_one_operand);
        test_process_false_one.push_back(&process_codepage_false_operand);
        test_process_false_two.push_back(&process_fail_false_operand);
        test_process_false_three.push_back(&process_flag_false_operand);
        test_process_false_four.push_back(&process_info_false_operand);
        test_process_false_five.push_back(&process_mxref_false_operand);
        test_process_false_six.push_back(&process_sectalgn_false_operand);
        test_process_false_seven.push_back(&process_typecheck_false_operand);
        test_process_false_eight.push_back(&process_using_false_operand);
        test_process_false_nine.push_back(&process_xref_false_operand_p);
    };

protected:
    complex_operand test_process_true_three_override;
    complex_operand process_xref_false_operand_p;
    complex_operand extrn_part_operand;
    complex_operand process_using_false_operand;
    complex_operand xattr_scope_operand;
    complex_operand xattr_refenrece_true_operand;
    complex_operand xattr_reference_false_operand;
    complex_operand xattr_attributes_operand;
    complex_operand using_first_true_operand;
    complex_operand using_first_false_operand;
    complex_operand xattr_linkage_true_operand;
    complex_operand xattr_linkage_false_operand;
    complex_operand process_xref_true_one_operand;
    complex_operand process_using_true_operand;
    complex_operand cattr_operand_rmode;
    complex_operand cattr_operand_priority;
    complex_operand end_lang_operand_first;
    complex_operand end_lang_operand_second;
    complex_operand process_profile_true_operand;
    complex_operand process_compat_true_operand;
    complex_operand process_flag_true_operand;
    complex_operand process_codepage_false_operand;
    complex_operand process_flag_false_operand;
    complex_operand xattr_reference_true_operand;
    complex_operand process_typecheck_true_operand;
    complex_operand process_typecheck_false_operand;
    complex_operand process_fail_false_operand;
    complex_operand acontrol_operand_flag;
    complex_operand acontrol_operand_typecheck;
    complex_operand acontrol_operand_optable;
    complex_operand cattr_operand_align;
    complex_operand cattr_operand_fill;
    complex_operand end_lang_operand_false;
    complex_operand process_mxref_false_operand;
    complex_operand process_machine_true_operand;
    complex_operand process_info_false_operand;
    complex_operand process_suprwarn_true_operand;
    complex_operand process_sectalgn_false_operand;
    complex_operand process_mxref_true_operand;
    complex_operand acontrol_operand_compat;
    complex_operand process_codepage_true_operand;
    complex_operand cattr_operand_part;
    complex_operand xattr_psect_operand;

    one_operand ictl_test_str = one_operand("Test string");
    one_operand end_one = one_operand("BEGIN");
    one_operand end_two = one_operand("ENTRYPT");
    one_operand copy_first = one_operand("A");
    checking::empty_operand empty_val = checking::empty_operand();
    one_operand zero_val = one_operand(0);
    one_operand op_val_1 = one_operand(1);
    one_operand op_val_5 = one_operand(5);
    one_operand op_val_6 = one_operand(6);
    one_operand op_val_7 = one_operand(7);
    one_operand op_val_8 = one_operand(8);
    one_operand op_val_11 = one_operand(11);
    one_operand op_val_12 = one_operand(12);
    one_operand op_val_13 = one_operand(13);
    one_operand op_val_15 = one_operand(15);
    one_operand op_val_16 = one_operand(16);
    one_operand op_val_18 = one_operand(18);
    one_operand op_val_35 = one_operand(35);
    one_operand op_val_40 = one_operand(40);
    one_operand op_val_71 = one_operand(71);
    one_operand op_val_128 = one_operand(128);
    one_operand op_val_256 = one_operand(256);
    one_operand op_val_287 = one_operand(287);
    one_operand op_val_1024 = one_operand(1024);
    one_operand op_val_1025 = one_operand(1025);
    one_operand op_val_2048 = one_operand(2048);
    one_operand op_val_min_1 = one_operand(-1);
    one_operand op_val_min_567 = one_operand(-567);
    one_operand op_val_min_1024 = one_operand(-1024);
    one_operand op_str_off = one_operand("OFF");
    one_operand op_str_nogen = one_operand("NOGEN");
    one_operand op_str_data = one_operand("DATA");
    one_operand op_str_mcall = one_operand("MCALL");
    one_operand op_str_nomcall = one_operand("NOMCALL");
    one_operand op_str_msource = one_operand("MSOURCE");
    one_operand op_str_nouhead = one_operand("NOUHEAD");
    one_operand op_str_noprint = one_operand("NOPRINT");
    one_operand op_str_acontrol = one_operand("ACONTROL");
    one_operand op_str_using = one_operand("USING");
    one_operand op_str_print = one_operand("PRINT");
    one_operand op_str_nocompat = one_operand("NOCOMPAT");
    one_operand op_str_lmac = one_operand("LMAC");
    one_operand op_str_ra2 = one_operand("RA2");
    one_operand op_str_notc = one_operand("NOTC");
    one_operand op_str_noalign = one_operand("NOALIGN");
    one_operand op_str_batch = one_operand("BATCH");
    one_operand op_str_dbcs = one_operand("DBCS");
    one_operand op_str_dx = one_operand("DX");
    one_operand op_str_noesd = one_operand("NOESD");
    one_operand op_str_ilma = one_operand("ILMA");
    one_operand op_str_nomx = one_operand("NOMX");
    one_operand op_str_nopc = one_operand("NOPC");
    one_operand op_str_info = one_operand("INFO");
    one_operand op_str_disk = one_operand("DISK");
    one_operand op_str_noprof = one_operand("NOPROF");
    one_operand op_str_rld = one_operand("RLD");
    one_operand op_str_rx = one_operand("RX");
    one_operand op_str_seg = one_operand("SEG");
    one_operand op_str_notest = one_operand("NOTEST");
    one_operand op_str_thr = one_operand("THR");
    one_operand op_str_nous = one_operand("NOUS");
    one_operand op_str_noworkfile = one_operand("NOWORKFILE");
    one_operand op_str_noxref = one_operand("NOXREF");
    one_operand op_str_defload = one_operand("DEFLOAD");
    one_operand op_str_movable = one_operand("MOVABLE");
    one_operand op_str_notreus = one_operand("NOTREUS");
    one_operand op_str_refr = one_operand("REFR");
    one_operand op_str_reus = one_operand("REUS");
    one_operand adata_string_val = one_operand("'temporary_string'");
    one_operand adata_string_wrong_val = one_operand("'temporary");
    one_operand op_str_alias_one = one_operand("C'lowerl'");
    one_operand op_str_alias_two = one_operand("X'9396A68599F2'");
    one_operand op_str_alias_three = one_operand("X'FF92'");
    one_operand lorem_one = one_operand("'Lorem ipsum dolor &sit amet'");
    one_operand lorem_two = one_operand("'Lorem ipsum dolor &sit amet");
    one_operand lorem_three = one_operand("'Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod "
                                          "tempor incididunt ut labore et dolore magna aliqua'");
    one_operand acontrol_afpr = one_operand("AFPR");
    one_operand opsyn_first = one_operand("STH");
    one_operand ainsert_one = one_operand("'test string'");
    one_operand ainsert_back = one_operand("BACK");
    one_operand ainsert_front = one_operand("FRONT");
    one_operand ainsert_lorem =
        one_operand("'Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmo magna'");
    one_operand ainsert_apo = one_operand("'");
    one_operand rmode_64 = one_operand(64);
    one_operand rmode_any = one_operand("ANY");
    one_operand data_first_val = one_operand(" FS4'-10,25.3,U268435455'");
    one_operand data_second_val = one_operand("DL7S3E50'2.7182'");
    one_operand data_third_val = one_operand("FP(Rate5)'35.92'");
    one_operand data_fourth_val = one_operand("3XP(C'APC')'FF'");
    one_operand data_fifth_val = one_operand("10EBP(7)L2'12'");
    one_operand amode_any31 = one_operand("ANY31");
    one_operand mnote_error_message = one_operand("'Error message text'");
    one_operand mnote_asterisk = one_operand("*");
    one_operand mnote_error_message_wrong = one_operand("'Error message text");

    one_operand exitctl_one = one_operand("SOURCE");
    one_operand exitctl_two = one_operand(2147483647);
    one_operand exitctl_three = one_operand("SOURCES");
    one_operand exitctl_four = one_operand("LISTING");
    one_operand first_val = one_operand("FIRST");
    one_operand second_val = one_operand("SECOND");
    one_operand drop_lorem_val = one_operand("Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod "
                                             "tempor incididunt ut labore et dolore magna aliqua");
    one_operand extrn_test_str_one = one_operand("test_symbol_one");
    one_operand extrn_test_str_two = one_operand("test_symbol_two");
    one_operand equ_CR32 = one_operand("CR32");
    one_operand equ_A1 = one_operand("A1");

    diagnostic_collector collector;

    std::vector<const checking::asm_operand*> test_no_operand_true = std::vector<const checking::asm_operand*>();
    std::vector<const checking::asm_operand*> test_adata_true_one = std::vector<const checking::asm_operand*>();
    std::vector<const checking::asm_operand*> test_adata_true_two = std::vector<const checking::asm_operand*>();
    std::vector<const checking::asm_operand*> test_acontrol_true = std::vector<const checking::asm_operand*>();
    std::vector<const checking::asm_operand*> test_ainsert_true_one = std::vector<const checking::asm_operand*>();
    std::vector<const checking::asm_operand*> test_ainsert_true_two = std::vector<const checking::asm_operand*>();
    std::vector<const checking::asm_operand*> test_ainsert_false = std::vector<const checking::asm_operand*>();
    std::vector<const checking::asm_operand*> test_alias_true_one = std::vector<const checking::asm_operand*>();
    std::vector<const checking::asm_operand*> test_alias_true_two = std::vector<const checking::asm_operand*>();
    std::vector<const checking::asm_operand*> test_alias_false = std::vector<const checking::asm_operand*>();
    std::vector<const checking::asm_operand*> test_amode_true = std::vector<const checking::asm_operand*>();
    std::vector<const checking::asm_operand*> test_cattr_true = std::vector<const checking::asm_operand*>();
    std::vector<const checking::asm_operand*> test_expression_true = std::vector<const checking::asm_operand*>();

    std::vector<const checking::asm_operand*> test_cnop_true = std::vector<const checking::asm_operand*>();
    std::vector<const checking::asm_operand*> test_cnop_one_false = std::vector<const checking::asm_operand*>();
    std::vector<const checking::asm_operand*> test_cnop_two_false = std::vector<const checking::asm_operand*>();
    std::vector<const checking::asm_operand*> test_copy_true = std::vector<const checking::asm_operand*>();
    std::vector<const checking::asm_operand*> test_data_true_one = std::vector<const checking::asm_operand*>();
    std::vector<const checking::asm_operand*> test_data_true_two = std::vector<const checking::asm_operand*>();
    std::vector<const checking::asm_operand*> test_drop_true_one = std::vector<const checking::asm_operand*>();
    std::vector<const checking::asm_operand*> test_drop_true_two = std::vector<const checking::asm_operand*>();
    std::vector<const checking::asm_operand*> test_end_true_one = std::vector<const checking::asm_operand*>();
    std::vector<const checking::asm_operand*> test_end_true_two = std::vector<const checking::asm_operand*>();
    std::vector<const checking::asm_operand*> test_end_false = std::vector<const checking::asm_operand*>();
    std::vector<const checking::asm_operand*> test_equ_true_two = std::vector<const checking::asm_operand*>();
    std::vector<const checking::asm_operand*> test_exitctl_true = std::vector<const checking::asm_operand*>();
    std::vector<const checking::asm_operand*> test_exitctl_false_one = std::vector<const checking::asm_operand*>();
    std::vector<const checking::asm_operand*> test_extrn_true_one = std::vector<const checking::asm_operand*>();
    std::vector<const checking::asm_operand*> test_extrn_true_two = std::vector<const checking::asm_operand*>();
    std::vector<const checking::asm_operand*> test_ictl_true_one = std::vector<const checking::asm_operand*>();
    std::vector<const checking::asm_operand*> test_ictl_true_two = std::vector<const checking::asm_operand*>();
    std::vector<const checking::asm_operand*> test_ictl_false_one = std::vector<const checking::asm_operand*>();
    std::vector<const checking::asm_operand*> test_ictl_false_two = std::vector<const checking::asm_operand*>();
    std::vector<const checking::asm_operand*> test_iseq_true = std::vector<const checking::asm_operand*>();
    std::vector<const checking::asm_operand*> test_iseq_false = std::vector<const checking::asm_operand*>();
    std::vector<const checking::asm_operand*> test_mnote_true_one = std::vector<const checking::asm_operand*>();
    std::vector<const checking::asm_operand*> test_mnote_true_two = std::vector<const checking::asm_operand*>();
    std::vector<const checking::asm_operand*> test_mnote_true_three = std::vector<const checking::asm_operand*>();
    std::vector<const checking::asm_operand*> test_mnote_false_one = std::vector<const checking::asm_operand*>();
    std::vector<const checking::asm_operand*> test_mnote_false_two = std::vector<const checking::asm_operand*>();
    std::vector<const checking::asm_operand*> test_opsyn_true = std::vector<const checking::asm_operand*>();
    std::vector<const checking::asm_operand*> test_org_true_one = std::vector<const checking::asm_operand*>();
    std::vector<const checking::asm_operand*> test_org_true_two = std::vector<const checking::asm_operand*>();
    std::vector<const checking::asm_operand*> test_org_false = std::vector<const checking::asm_operand*>();
    std::vector<const checking::asm_operand*> test_stack_true_one = std::vector<const checking::asm_operand*>();
    std::vector<const checking::asm_operand*> test_stack_true_two = std::vector<const checking::asm_operand*>();
    std::vector<const checking::asm_operand*> test_stack_false_one = std::vector<const checking::asm_operand*>();
    std::vector<const checking::asm_operand*> test_stack_false_two = std::vector<const checking::asm_operand*>();
    std::vector<const checking::asm_operand*> test_print_true = std::vector<const checking::asm_operand*>();
    std::vector<const checking::asm_operand*> test_punch_true = std::vector<const checking::asm_operand*>();
    std::vector<const checking::asm_operand*> test_punch_false_one = std::vector<const checking::asm_operand*>();
    std::vector<const checking::asm_operand*> test_punch_false_two = std::vector<const checking::asm_operand*>();
    std::vector<const checking::asm_operand*> test_rmode_true_one = std::vector<const checking::asm_operand*>();
    std::vector<const checking::asm_operand*> test_rmode_true_two = std::vector<const checking::asm_operand*>();
    std::vector<const checking::asm_operand*> test_using_true_one = std::vector<const checking::asm_operand*>();
    std::vector<const checking::asm_operand*> test_using_true_two = std::vector<const checking::asm_operand*>();
    std::vector<const checking::asm_operand*> test_using_true_three = std::vector<const checking::asm_operand*>();
    std::vector<const checking::asm_operand*> test_using_false_one = std::vector<const checking::asm_operand*>();
    std::vector<const checking::asm_operand*> test_using_false_two = std::vector<const checking::asm_operand*>();
    std::vector<const checking::asm_operand*> test_xattr_true = std::vector<const checking::asm_operand*>();
    std::vector<const checking::asm_operand*> test_xattr_false_one = std::vector<const checking::asm_operand*>();
    std::vector<const checking::asm_operand*> test_xattr_false_two = std::vector<const checking::asm_operand*>();
    std::vector<const checking::asm_operand*> test_process_true_one = std::vector<const checking::asm_operand*>();
    std::vector<const checking::asm_operand*> test_process_true_two = std::vector<const checking::asm_operand*>();
    std::vector<const checking::asm_operand*> test_process_true_three = std::vector<const checking::asm_operand*>();
    std::vector<const checking::asm_operand*> test_process_false_one = std::vector<const checking::asm_operand*>();
    std::vector<const checking::asm_operand*> test_process_false_two = std::vector<const checking::asm_operand*>();
    std::vector<const checking::asm_operand*> test_process_false_three = std::vector<const checking::asm_operand*>();
    std::vector<const checking::asm_operand*> test_process_false_four = std::vector<const checking::asm_operand*>();
    std::vector<const checking::asm_operand*> test_process_false_five = std::vector<const checking::asm_operand*>();
    std::vector<const checking::asm_operand*> test_process_false_six = std::vector<const checking::asm_operand*>();
    std::vector<const checking::asm_operand*> test_process_false_seven = std::vector<const checking::asm_operand*>();
    std::vector<const checking::asm_operand*> test_process_false_eight = std::vector<const checking::asm_operand*>();
    std::vector<const checking::asm_operand*> test_process_false_nine = std::vector<const checking::asm_operand*>();
    std::vector<const checking::asm_operand*> test_entry_false = std::vector<const checking::asm_operand*>();
};

TEST_F(instruction_test, process)
{
    EXPECT_FALSE(check_asm_ops("*PROCESS", test_no_operand_true, range(), collector));
    EXPECT_TRUE(check_asm_ops("*PROCESS", test_process_true_one, range(), collector));
    EXPECT_TRUE(check_asm_ops("*PROCESS", test_process_true_two, range(), collector));
    EXPECT_TRUE(check_asm_ops("*PROCESS", test_process_true_three, range(), collector));
    EXPECT_FALSE(check_asm_ops("*PROCESS", test_process_false_one, range(), collector));
    EXPECT_FALSE(check_asm_ops("*PROCESS", test_process_false_two, range(), collector));
    EXPECT_FALSE(check_asm_ops("*PROCESS", test_process_false_three, range(), collector));
    EXPECT_FALSE(check_asm_ops("*PROCESS", test_process_false_four, range(), collector));
    EXPECT_FALSE(check_asm_ops("*PROCESS", test_process_false_five, range(), collector));
    EXPECT_FALSE(check_asm_ops("*PROCESS", test_process_false_six, range(), collector));
    EXPECT_FALSE(check_asm_ops("*PROCESS", test_process_false_seven, range(), collector));
    EXPECT_FALSE(check_asm_ops("*PROCESS", test_process_false_eight, range(), collector));
    EXPECT_FALSE(check_asm_ops("*PROCESS", test_process_false_nine, range(), collector));
}

TEST_F(instruction_test, no_operand)
{
    EXPECT_TRUE(check_asm_ops("LOCTR", test_no_operand_true, range(), collector));
    EXPECT_FALSE(check_asm_ops("LOCTR", test_acontrol_true, range(), collector));
}

TEST_F(instruction_test, adata)
{
    EXPECT_FALSE(check_asm_ops("ADATA", test_no_operand_true, range(), collector));
    EXPECT_TRUE(check_asm_ops("ADATA", test_adata_true_one, range(), collector));
    EXPECT_TRUE(check_asm_ops("ADATA", test_adata_true_two, range(), collector));
}

TEST_F(instruction_test, acontrol)
{
    EXPECT_FALSE(check_asm_ops("ACONTROL", test_no_operand_true, range(), collector));
    EXPECT_TRUE(check_asm_ops("ACONTROL", test_acontrol_true, range(), collector));
}

TEST_F(instruction_test, ainsert)
{
    EXPECT_TRUE(check_asm_ops("AINSERT", test_ainsert_true_one, range(), collector));
    EXPECT_TRUE(check_asm_ops("AINSERT", test_ainsert_true_two, range(), collector));
    EXPECT_FALSE(check_asm_ops("AINSERT", test_ainsert_false, range(), collector));
    EXPECT_FALSE(check_asm_ops("AINSERT", test_no_operand_true, range(), collector));
}

TEST_F(instruction_test, alias)
{
    EXPECT_FALSE(check_asm_ops("ALIAS", test_alias_false, range(), collector));
    EXPECT_TRUE(check_asm_ops("ALIAS", test_alias_true_one, range(), collector));
    EXPECT_TRUE(check_asm_ops("ALIAS", test_alias_true_two, range(), collector));
    EXPECT_FALSE(check_asm_ops("ALIAS", test_acontrol_true, range(), collector));
}

TEST_F(instruction_test, amode)
{
    EXPECT_FALSE(check_asm_ops("AMODE", test_no_operand_true, range(), collector));
    EXPECT_TRUE(check_asm_ops("AMODE", test_amode_true, range(), collector));
    EXPECT_FALSE(check_asm_ops("AMODE", test_alias_true_one, range(), collector));
}

TEST_F(instruction_test, cattr)
{
    EXPECT_TRUE(check_asm_ops("CATTR", test_no_operand_true, range(), collector));
    EXPECT_TRUE(check_asm_ops("CATTR", test_cattr_true, range(), collector));
}

TEST_F(instruction_test, expression)
{
    EXPECT_TRUE(check_asm_ops("CEJECT", test_no_operand_true, range(), collector));
    EXPECT_FALSE(check_asm_ops("CEJECT", test_acontrol_true, range(), collector));
    EXPECT_TRUE(check_asm_ops("CEJECT", test_expression_true, range(), collector));
}

TEST_F(instruction_test, ccw)
{
    EXPECT_FALSE(check_asm_ops("CCW", test_no_operand_true, range(), collector));

    one_operand ccw_first("X'06'", 6);
    // Current value of relocatable symbols for checker is one_operand("0", 0), see
    // expr_assembler_operand::get_operand_value_inner
    one_operand relocatable("0", 0);
    one_operand big_absolute("17000000", 17000000);
    one_operand ccw_third("X'40'", 64);
    one_operand ccw_fourth("MyBlkSize", 20);
    std::vector<const checking::asm_operand*> test_ccw_true { &ccw_first, &relocatable, &ccw_third, &ccw_fourth };
    EXPECT_TRUE(check_asm_ops("CCW", test_ccw_true, range(), collector));

    std::vector<const checking::asm_operand*> ccw_big_absolute_address {
        &ccw_first, &big_absolute, &ccw_third, &ccw_fourth
    };
    EXPECT_FALSE(check_asm_ops("CCW", ccw_big_absolute_address, range(), collector));
    EXPECT_TRUE(check_asm_ops("CCW1", ccw_big_absolute_address, range(), collector));

    EXPECT_FALSE(check_asm_ops("CCW", test_expression_true, range(), collector));
    EXPECT_FALSE(check_asm_ops("CCW", test_acontrol_true, range(), collector));
}

TEST_F(instruction_test, cnop)
{
    EXPECT_FALSE(check_asm_ops("CNOP", test_no_operand_true, range(), collector));
    EXPECT_TRUE(check_asm_ops("CNOP", test_cnop_true, range(), collector));
    EXPECT_FALSE(check_asm_ops("CNOP", test_cnop_one_false, range(), collector));
    EXPECT_FALSE(check_asm_ops("CNOP", test_cnop_two_false, range(), collector));
    EXPECT_FALSE(check_asm_ops("CNOP", test_acontrol_true, range(), collector));
}

TEST_F(instruction_test, copy)
{
    EXPECT_FALSE(check_asm_ops("COPY", test_no_operand_true, range(), collector));
    EXPECT_TRUE(check_asm_ops("COPY", test_copy_true, range(), collector));
    EXPECT_FALSE(check_asm_ops("COPY", test_acontrol_true, range(), collector));
}

TEST_F(instruction_test, drop)
{
    EXPECT_TRUE(check_asm_ops("DROP", test_no_operand_true, range(), collector));
    EXPECT_TRUE(check_asm_ops("DROP", test_drop_true_one, range(), collector));
    EXPECT_TRUE(check_asm_ops("DROP", test_drop_true_two, range(), collector));
    EXPECT_FALSE(check_asm_ops("DROP", test_acontrol_true, range(), collector));
}

TEST_F(instruction_test, end)
{
    EXPECT_TRUE(check_asm_ops("END", test_no_operand_true, range(), collector));
    EXPECT_TRUE(check_asm_ops("END", test_end_true_one, range(), collector));
    EXPECT_TRUE(check_asm_ops("END", test_end_true_two, range(), collector));
    EXPECT_FALSE(check_asm_ops("END", test_end_false, range(), collector));
    EXPECT_FALSE(check_asm_ops("END", test_exitctl_true, range(), collector));
}

TEST_F(instruction_test, entry)
{
    EXPECT_FALSE(check_asm_ops("ENTRY", test_no_operand_true, range(), collector));
    EXPECT_TRUE(check_asm_ops("ENTRY", test_data_true_one, range(), collector));
    EXPECT_TRUE(check_asm_ops("ENTRY", test_ainsert_true_one, range(), collector));
    EXPECT_FALSE(check_asm_ops("ENTRY", test_entry_false, range(), collector));
}
TEST_F(instruction_test, exitctl)
{
    EXPECT_FALSE(check_asm_ops("EXITCTL", test_no_operand_true, range(), collector));
    EXPECT_TRUE(check_asm_ops("EXITCTL", test_exitctl_true, range(), collector));
    EXPECT_FALSE(check_asm_ops("EXITCTL", test_exitctl_false_one, range(), collector));
    EXPECT_FALSE(check_asm_ops("EXITCTL", test_exitctl_false_one, range(), collector));
}

TEST_F(instruction_test, external)
{
    EXPECT_FALSE(check_asm_ops("EXTRN", test_no_operand_true, range(), collector));
    EXPECT_TRUE(check_asm_ops("EXTRN", test_extrn_true_one, range(), collector));
    EXPECT_TRUE(check_asm_ops("EXTRN", test_extrn_true_two, range(), collector));
}

TEST_F(instruction_test, ictl)
{
    EXPECT_FALSE(check_asm_ops("ICTL", test_no_operand_true, range(), collector));
    EXPECT_TRUE(check_asm_ops("ICTL", test_ictl_true_one, range(), collector));
    EXPECT_TRUE(check_asm_ops("ICTL", test_ictl_true_two, range(), collector));
    EXPECT_FALSE(check_asm_ops("ICTL", test_ictl_false_one, range(), collector));
    EXPECT_FALSE(check_asm_ops("ICTL", test_ictl_false_two, range(), collector));
}

TEST_F(instruction_test, iseq)
{
    EXPECT_TRUE(check_asm_ops("ISEQ", test_no_operand_true, range(), collector));
    EXPECT_TRUE(check_asm_ops("ISEQ", test_iseq_true, range(), collector));
    EXPECT_FALSE(check_asm_ops("ISEQ", test_iseq_false, range(), collector));
    EXPECT_FALSE(check_asm_ops("ISEQ", test_ainsert_true_one, range(), collector));
    EXPECT_FALSE(check_asm_ops("ISEQ", test_extrn_true_one, range(), collector));
}

TEST_F(instruction_test, mnote)
{
    EXPECT_FALSE(check_asm_ops("MNOTE", test_no_operand_true, range(), collector));
    EXPECT_TRUE(check_asm_ops("MNOTE", test_mnote_true_one, range(), collector));
    EXPECT_TRUE(check_asm_ops("MNOTE", test_mnote_true_two, range(), collector));
    EXPECT_TRUE(check_asm_ops("MNOTE", test_mnote_true_three, range(), collector));
    EXPECT_FALSE(check_asm_ops("MNOTE", test_mnote_false_one, range(), collector));
    EXPECT_FALSE(check_asm_ops("MNOTE", test_mnote_false_two, range(), collector));
}

TEST_F(instruction_test, opsyn)
{
    EXPECT_TRUE(check_asm_ops("OPSYN", test_no_operand_true, range(), collector));
    EXPECT_TRUE(check_asm_ops("OPSYN", test_opsyn_true, range(), collector));
    EXPECT_FALSE(check_asm_ops("OPSYN", test_mnote_true_one, range(), collector));
}

TEST_F(instruction_test, org)
{
    EXPECT_TRUE(check_asm_ops("ORG", test_no_operand_true, range(), collector));
    EXPECT_TRUE(check_asm_ops("ORG", test_org_true_one, range(), collector));
    EXPECT_TRUE(check_asm_ops("ORG", test_org_true_two, range(), collector));
    EXPECT_FALSE(check_asm_ops("ORG", test_org_false, range(), collector));
    EXPECT_FALSE(check_asm_ops("ORG", test_equ_true_two, range(), collector));
}

TEST_F(instruction_test, stack)
{
    EXPECT_FALSE(check_asm_ops("POP", test_no_operand_true, range(), collector));
    EXPECT_TRUE(check_asm_ops("POP", test_stack_true_one, range(), collector));
    EXPECT_TRUE(check_asm_ops("POP", test_stack_true_two, range(), collector));
    EXPECT_FALSE(check_asm_ops("POP", test_stack_false_one, range(), collector));
    EXPECT_FALSE(check_asm_ops("POP", test_stack_false_two, range(), collector));
    EXPECT_FALSE(check_asm_ops("POP", test_extrn_true_two, range(), collector));
}

TEST_F(instruction_test, print)
{
    EXPECT_FALSE(check_asm_ops("PRINT", test_no_operand_true, range(), collector));
    EXPECT_TRUE(check_asm_ops("PRINT", test_print_true, range(), collector));
    EXPECT_FALSE(check_asm_ops("PRINT", test_stack_true_one, range(), collector));
    EXPECT_FALSE(check_asm_ops("PRINT", test_extrn_true_two, range(), collector));
}

TEST_F(instruction_test, rmode)
{
    EXPECT_FALSE(check_asm_ops("RMODE", test_no_operand_true, range(), collector));
    EXPECT_TRUE(check_asm_ops("RMODE", test_rmode_true_one, range(), collector));
    EXPECT_TRUE(check_asm_ops("RMODE", test_rmode_true_two, range(), collector));
    EXPECT_FALSE(check_asm_ops("RMODE", test_amode_true, range(), collector));
    EXPECT_FALSE(check_asm_ops("RMODE", test_extrn_true_two, range(), collector));
}

TEST_F(instruction_test, title)
{
    EXPECT_FALSE(check_asm_ops("TITLE", test_no_operand_true, range(), collector));
    EXPECT_TRUE(check_asm_ops("TITLE", test_punch_true, range(), collector));
    EXPECT_FALSE(check_asm_ops("TITLE", test_punch_false_two, range(), collector));
    EXPECT_FALSE(check_asm_ops("TITLE", test_punch_false_one, range(), collector));
    EXPECT_FALSE(check_asm_ops("TITLE", test_amode_true, range(), collector));
    EXPECT_FALSE(check_asm_ops("TITLE", test_extrn_true_two, range(), collector));
}

TEST_F(instruction_test, using_instr)
{
    EXPECT_FALSE(check_asm_ops("USING", test_no_operand_true, range(), collector));
    /*

    TO DO once using is resolved

    EXPECT_FALSE(check_assembler_instruction("USING", test_extrn_true_two, range(), collector));
    EXPECT_TRUE(check_assembler_instruction("USING", test_using_true_one, range(), collector));
    EXPECT_TRUE(check_assembler_instruction("USING", test_using_true_two, range(), collector));
    EXPECT_TRUE(check_assembler_instruction("USING", test_using_true_three, range(), collector));
    EXPECT_FALSE(check_assembler_instruction("USING", test_using_false_one, range(), collector));
    EXPECT_FALSE(check_assembler_instruction("USING", test_using_false_two, range(), collector));
    EXPECT_FALSE(check_assembler_instruction("USING", test_rmode_true_one, range(), collector)); */
}

TEST_F(instruction_test, xattr)
{
    EXPECT_FALSE(check_asm_ops("XATTR", test_no_operand_true, range(), collector));
    EXPECT_TRUE(check_asm_ops("XATTR", test_xattr_true, range(), collector));
    EXPECT_FALSE(check_asm_ops("XATTR", test_xattr_false_one, range(), collector));
    EXPECT_FALSE(check_asm_ops("XATTR", test_xattr_false_two, range(), collector));
    EXPECT_FALSE(check_asm_ops("XATTR", test_extrn_true_two, range(), collector));
    EXPECT_FALSE(check_asm_ops("XATTR", test_acontrol_true, range(), collector));
}
