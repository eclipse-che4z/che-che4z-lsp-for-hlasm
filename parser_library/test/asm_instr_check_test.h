#pragma once
#include "../src/checking/instruction_checker.h"

using namespace hlasm_plugin::parser_library::checking;

class instruction_test : public testing::Test
{
public:
	virtual void SetUp(std::string param) {}
	virtual void TearDown() {}
	instruction_test(){
		test_adata_true_one.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("287"));
		test_adata_true_one.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>(""));
		test_adata_true_one.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("-567"));
		test_adata_true_one.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("0"));
		test_adata_true_one.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("'temporary_string'"));

		test_adata_true_two.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("0"));
		test_adata_true_two.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>(""));
		test_adata_true_two.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>(""));
		test_adata_true_two.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>(""));
		test_adata_true_two.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>(""));

		test_adata_false.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>(""));
		test_adata_false.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>(""));
		test_adata_false.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>(""));
		test_adata_false.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("21474836484"));
		test_adata_false.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("'temporary"));

		// acontrol instruction

		auto acontrol_compat_vector = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>{};
		acontrol_compat_vector.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("CASE"));
		acontrol_compat_vector.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("NOMC"));
		acontrol_compat_vector.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("NOTRS"));
		acontrol_compat_vector.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("NOTRANSDT"));
		acontrol_compat_vector.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("NOTRANSDT"));
		auto acontrol_operand_compat = std::make_unique<hlasm_plugin::parser_library::checking::complex_operand>("COMPAT", std::move(acontrol_compat_vector));

		auto acontrol_flag_vector = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>{};
		acontrol_flag_vector.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("2"));
		acontrol_flag_vector.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("AL"));
		acontrol_flag_vector.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("NOCONT"));
		acontrol_flag_vector.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("IMPLEN"));
		acontrol_flag_vector.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("NOUSING0"));
		auto acontrol_operand_flag = std::make_unique<hlasm_plugin::parser_library::checking::complex_operand>("FLAG", std::move(acontrol_flag_vector));

		auto acontrol_typecheck_vector = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>{};
		acontrol_typecheck_vector.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("MAGNITUDE"));
		acontrol_typecheck_vector.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("NOREGISTER"));
		acontrol_typecheck_vector.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("REGISTER"));
		auto acontrol_operand_typecheck = std::make_unique<hlasm_plugin::parser_library::checking::complex_operand>("TYPECHECK", std::move(acontrol_typecheck_vector));

		auto acontrol_optable_vector = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>{};
		acontrol_optable_vector.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("ZS6"));
		acontrol_optable_vector.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("LIST"));
		auto acontrol_operand_optable = std::make_unique<hlasm_plugin::parser_library::checking::complex_operand>("OPTABLE", std::move(acontrol_optable_vector));

		test_acontrol_true.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("AFPR"));
		test_acontrol_true.push_back(std::move(acontrol_operand_compat));
		test_acontrol_true.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("NOCOMPAT"));
		test_acontrol_true.push_back(std::move(acontrol_operand_flag));
		test_acontrol_true.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("LMAC"));
		test_acontrol_true.push_back(std::move(acontrol_operand_typecheck));
		test_acontrol_true.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("RA2"));
		test_acontrol_true.push_back(std::move(acontrol_operand_optable));
		test_acontrol_true.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("NOTC"));

		// ainsert instruction

		test_ainsert_true_one.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("'test string'"));
		test_ainsert_true_one.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("BACK"));

		test_ainsert_true_two.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("'Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.'"));
		test_ainsert_true_two.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("FRONT"));

		test_ainsert_false.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("'"));
		test_ainsert_false.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("FRONT"));

		//alias instruction

		test_alias_true_one.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("C'lowerl'"));

		test_alias_true_two.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("X'9396A68599F2'"));

		test_alias_false.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("X'FF92'"));

		// amode instruction

		test_amode_true.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("ANY31"));

		// cattr instruction

		auto cattr_rmode_vector = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>{};
		cattr_rmode_vector.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("ANY"));
		auto cattr_operand_rmode = std::make_unique<hlasm_plugin::parser_library::checking::complex_operand>("RMODE", std::move(cattr_rmode_vector));

		auto cattr_align_vector = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>{};
		cattr_align_vector.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("12"));
		auto cattr_operand_align = std::make_unique<hlasm_plugin::parser_library::checking::complex_operand>("ALIGN", std::move(cattr_align_vector));

		auto cattr_fill_vector = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>{};
		cattr_fill_vector.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("128"));
		auto cattr_operand_fill = std::make_unique<hlasm_plugin::parser_library::checking::complex_operand>("FILL", std::move(cattr_fill_vector));

		auto cattr_part_vector = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>{};
		cattr_part_vector.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("test string"));
		auto cattr_operand_part = std::make_unique<hlasm_plugin::parser_library::checking::complex_operand>("PART", std::move(cattr_part_vector));

		auto cattr_priority_vector = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>{};
		cattr_priority_vector.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("256"));
		auto cattr_operand_priority = std::make_unique<hlasm_plugin::parser_library::checking::complex_operand>("PRIORITY", std::move(cattr_priority_vector));

		test_cattr_true.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("DEFLOAD"));
		test_cattr_true.push_back(std::move(cattr_operand_rmode));
		test_cattr_true.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("MOVABLE"));
		test_cattr_true.push_back(std::move(cattr_operand_align));
		test_cattr_true.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("NOTREUS"));
		test_cattr_true.push_back(std::move(cattr_operand_fill));
		test_cattr_true.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("REFR"));
		test_cattr_true.push_back(std::move(cattr_operand_part));
		test_cattr_true.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("REUS"));
		test_cattr_true.push_back(std::move(cattr_operand_priority));
		test_cattr_true.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("REUS"));

		// expression_instruction 

		test_expression_true.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("8"));

		// ccw

		test_ccw_true.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("X'06'"));
		test_ccw_true.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("MyData"));
		test_ccw_true.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("X'40'"));
		test_ccw_true.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("MyBlkSize"));

		// cnop

		test_cnop_true.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("6"));
		test_cnop_true.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("8"));
		test_cnop_one_false.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("13"));
		test_cnop_one_false.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("15"));
		test_cnop_two_false.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("128"));
		test_cnop_two_false.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("128"));

		// copy

		test_copy_true.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("A"));

		// data

		test_data_true_one.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>(" FS4'-10,25.3,U268435455'"));
		test_data_true_one.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("DL7S3E50'2.7182'"));
		test_data_true_one.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("FP(Rate5)'35.92'"));
		test_data_true_one.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("3XP(C'APC')'FF'"));
		test_data_true_two.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("10EBP(7)L2'12'"));

		// drop
		test_drop_true_one.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("7"));
		test_drop_true_one.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("11"));
		test_drop_true_one.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("FIRST"));
		test_drop_true_two.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("SECOND"));
		test_drop_false.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("FIRST"));
		test_drop_false.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua"));

		// end
		auto end_lang_one_vector = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>{};
		end_lang_one_vector.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("MYCOMPILER"));
		end_lang_one_vector.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("0101"));
		end_lang_one_vector.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("00273"));
		auto end_lang_operand_first = std::make_unique<hlasm_plugin::parser_library::checking::complex_operand>("", std::move(end_lang_one_vector));

		auto end_lang_two_vector = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>{};
		end_lang_two_vector.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("MYCOMPILER"));
		end_lang_two_vector.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("0101"));
		end_lang_two_vector.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("00273"));
		auto end_lang_operand_second = std::make_unique<hlasm_plugin::parser_library::checking::complex_operand>("", std::move(end_lang_two_vector));

		auto end_lang_false_vector = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>{};
		end_lang_false_vector.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("MYOWNCOMPILER"));
		end_lang_false_vector.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("01010"));
		end_lang_false_vector.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("002737"));
		auto end_lang_operand_false = std::make_unique<hlasm_plugin::parser_library::checking::complex_operand>("", std::move(end_lang_false_vector));

		test_end_true_one.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("BEGIN"));
		test_end_true_one.push_back(std::move(end_lang_operand_first));
		test_end_true_two.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>(""));
		test_end_true_two.push_back(std::move(end_lang_operand_second));
		test_end_false.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("ENTRYPT"));
		test_end_false.push_back(std::move(end_lang_operand_false));

		// equ

		test_equ_true_one.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("256"));
		test_equ_true_one.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("40"));
		test_equ_true_one.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("128"));
		test_equ_true_one.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("128"));
		test_equ_true_one.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("CR32"));

		test_equ_true_two.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("16"));
		test_equ_true_two.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("8"));
		test_equ_true_two.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("128"));
		test_equ_true_two.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>(""));
		test_equ_true_two.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("CR32"));

		test_equ_false.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("A1"));
		test_equ_false.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("256"));
		test_equ_false.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("256"));
		test_equ_false.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("256"));
		test_equ_false.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("18"));

		// exitctl

		test_exitctl_true.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("SOURCE"));
		test_exitctl_true.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("0"));
		test_exitctl_true.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("-1024"));
		test_exitctl_true.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("2147483647"));
		test_exitctl_false_one.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("SOURCES"));
		test_exitctl_false_one.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("15"));
		test_exitctl_false_two.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("LISTING"));
		test_exitctl_false_two.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("21474836489"));
		test_exitctl_false_two.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("128"));

		// extrn

		auto extrn_vector = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>{};
		extrn_vector.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("test_symbol_one"));
		extrn_vector.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("test_symbol_two"));
		auto extrn_part_operand = std::make_unique<hlasm_plugin::parser_library::checking::complex_operand>("PART", std::move(extrn_vector));

		test_extrn_true_one.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("test_symbol_one"));
		test_extrn_true_one.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("test_symbol_two"));
		test_extrn_true_two.push_back(std::move(extrn_part_operand));

		// ictl
		test_ictl_true_one.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("1"));
		test_ictl_true_one.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("71"));
		test_ictl_true_one.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("16"));
		test_ictl_true_two.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("5"));
		test_ictl_false_one.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("16"));
		test_ictl_false_one.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("40"));
		test_ictl_false_one.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("16"));
		test_ictl_false_two.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("Test string"));

		// iseq

		test_iseq_true.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("40"));
		test_iseq_true.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("40"));
		test_iseq_false.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("40"));
		test_iseq_false.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("35"));

		// mnote

		test_mnote_true_one.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("128"));
		test_mnote_true_one.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("'Error message text'"));
		test_mnote_true_two.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("*"));
		test_mnote_true_two.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("'Error message text'"));
		test_mnote_false_one.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("256"));
		test_mnote_false_one.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("'Error message text'"));
		test_mnote_false_two.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("*"));
		test_mnote_false_two.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("'Error message text"));

		// opsyn

		test_opsyn_true.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("STH"));

		// org

		test_org_true_one.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("1024"));
		test_org_true_one.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("2048"));
		test_org_true_one.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("16"));
		test_org_true_two.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("16"));
		test_org_true_two.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>(""));
		test_org_true_two.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("16"));
		test_org_false.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("1024"));
		test_org_false.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("1025"));
		test_org_false.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("16"));

		// stack instructions

		test_stack_true_one.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("PRINT"));
		test_stack_true_one.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("USING"));
		test_stack_true_one.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("ACONTROL"));
		test_stack_true_one.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("NOPRINT"));
		test_stack_true_two.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("USING"));
		test_stack_false_one.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("USING"));
		test_stack_false_one.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("PRINT"));
		test_stack_false_one.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("ACONTROL"));
		test_stack_false_one.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("PRINT"));
		test_stack_false_one.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("NOPRINT"));
		test_stack_false_two.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>(""));
		test_stack_false_two.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("NOPRINT"));

		// print

		test_print_true.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("OFF"));
		test_print_true.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("NOGEN"));
		test_print_true.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("DATA"));
		test_print_true.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("MCALL"));
		test_print_true.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("NOMCALL"));
		test_print_true.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("MSOURCE"));
		test_print_true.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("NOUHEAD"));
		test_print_true.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("NOUHEAD"));
		test_print_true.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("NOPRINT"));

		// punch

		test_punch_true.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("'Lorem ipsum dolor &sit amet'"));
		test_punch_false_one.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("'Lorem ipsum dolor &sit amet"));
		test_punch_false_two.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("'Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua'"));

		// rmode

		test_rmode_true_one.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("64"));
		test_rmode_true_two.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("ANY"));

		// using

		auto using_true_vector = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>{};
		using_true_vector.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("2048"));
		using_true_vector.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("4096"));
		auto using_first_true_operand = std::make_unique<hlasm_plugin::parser_library::checking::complex_operand>("", std::move(using_true_vector));

		auto using_false_vector = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>{};
		using_false_vector.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("1024"));
		using_false_vector.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("1024"));
		auto using_first_false_operand = std::make_unique<hlasm_plugin::parser_library::checking::complex_operand>("", std::move(using_false_vector));

		test_using_true_one.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("256"));
		test_using_true_one.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("7"));
		test_using_true_two.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("2048"));
		test_using_true_two.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("15"));
		test_using_true_two.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("12"));
		test_using_true_two.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("12"));
		test_using_true_two.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("0"));
		test_using_true_three.push_back(std::move(using_first_true_operand));
		test_using_true_three.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("5"));
		test_using_false_one.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("5"));
		test_using_false_one.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("-1"));
		test_using_false_two.push_back(std::move(using_first_false_operand));
		test_using_false_two.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("8"));

		// xattr

		auto xattr_attributes_vector = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>{};
		xattr_attributes_vector.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("test_label"));
		auto xattr_attributes_operand = std::make_unique<hlasm_plugin::parser_library::checking::complex_operand>("ATTR", std::move(xattr_attributes_vector));

		auto xattr_linkage_true_vector = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>{};
		xattr_linkage_true_vector.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("XPLINK"));
		auto xattr_linkage_true_operand = std::make_unique<hlasm_plugin::parser_library::checking::complex_operand>("LINKAGE", std::move(xattr_linkage_true_vector));

		auto xattr_psect_vector = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>{};
		xattr_psect_vector.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("test_name"));
		auto xattr_psect_operand = std::make_unique<hlasm_plugin::parser_library::checking::complex_operand>("PSECT", std::move(xattr_psect_vector));

		auto xattr_reference_true_vector = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>{};
		xattr_reference_true_vector.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("DIRECT"));
		xattr_reference_true_vector.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("CODE"));
		auto xattr_reference_true_operand = std::make_unique<hlasm_plugin::parser_library::checking::complex_operand>("REFERENCE", std::move(xattr_reference_true_vector));

		auto xattr_scope_vector = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>{};
		xattr_scope_vector.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("X"));
		auto xattr_scope_operand = std::make_unique<hlasm_plugin::parser_library::checking::complex_operand>("SCOPE", std::move(xattr_scope_vector));

		auto xattr_reference_false_vector = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>{};
		xattr_reference_false_vector.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("DATA"));
		xattr_reference_false_vector.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("CODE"));
		auto xattr_reference_false_operand = std::make_unique<hlasm_plugin::parser_library::checking::complex_operand>("REFERENCE", std::move(xattr_reference_false_vector));

		auto xattr_linkage_false_vector = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>{};
		xattr_linkage_false_vector.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("OS"));
		xattr_linkage_false_vector.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("CODE"));
		auto xattr_linkage_false_operand = std::make_unique<hlasm_plugin::parser_library::checking::complex_operand>("LINKAGE", std::move(xattr_linkage_false_vector));

		test_xattr_true.push_back(std::move(xattr_attributes_operand));
		test_xattr_true.push_back(std::move(xattr_linkage_true_operand));
		test_xattr_true.push_back(std::move(xattr_psect_operand));
		test_xattr_true.push_back(std::move(xattr_reference_true_operand));
		test_xattr_true.push_back(std::move(xattr_scope_operand));
		test_xattr_false_one.push_back(std::move(xattr_reference_false_operand));
		test_xattr_false_two.push_back(std::move(xattr_linkage_false_operand));

		// process instruction

		auto process_codepage_true_vector = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>{};
		process_codepage_true_vector.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("X'47C'"));
		auto process_codepage_true_operand = std::make_unique<hlasm_plugin::parser_library::checking::complex_operand>("CP", std::move(process_codepage_true_vector));

		auto process_codepage_false_vector = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>{};
		process_codepage_false_vector.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("63978"));
		auto process_codepage_false_operand = std::make_unique<hlasm_plugin::parser_library::checking::complex_operand>("CP", std::move(process_codepage_false_vector));

		auto process_compat_true_vector = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>{};
		process_compat_true_vector.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("TRANSDT"));
		process_compat_true_vector.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("NOCASE"));
		process_compat_true_vector.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("LITTYPE"));
		process_compat_true_vector.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("NOSYSLIST"));
		process_compat_true_vector.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("TRANSDT"));
		process_compat_true_vector.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("CASE"));
		auto process_compat_true_operand = std::make_unique<hlasm_plugin::parser_library::checking::complex_operand>("CPAT", std::move(process_compat_true_vector));

		auto msg_true_vector = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>{};
		msg_true_vector.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("4"));
		auto msg_true_operand = std::make_unique<hlasm_plugin::parser_library::checking::complex_operand>("MSG", std::move(msg_true_vector));

		auto mnote_true_vector = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>{};
		mnote_true_vector.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("6"));
		auto mnote_true_operand = std::make_unique<hlasm_plugin::parser_library::checking::complex_operand>("MNOTE", std::move(mnote_true_vector));

		auto maxxers_true_vector = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>{};
		maxxers_true_vector.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("256"));
		auto maxxers_true_operand = std::make_unique<hlasm_plugin::parser_library::checking::complex_operand>("MAXXERS", std::move(maxxers_true_vector));

		auto process_fail_true_vector = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>{};
		process_fail_true_vector.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("NOMSG"));
		process_fail_true_vector.push_back(std::move(maxxers_true_operand));
		process_fail_true_vector.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("NOMNOTE"));
		process_fail_true_vector.push_back(std::move(mnote_true_operand));
		process_fail_true_vector.push_back(std::move(msg_true_operand));
		auto process_fail_true_operand = std::make_unique<hlasm_plugin::parser_library::checking::complex_operand>("FAIL", std::move(process_fail_true_vector));

		auto maxxers_false_vector = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>{};
		maxxers_false_vector.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("16"));
		auto maxxers_false_operand = std::make_unique<hlasm_plugin::parser_library::checking::complex_operand>("MAXXERS", std::move(maxxers_false_vector));

		auto process_fail_false_vector = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>{};
		process_fail_false_vector.push_back(std::move(maxxers_false_operand));
		auto process_fail_false_operand = std::make_unique<hlasm_plugin::parser_library::checking::complex_operand>("FAIL", std::move(process_fail_false_vector));

		auto process_flag_true_vector = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>{};
		process_flag_true_vector.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("128"));
		process_flag_true_vector.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("EXLITW"));
		process_flag_true_vector.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("NOSUBSTR"));
		process_flag_true_vector.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("PAGE0"));
		process_flag_true_vector.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("NOCONT"));
		auto process_flag_true_operand = std::make_unique<hlasm_plugin::parser_library::checking::complex_operand>("FLAG", std::move(process_flag_true_vector));

		auto process_flag_false_vector = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>{};
		process_flag_false_vector.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("256"));
		auto process_flag_false_operand = std::make_unique<hlasm_plugin::parser_library::checking::complex_operand>("FLAG", std::move(process_flag_false_vector));

		auto process_info_true_vector = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>{};
		process_info_true_vector.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("20180918"));
		auto process_info_true_operand = std::make_unique<hlasm_plugin::parser_library::checking::complex_operand>("INFO", std::move(process_info_true_vector));

		auto process_info_false_vector = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>{};
		process_info_false_vector.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("20180931"));
		auto process_info_false_operand = std::make_unique<hlasm_plugin::parser_library::checking::complex_operand>("INFO", std::move(process_info_false_vector));

		auto process_machine_true_vector = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>{};
		process_machine_true_vector.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("ZSERIES-7"));
		process_machine_true_vector.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("LIST"));
		auto process_machine_true_operand = std::make_unique<hlasm_plugin::parser_library::checking::complex_operand>("MAC", std::move(process_machine_true_vector));

		auto process_mxref_true_vector = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>{};
		process_mxref_true_vector.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("SOURCE"));
		auto process_mxref_true_operand = std::make_unique<hlasm_plugin::parser_library::checking::complex_operand>("MX", std::move(process_mxref_true_vector));

		auto process_mxref_false_vector = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>{};
		process_mxref_false_vector.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("SOURCE"));
		process_mxref_false_vector.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("FULL"));
		auto process_mxref_false_operand = std::make_unique<hlasm_plugin::parser_library::checking::complex_operand>("MX", std::move(process_mxref_false_vector));

		auto process_optable_true_vector = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>{};
		process_optable_true_vector.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("ESA"));
		process_optable_true_vector.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("NOLIST"));
		auto process_optable_true_operand = std::make_unique<hlasm_plugin::parser_library::checking::complex_operand>("OP", std::move(process_optable_true_vector));

		auto process_pcontrol_true_vector = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>{};
		process_pcontrol_true_vector.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("MCALL"));
		process_pcontrol_true_vector.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("ON"));
		process_pcontrol_true_vector.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("OFF"));
		process_pcontrol_true_vector.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("NOUHEAD"));
		process_pcontrol_true_vector.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("NOGEN"));
		process_pcontrol_true_vector.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("UHEAD"));
		auto process_pcontrol_true_operand = std::make_unique<hlasm_plugin::parser_library::checking::complex_operand>("PC", std::move(process_pcontrol_true_vector));

		auto process_profile_true_vector = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>{};
		process_profile_true_vector.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("test_name"));
		auto process_profile_true_operand = std::make_unique<hlasm_plugin::parser_library::checking::complex_operand>("PROF", std::move(process_profile_true_vector));

		auto process_sectalgn_true_vector = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>{};
		process_sectalgn_true_vector.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("8"));
		auto process_sectalgn_true_operand = std::make_unique<hlasm_plugin::parser_library::checking::complex_operand>("SECTALGN", std::move(process_sectalgn_true_vector));

		auto process_sectalgn_false_vector = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>{};
		process_sectalgn_false_vector.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("8192"));
		auto process_sectalgn_false_operand = std::make_unique<hlasm_plugin::parser_library::checking::complex_operand>("SECTALGN", std::move(process_sectalgn_false_vector));

		auto process_suprwarn_true_vector = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>{};
		process_suprwarn_true_vector.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("066"));
		process_suprwarn_true_vector.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("318"));
		auto process_suprwarn_true_operand = std::make_unique<hlasm_plugin::parser_library::checking::complex_operand>("NOSUP", std::move(process_suprwarn_true_vector));

		auto process_typecheck_true_vector = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>{};
		process_typecheck_true_vector.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("MAG"));
		process_typecheck_true_vector.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("NOREG"));
		auto process_typecheck_true_operand = std::make_unique<hlasm_plugin::parser_library::checking::complex_operand>("TYPECHECK", std::move(process_typecheck_true_vector));

		auto process_typecheck_false_vector = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>{};
		process_typecheck_false_vector.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("NOMAP"));
		auto process_typecheck_false_operand = std::make_unique<hlasm_plugin::parser_library::checking::complex_operand>("TYPECHECK", std::move(process_typecheck_false_vector));

		auto process_warn_true_vector = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>{};
		process_warn_true_vector.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("15"));
		auto process_warn_true_operand = std::make_unique<hlasm_plugin::parser_library::checking::complex_operand>("WARN", std::move(process_warn_true_vector));

		auto process_limit_true_vector = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>{};
		process_limit_true_vector.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("X'F00'"));
		auto process_limit_true_operand = std::make_unique<hlasm_plugin::parser_library::checking::complex_operand>("LIMIT", std::move(process_limit_true_vector));

		auto process_using_true_vector = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>{};
		process_using_true_vector.push_back(std::move(process_warn_true_operand));
		process_using_true_vector.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("NOMAP"));
		process_using_true_vector.push_back(std::move(process_limit_true_operand));
		process_using_true_vector.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("NOWARN"));
		auto process_using_true_operand = std::make_unique<hlasm_plugin::parser_library::checking::complex_operand>("USING", std::move(process_using_true_vector));

		auto process_limit_false_vector = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>{};
		process_limit_false_vector.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("X'FK0'"));
		auto process_limit_false_operand = std::make_unique<hlasm_plugin::parser_library::checking::complex_operand>("LIMIT", std::move(process_limit_false_vector));

		auto process_using_false_vector = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>{};
		process_using_false_vector.push_back(std::move(process_limit_false_operand));
		auto process_using_false_operand = std::make_unique<hlasm_plugin::parser_library::checking::complex_operand>("USING", std::move(process_using_false_vector));

		auto process_xref_true_one_vector = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>{};
		process_xref_true_one_vector.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("FULL"));
		auto process_xref_true_one_operand = std::make_unique<hlasm_plugin::parser_library::checking::complex_operand>("XREF", std::move(process_xref_true_one_vector));

		auto process_xref_true_two_vector = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>{};
		process_xref_true_two_vector.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("SHORT"));
		process_xref_true_two_vector.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("UNREFS"));
		auto process_xref_true_two_operand = std::make_unique<hlasm_plugin::parser_library::checking::complex_operand>("XREF", std::move(process_xref_true_two_vector));

		auto process_xref_false_vector = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>{};
		process_xref_false_vector.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("FULL"));
		process_xref_false_vector.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("UNREFS"));
		auto process_xref_false_operand = std::make_unique<hlasm_plugin::parser_library::checking::complex_operand>("XREF", std::move(process_xref_false_vector));

		auto test_process_true_overriden = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>{};
		test_process_true_overriden.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("NOFAIL"));
		test_process_true_overriden.push_back(std::move(process_xref_true_two_operand));
		test_process_true_overriden.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("FOLD"));
		test_process_true_overriden.push_back(std::move(process_sectalgn_true_operand));
		test_process_true_overriden.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("NOCOMPAT"));
		test_process_true_overriden.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("NOSUPRWARN"));
		test_process_true_overriden.push_back(std::move(process_pcontrol_true_operand));
		test_process_true_overriden.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("NORA2"));
		test_process_true_overriden.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("ERASE"));
		test_process_true_overriden.push_back(std::move(process_info_true_operand));
		test_process_true_overriden.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("NOLIBMAC"));
		test_process_true_overriden.push_back(std::move(process_optable_true_operand));
		test_process_true_overriden.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("NORENT"));
		test_process_true_three.push_back(std::make_unique<hlasm_plugin::parser_library::checking::complex_operand>("OVERRIDE", std::move(test_process_true_overriden)));

		test_process_true_one.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("NOALIGN"));
		test_process_true_one.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("BATCH"));
		test_process_true_one.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("DBCS"));
		test_process_true_one.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("DX"));
		test_process_true_one.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("NOESD"));
		test_process_true_one.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("ILMA"));
		test_process_true_one.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("INFO"));
		test_process_true_one.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("NOMX"));
		test_process_true_one.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("NOPC"));
		test_process_true_one.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("DISK"));
		test_process_true_one.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("NOPROF"));
		test_process_true_one.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("RLD"));
		test_process_true_one.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("RX"));
		test_process_true_one.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("SEG"));
		test_process_true_one.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("NOTEST"));
		test_process_true_one.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("THR"));
		test_process_true_one.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("NOTC"));
		test_process_true_one.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("NOUS"));
		test_process_true_one.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("NOWORKFILE"));
		test_process_true_one.push_back(std::make_unique<hlasm_plugin::parser_library::checking::one_operand>("NOXREF"));

		test_process_true_two.push_back(std::move(process_codepage_true_operand));
		test_process_true_two.push_back(std::move(process_compat_true_operand));
		test_process_true_two.push_back(std::move(process_flag_true_operand));
		test_process_true_two.push_back(std::move(process_machine_true_operand));
		test_process_true_two.push_back(std::move(process_mxref_true_operand));
		test_process_true_two.push_back(std::move(process_profile_true_operand));
		test_process_true_two.push_back(std::move(process_suprwarn_true_operand));
		test_process_true_two.push_back(std::move(process_typecheck_true_operand));
		test_process_true_two.push_back(std::move(process_using_true_operand));
		test_process_true_two.push_back(std::move(process_xref_true_one_operand));

		test_process_false_one.push_back(std::move(process_codepage_false_operand));
		test_process_false_two.push_back(std::move(process_fail_false_operand));
		test_process_false_three.push_back(std::move(process_flag_false_operand));
		test_process_false_four.push_back(std::move(process_info_false_operand));
		test_process_false_five.push_back(std::move(process_mxref_false_operand));
		test_process_false_six.push_back(std::move(process_sectalgn_false_operand));
		test_process_false_seven.push_back(std::move(process_typecheck_false_operand));
		test_process_false_eight.push_back(std::move(process_using_false_operand));
		test_process_false_nine.push_back(std::move(process_xref_false_operand));
	};
protected:
	hlasm_plugin::parser_library::checking::assembler_instruction_checker checker;

	std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>> test_no_operand_true = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>();
	std::vector <std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>> test_adata_true_one = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>();
	std::vector <std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>> test_adata_true_two = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>();
	std::vector <std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>> test_adata_false = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>();
	std::vector <std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>> test_acontrol_true = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>();
	std::vector <std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>> test_ainsert_true_one = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>();
	std::vector <std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>> test_ainsert_true_two = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>();
	std::vector <std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>> test_ainsert_false = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>();
	std::vector <std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>> test_alias_true_one = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>();
	std::vector <std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>> test_alias_true_two = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>();
	std::vector <std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>> test_alias_false = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>();
	std::vector <std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>> test_amode_true = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>();
	std::vector <std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>> test_cattr_true = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>();
	std::vector <std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>> test_expression_true = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>();
	std::vector <std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>> test_ccw_true = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>();
	std::vector <std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>> test_cnop_true = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>();
	std::vector <std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>> test_cnop_one_false = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>();
	std::vector <std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>> test_cnop_two_false = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>();
	std::vector <std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>> test_copy_true = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>();
	std::vector <std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>> test_data_true_one = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>();
	std::vector <std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>> test_data_true_two = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>();
	std::vector <std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>> test_drop_true_one = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>();
	std::vector <std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>> test_drop_true_two = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>();
	std::vector <std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>> test_drop_false = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>();
	std::vector <std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>> test_end_true_one = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>();
	std::vector <std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>> test_end_true_two = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>();
	std::vector <std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>> test_end_false = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>();
	std::vector <std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>> test_equ_true_one = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>();
	std::vector <std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>> test_equ_true_two = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>();
	std::vector <std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>> test_equ_false = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>();
	std::vector <std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>> test_exitctl_true = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>();
	std::vector <std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>> test_exitctl_false_one = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>();
	std::vector <std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>> test_exitctl_false_two = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>();
	std::vector <std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>> test_extrn_true_one = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>();
	std::vector <std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>> test_extrn_true_two = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>();
	std::vector <std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>> test_ictl_true_one = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>();
	std::vector <std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>> test_ictl_true_two = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>();
	std::vector <std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>> test_ictl_false_one = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>();
	std::vector <std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>> test_ictl_false_two = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>();
	std::vector <std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>> test_iseq_true = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>();
	std::vector <std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>> test_iseq_false = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>();
	std::vector <std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>> test_mnote_true_one = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>();
	std::vector <std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>> test_mnote_true_two = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>();
	std::vector <std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>> test_mnote_false_one = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>();
	std::vector <std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>> test_mnote_false_two = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>();
	std::vector <std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>> test_opsyn_true = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>();
	std::vector <std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>> test_org_true_one = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>();
	std::vector <std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>> test_org_true_two = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>();
	std::vector <std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>> test_org_false = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>();
	std::vector <std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>> test_stack_true_one = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>();
	std::vector <std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>> test_stack_true_two = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>();
	std::vector <std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>> test_stack_false_one = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>();
	std::vector <std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>> test_stack_false_two = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>();
	std::vector <std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>> test_print_true = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>();
	std::vector <std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>> test_punch_true = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>();
	std::vector <std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>> test_punch_false_one = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>();
	std::vector <std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>> test_punch_false_two = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>();
	std::vector <std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>> test_rmode_true_one = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>();
	std::vector <std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>> test_rmode_true_two = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>();
	std::vector <std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>> test_using_true_one = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>();
	std::vector <std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>> test_using_true_two = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>();
	std::vector <std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>> test_using_true_three = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>();
	std::vector <std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>> test_using_false_one = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>();
	std::vector <std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>> test_using_false_two = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>();
	std::vector <std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>> test_xattr_true = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>();
	std::vector <std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>> test_xattr_false_one = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>();
	std::vector <std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>> test_xattr_false_two = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>();
	std::vector <std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>> test_process_true_one = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>();
	std::vector <std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>> test_process_true_two = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>();
	std::vector <std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>> test_process_true_three = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>();
	std::vector <std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>> test_process_false_one = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>();
	std::vector <std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>> test_process_false_two = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>();
	std::vector <std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>> test_process_false_three = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>();
	std::vector <std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>> test_process_false_four = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>();
	std::vector <std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>> test_process_false_five = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>();
	std::vector <std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>> test_process_false_six = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>();
	std::vector <std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>> test_process_false_seven = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>();
	std::vector <std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>> test_process_false_eight = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>();
	std::vector <std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>> test_process_false_nine = std::vector<std::unique_ptr<hlasm_plugin::parser_library::checking::one_operand>>();
/*
protected:
	hlasm_plugin::parser_library::checking::one_operand n_287 = { "287" };
	hlasm_plugin::parser_library::checking::one_operand empty = { "" };
	hlasm_plugin::parser_library::checking::one_operand n_min567 = { "-567" };
	hlasm_plugin::parser_library::checking::one_operand n_0 = { "0" };
	hlasm_plugin::parser_library::checking::one_operand temporary_string = { "'temporary_string'" };
	hlasm_plugin::parser_library::checking::one_operand n_21474836484 = { "21474836484" };
	hlasm_plugin::parser_library::checking::one_operand temporary = { "'temporary" };
	hlasm_plugin::parser_library::checking::one_operand CASE = { "CASE" };
	hlasm_plugin::parser_library::checking::one_operand NOMC = { "NOMC" };
	hlasm_plugin::parser_library::checking::one_operand NOTRS = { "NOTRS" };
	hlasm_plugin::parser_library::checking::one_operand NOTRANSDT = { "NOTRANSDT" };
	hlasm_plugin::parser_library::checking::one_operand COMPAT = { "COMPAT" };
	hlasm_plugin::parser_library::checking::one_operand n_2 = { "2" };
	hlasm_plugin::parser_library::checking::one_operand AL = { "AL" };
	hlasm_plugin::parser_library::checking::one_operand NOCONT = { "NOCONT" };
	hlasm_plugin::parser_library::checking::one_operand IMPLEN = { "IMPLEN" };
	hlasm_plugin::parser_library::checking::one_operand NOUSING0 = { "NOUSING0" };
	hlasm_plugin::parser_library::checking::one_operand FLAG = { "FLAG" };
	hlasm_plugin::parser_library::checking::one_operand MAGNITUDE = { "MAGNITUDE" };
	hlasm_plugin::parser_library::checking::one_operand NOREGISTER = { "NOREGISTER" };
	hlasm_plugin::parser_library::checking::one_operand REGISTER = { "REGISTER" };
	hlasm_plugin::parser_library::checking::one_operand TYPECHECK = { "TYPECHECK" };
	hlasm_plugin::parser_library::checking::one_operand ZS6 = { "ZS6" };
	hlasm_plugin::parser_library::checking::one_operand LIST = { "LIST" };
	hlasm_plugin::parser_library::checking::one_operand OPTABLE = { "OPTABLE" };
	hlasm_plugin::parser_library::checking::one_operand AFPR = { "AFPR" };
	hlasm_plugin::parser_library::checking::one_operand NOCOMPAT = { "NOCOMPAT" };
	hlasm_plugin::parser_library::checking::one_operand LMAC = { "LMAC" };
	hlasm_plugin::parser_library::checking::one_operand RA2 = { "RA2" };
	hlasm_plugin::parser_library::checking::one_operand NOTC = { "NOTC" };
	hlasm_plugin::parser_library::checking::one_operand n_test_string = { "'test string'" };
	hlasm_plugin::parser_library::checking::one_operand BACK = { "BACK" };
	hlasm_plugin::parser_library::checking::one_operand n_lorem = { "'Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.'" };
	hlasm_plugin::parser_library::checking::one_operand FRONT = { "FRONT" };
	hlasm_plugin::parser_library::checking::one_operand n_apostrophe = { "'" };
	hlasm_plugin::parser_library::checking::one_operand Clowerl = { "C'lowerl'" };
	hlasm_plugin::parser_library::checking::one_operand X9396A68599F2 = { "X'9396A68599F2'" };
	hlasm_plugin::parser_library::checking::one_operand XFF92 = { "X'FF92'" };
	hlasm_plugin::parser_library::checking::one_operand ANY31 = { "ANY31" };
	hlasm_plugin::parser_library::checking::one_operand ANY = { "ANY" };
	hlasm_plugin::parser_library::checking::one_operand RMODE = { "RMODE" };
	hlasm_plugin::parser_library::checking::one_operand n_12 = { "12" };
	hlasm_plugin::parser_library::checking::one_operand ALIGN = { "ALIGN" };
	hlasm_plugin::parser_library::checking::one_operand n_128 = { "128" };
	hlasm_plugin::parser_library::checking::one_operand FILL = { "FILL" };
	hlasm_plugin::parser_library::checking::one_operand test_string = { "test string" };
	hlasm_plugin::parser_library::checking::one_operand PART = { "PART" };
	hlasm_plugin::parser_library::checking::one_operand n_256 = { "256" };
	hlasm_plugin::parser_library::checking::one_operand PRIORITY = { "PRIORITY" };
	hlasm_plugin::parser_library::checking::one_operand DEFLOAD = { "DEFLOAD" };
	hlasm_plugin::parser_library::checking::one_operand MOVABLE = { "MOVABLE" };
	hlasm_plugin::parser_library::checking::one_operand NOTREUS = { "NOTREUS" };
	hlasm_plugin::parser_library::checking::one_operand REFR = { "REFR" };
	hlasm_plugin::parser_library::checking::one_operand REUS = { "REUS" };
	hlasm_plugin::parser_library::checking::one_operand n_8 = { "8" };
	hlasm_plugin::parser_library::checking::one_operand X06 = { "X'06'" };
	hlasm_plugin::parser_library::checking::one_operand MyData = { "MyData" };
	hlasm_plugin::parser_library::checking::one_operand X40 = { "X'40'" };
	hlasm_plugin::parser_library::checking::one_operand MyBlkSize = { "MyBlkSize" };
	hlasm_plugin::parser_library::checking::one_operand n_6 = { "6" };
	hlasm_plugin::parser_library::checking::one_operand n_13 = { "13" };
	hlasm_plugin::parser_library::checking::one_operand n_15 = { "15" };
	hlasm_plugin::parser_library::checking::one_operand A = { "A" };
	hlasm_plugin::parser_library::checking::one_operand  FS410253U268435455 = { " FS4'-10,25.3,U268435455'" };
	hlasm_plugin::parser_library::checking::one_operand DL7S3E5027182 = { "DL7S3E50'2.7182'" };
	hlasm_plugin::parser_library::checking::one_operand FPRate53592 = { "FP(Rate5)'35.92'" };
	hlasm_plugin::parser_library::checking::one_operand XPCAPF = { "3XP(C'APC')'FF'" };
	hlasm_plugin::parser_library::checking::one_operand EBP7L212 = { "10EBP(7)L2'12'" };
	hlasm_plugin::parser_library::checking::one_operand n_7 = { "7" };
	hlasm_plugin::parser_library::checking::one_operand n_11 = { "11" };
	hlasm_plugin::parser_library::checking::one_operand FIRST = { "FIRST" };
	hlasm_plugin::parser_library::checking::one_operand SECOND = { "SECOND" };
	hlasm_plugin::parser_library::checking::one_operand n_sLorem = { "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua" };
	hlasm_plugin::parser_library::checking::one_operand MYCOMPILER = { "MYCOMPILER" };
	hlasm_plugin::parser_library::checking::one_operand n_0101 = { "0101" };
	hlasm_plugin::parser_library::checking::one_operand n_00273 = { "00273" };
	hlasm_plugin::parser_library::checking::one_operand MYOWNCOMPILER = { "MYOWNCOMPILER" };
	hlasm_plugin::parser_library::checking::one_operand n_01010 = { "01010" };
	hlasm_plugin::parser_library::checking::one_operand n_002737 = { "002737" };
	hlasm_plugin::parser_library::checking::one_operand BEGIN = { "BEGIN" };
	hlasm_plugin::parser_library::checking::one_operand ENTRYPT = { "ENTRYPT" };
	hlasm_plugin::parser_library::checking::one_operand n_40 = { "40" };
	hlasm_plugin::parser_library::checking::one_operand CR32 = { "CR32" };
	hlasm_plugin::parser_library::checking::one_operand n_16 = { "16" };
	hlasm_plugin::parser_library::checking::one_operand A1 = { "A1" };
	hlasm_plugin::parser_library::checking::one_operand n_18 = { "18" };
	hlasm_plugin::parser_library::checking::one_operand SOURCE = { "SOURCE" };
	hlasm_plugin::parser_library::checking::one_operand min1024 = { "-1024" };
	hlasm_plugin::parser_library::checking::one_operand n_2147483647 = { "2147483647" };
	hlasm_plugin::parser_library::checking::one_operand SOURCES = { "SOURCES" };
	hlasm_plugin::parser_library::checking::one_operand LISTING = { "LISTING" };
	hlasm_plugin::parser_library::checking::one_operand n_21474836489 = { "21474836489" };
	hlasm_plugin::parser_library::checking::one_operand test_symbol_one = { "test_symbol_one" };
	hlasm_plugin::parser_library::checking::one_operand test_symbol_two = { "test_symbol_two" };
	hlasm_plugin::parser_library::checking::one_operand n_1 = { "1" };
	hlasm_plugin::parser_library::checking::one_operand n_71 = { "71" };
	hlasm_plugin::parser_library::checking::one_operand n_5 = { "5" };
	hlasm_plugin::parser_library::checking::one_operand n_35 = { "35" };
	hlasm_plugin::parser_library::checking::one_operand error_mess = { "'Error message text'" };
	hlasm_plugin::parser_library::checking::one_operand star = { "*" };
	hlasm_plugin::parser_library::checking::one_operand error_mess_w = { "'Error message text" };
	hlasm_plugin::parser_library::checking::one_operand STH = { "STH" };
	hlasm_plugin::parser_library::checking::one_operand n_1024 = { "1024" };
	hlasm_plugin::parser_library::checking::one_operand n_2048 = { "2048" };
	hlasm_plugin::parser_library::checking::one_operand n_1025 = { "1025" };
	hlasm_plugin::parser_library::checking::one_operand PRINT = { "PRINT" };
	hlasm_plugin::parser_library::checking::one_operand ACONTROL = { "ACONTROL" };
	hlasm_plugin::parser_library::checking::one_operand NOPRINT = { "NOPRINT" };
	hlasm_plugin::parser_library::checking::one_operand USING = { "USING" };
	hlasm_plugin::parser_library::checking::one_operand OFF = { "OFF" };
	hlasm_plugin::parser_library::checking::one_operand NOGEN = { "NOGEN" };
	hlasm_plugin::parser_library::checking::one_operand DATA = { "DATA" };
	hlasm_plugin::parser_library::checking::one_operand MCALL = { "MCALL" };
	hlasm_plugin::parser_library::checking::one_operand NOMCALL = { "NOMCALL" };
	hlasm_plugin::parser_library::checking::one_operand MSOURCE = { "MSOURCE" };
	hlasm_plugin::parser_library::checking::one_operand NOUHEAD = { "NOUHEAD" };
	hlasm_plugin::parser_library::checking::one_operand lorem1 = { "'Lorem ipsum dolor &sit amet'" };
	hlasm_plugin::parser_library::checking::one_operand lorem2 = { "'Lorem ipsum dolor &sit amet" };
	hlasm_plugin::parser_library::checking::one_operand lorem3 = { "'Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua'" };
	hlasm_plugin::parser_library::checking::one_operand n_64 = { "64" };
	hlasm_plugin::parser_library::checking::one_operand n_ANY = { "ANY" };
	hlasm_plugin::parser_library::checking::one_operand n_4096 = { "4096" };
	hlasm_plugin::parser_library::checking::one_operand min1 = { "-1" };
	hlasm_plugin::parser_library::checking::one_operand test_label = { "test_label" };
	hlasm_plugin::parser_library::checking::one_operand ATTR = { "ATTR" };
	hlasm_plugin::parser_library::checking::one_operand XPLINK = { "XPLINK" };
	hlasm_plugin::parser_library::checking::one_operand LINKAGE = { "LINKAGE" };
	hlasm_plugin::parser_library::checking::one_operand test_name = { "test_name" };
	hlasm_plugin::parser_library::checking::one_operand PSECT = { "PSECT" };
	hlasm_plugin::parser_library::checking::one_operand DIRECT = { "DIRECT" };
	hlasm_plugin::parser_library::checking::one_operand CODE = { "CODE" };
	hlasm_plugin::parser_library::checking::one_operand REFERENCE = { "REFERENCE" };
	hlasm_plugin::parser_library::checking::one_operand X = { "X" };
	hlasm_plugin::parser_library::checking::one_operand SCOPE = { "SCOPE" };
	hlasm_plugin::parser_library::checking::one_operand OS = { "OS" };
	hlasm_plugin::parser_library::checking::one_operand n_1148X47C = { "1148|X'47C'" };
	hlasm_plugin::parser_library::checking::one_operand n_63978X47C = { "63978|X'47C'" };
	hlasm_plugin::parser_library::checking::one_operand CP = { "CP" };
	hlasm_plugin::parser_library::checking::one_operand TRANSDT = { "TRANSDT" };
	hlasm_plugin::parser_library::checking::one_operand NOCASE = { "NOCASE" };
	hlasm_plugin::parser_library::checking::one_operand LITTYPE = { "LITTYPE" };
	hlasm_plugin::parser_library::checking::one_operand NOSYSLIST = { "NOSYSLIST" };
	hlasm_plugin::parser_library::checking::one_operand CPAT = { "CPAT" };
	hlasm_plugin::parser_library::checking::one_operand n_4 = { "4" };
	hlasm_plugin::parser_library::checking::one_operand MSG = { "MSG" };
	hlasm_plugin::parser_library::checking::one_operand MNOTE = { "MNOTE" };
	hlasm_plugin::parser_library::checking::one_operand MAXXERS = { "MAXXERS" };
	hlasm_plugin::parser_library::checking::one_operand NOMSG = { "NOMSG" };
	hlasm_plugin::parser_library::checking::one_operand NOMNOTE = { "NOMNOTE" };
	hlasm_plugin::parser_library::checking::one_operand FAIL = { "FAIL" };
	hlasm_plugin::parser_library::checking::one_operand EXLITW = { "EXLITW" };
	hlasm_plugin::parser_library::checking::one_operand NOSUBSTR = { "NOSUBSTR" };
	hlasm_plugin::parser_library::checking::one_operand PAGE0 = { "PAGE0" };
	hlasm_plugin::parser_library::checking::one_operand n_20180918 = { "20180918" };
	hlasm_plugin::parser_library::checking::one_operand INFO = { "INFO" };
	hlasm_plugin::parser_library::checking::one_operand n_20180931 = { "20180931" };
	hlasm_plugin::parser_library::checking::one_operand ZSERIES7 = { "ZSERIES-7" };
	hlasm_plugin::parser_library::checking::one_operand MAC = { "MAC" };
	hlasm_plugin::parser_library::checking::one_operand MX = { "MX" };
	hlasm_plugin::parser_library::checking::one_operand FULL = { "FULL" };
	hlasm_plugin::parser_library::checking::one_operand ESA = { "ESA" };
	hlasm_plugin::parser_library::checking::one_operand NOLIST = { "NOLIST" };
	hlasm_plugin::parser_library::checking::one_operand OP = { "OP" };
	hlasm_plugin::parser_library::checking::one_operand ON = { "ON" };
	hlasm_plugin::parser_library::checking::one_operand UHEAD = { "UHEAD" };
	hlasm_plugin::parser_library::checking::one_operand PC = { "PC" };
	hlasm_plugin::parser_library::checking::one_operand PROF = { "PROF" };
	hlasm_plugin::parser_library::checking::one_operand SECTALGN = { "SECTALGN" };
	hlasm_plugin::parser_library::checking::one_operand n_8192 = { "8192" };
	hlasm_plugin::parser_library::checking::one_operand n_066 = { "066" };
	hlasm_plugin::parser_library::checking::one_operand n_318 = { "318" };
	hlasm_plugin::parser_library::checking::one_operand NOSUP = { "NOSUP" };
	hlasm_plugin::parser_library::checking::one_operand MAG = { "MAG" };
	hlasm_plugin::parser_library::checking::one_operand NOREG = { "NOREG" };
	hlasm_plugin::parser_library::checking::one_operand NOMAP = { "NOMAP" };
	hlasm_plugin::parser_library::checking::one_operand WARN = { "WARN" };
	hlasm_plugin::parser_library::checking::one_operand XF00 = { "X'F00'" };
	hlasm_plugin::parser_library::checking::one_operand LIMIT = { "LIMIT" };
	hlasm_plugin::parser_library::checking::one_operand NOWARN = { "NOWARN" };
	hlasm_plugin::parser_library::checking::one_operand XFK0 = { "X'FK0'" };
	hlasm_plugin::parser_library::checking::one_operand XREF = { "XREF" };
	hlasm_plugin::parser_library::checking::one_operand SHORT = { "SHORT" };
	hlasm_plugin::parser_library::checking::one_operand UNREFS = { "UNREFS" };
	hlasm_plugin::parser_library::checking::one_operand NOFAIL = { "NOFAIL" };
	hlasm_plugin::parser_library::checking::one_operand FOLD = { "FOLD" };
	hlasm_plugin::parser_library::checking::one_operand NOSUPRWARN = { "NOSUPRWARN" };
	hlasm_plugin::parser_library::checking::one_operand NORA2 = { "NORA2" };
	hlasm_plugin::parser_library::checking::one_operand ERASE = { "ERASE" };
	hlasm_plugin::parser_library::checking::one_operand NOLIBMAC = { "NOLIBMAC" };
	hlasm_plugin::parser_library::checking::one_operand LIBMAC = { "LIBMAC" };
	hlasm_plugin::parser_library::checking::one_operand NORENT = { "NORENT" };
	hlasm_plugin::parser_library::checking::one_operand OVERRIDE = { "OVERRIDE" };
	hlasm_plugin::parser_library::checking::one_operand NOALIGN = { "NOALIGN" };
	hlasm_plugin::parser_library::checking::one_operand BATCH = { "BATCH" };
	hlasm_plugin::parser_library::checking::one_operand DBCS = { "DBCS" };
	hlasm_plugin::parser_library::checking::one_operand DX = { "DX" };
	hlasm_plugin::parser_library::checking::one_operand NOESD = { "NOESD" };
	hlasm_plugin::parser_library::checking::one_operand ILMA = { "ILMA" };
	hlasm_plugin::parser_library::checking::one_operand NOMX = { "NOMX" };
	hlasm_plugin::parser_library::checking::one_operand NOPC = { "NOPC" };
	hlasm_plugin::parser_library::checking::one_operand DISK = { "DISK" };
	hlasm_plugin::parser_library::checking::one_operand NOPROF = { "NOPROF" };
	hlasm_plugin::parser_library::checking::one_operand RLD = { "RLD" };
	hlasm_plugin::parser_library::checking::one_operand RX = { "RX" };
	hlasm_plugin::parser_library::checking::one_operand SEG = { "SEG" };
	hlasm_plugin::parser_library::checking::one_operand NOTEST = { "NOTEST" };
	hlasm_plugin::parser_library::checking::one_operand THR = { "THR" };
	hlasm_plugin::parser_library::checking::one_operand NOUS = { "NOUS" };
	hlasm_plugin::parser_library::checking::one_operand NOWORKFILE = { "NOWORKFILE" };
	hlasm_plugin::parser_library::checking::one_operand NOXREF = { "NOXREF" };

	std::vector<hlasm_plugin::parser_library::checking::one_operand*> test_adata_true_one{ std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &n_287, &empty, &n_min567, &n_0, &temporary_string } };
	std::vector<hlasm_plugin::parser_library::checking::one_operand*> test_adata_true_two{ std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &n_0, &empty, &empty, &empty, &empty} };
	std::vector<hlasm_plugin::parser_library::checking::one_operand*> test_adata_false{ std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &empty, &empty, &empty, &n_21474836484, &temporary } };

	hlasm_plugin::parser_library::checking::complex_operand acontrol_operand_compat{ "COMPAT", std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &CASE, &NOMC, &NOTRS, &NOTRANSDT, &NOTRANSDT} };
	hlasm_plugin::parser_library::checking::complex_operand acontrol_operand_flag{ "FLAG", std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &n_2, &AL, &NOCONT, &IMPLEN, &NOUSING0 } };
	hlasm_plugin::parser_library::checking::complex_operand acontrol_operand_typecheck{ "TYPECHECK", std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &MAGNITUDE, &NOREGISTER, &REGISTER} };
	hlasm_plugin::parser_library::checking::complex_operand acontrol_operand_optable{ "OPTABLE", std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &ZS6, &LIST} };
	std::vector<hlasm_plugin::parser_library::checking::one_operand*> test_acontrol_true{ std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &AFPR, &acontrol_operand_compat, &NOCOMPAT, &acontrol_operand_flag, &LMAC, &acontrol_operand_optable, &RA2, &acontrol_operand_typecheck, &NOTC } };

	std::vector<hlasm_plugin::parser_library::checking::one_operand*> test_ainsert_true_one{ std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &n_test_string, &BACK } };
	std::vector<hlasm_plugin::parser_library::checking::one_operand*> test_ainsert_true_two{ std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &n_lorem, &FRONT } };
	std::vector<hlasm_plugin::parser_library::checking::one_operand*> test_ainsert_false{ std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &n_apostrophe, &FRONT } };

	std::vector<hlasm_plugin::parser_library::checking::one_operand*> test_alias_true_one{ std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &Clowerl } };
	std::vector<hlasm_plugin::parser_library::checking::one_operand*> test_alias_true_two{ std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &X9396A68599F2 } };
	std::vector<hlasm_plugin::parser_library::checking::one_operand*> test_alias_false{ std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &XFF92 } };

	std::vector<hlasm_plugin::parser_library::checking::one_operand*> test_amode_true{ std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &ANY31 } };

	hlasm_plugin::parser_library::checking::complex_operand cattr_operand_rmode{ "RMODE", std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &ANY} };
	hlasm_plugin::parser_library::checking::complex_operand cattr_operand_align{ "ALIGN", std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &n_12} };
	hlasm_plugin::parser_library::checking::complex_operand cattr_operand_fill{ "FILL", std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &n_128} };
	hlasm_plugin::parser_library::checking::complex_operand cattr_operand_part{ "PART", std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &test_string } };
	hlasm_plugin::parser_library::checking::complex_operand cattr_operand_priority{ "PRIORITY", std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &n_256} };
	std::vector<hlasm_plugin::parser_library::checking::one_operand*> test_cattr_true{ std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &DEFLOAD, &cattr_operand_rmode, &MOVABLE, &cattr_operand_align, &NOTREUS, &cattr_operand_fill, &REFR, &cattr_operand_part, &REUS, &cattr_operand_priority, &REUS } };

	std::vector<hlasm_plugin::parser_library::checking::one_operand*> test_expression_true{ std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &n_8} };

	std::vector<hlasm_plugin::parser_library::checking::one_operand*> test_ccw_true{ std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &X06, &MyData, &X40, &MyBlkSize} };

	std::vector<hlasm_plugin::parser_library::checking::one_operand*> test_cnop_true{ std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &n_6, &n_8} };	
	std::vector<hlasm_plugin::parser_library::checking::one_operand*> test_cnop_one_false{ std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &n_13, &n_15} };
	std::vector<hlasm_plugin::parser_library::checking::one_operand*> test_cnop_two_false{ std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &n_128, &n_128} };

	std::vector<hlasm_plugin::parser_library::checking::one_operand*> test_copy_true{ std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &A} };

	std::vector<hlasm_plugin::parser_library::checking::one_operand*> test_data_true_one{ std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &FS410253U268435455, &DL7S3E5027182, &FPRate53592, &XPCAPF } };
	std::vector<hlasm_plugin::parser_library::checking::one_operand*> test_data_true_two{ std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &EBP7L212 } };

	std::vector<hlasm_plugin::parser_library::checking::one_operand*> test_drop_true_one{ std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &n_7, &n_11, &FIRST } };
	std::vector<hlasm_plugin::parser_library::checking::one_operand*> test_drop_true_two{ std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &SECOND } };
	std::vector<hlasm_plugin::parser_library::checking::one_operand*> test_drop_false{ std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &FIRST, &n_sLorem } };

	hlasm_plugin::parser_library::checking::complex_operand end_lang_operand_first{ "", std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &MYCOMPILER, &n_0101, &n_00273} };
	hlasm_plugin::parser_library::checking::complex_operand end_lang_operand_second{ "", std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &MYCOMPILER, &n_0101, &n_00273} };
	hlasm_plugin::parser_library::checking::complex_operand end_lang_operand_false{ "", std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &MYOWNCOMPILER, &n_01010, &n_002737 } };
	std::vector<hlasm_plugin::parser_library::checking::one_operand*> test_end_true_one{ std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &BEGIN, &end_lang_operand_first } };
	std::vector<hlasm_plugin::parser_library::checking::one_operand*> test_end_true_two{ std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &empty, &end_lang_operand_second } };
	std::vector<hlasm_plugin::parser_library::checking::one_operand*> test_end_false{ std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &ENTRYPT, &end_lang_operand_false } };

	std::vector<hlasm_plugin::parser_library::checking::one_operand*> test_equ_true_one{ std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &n_256, &n_40, &n_128, &n_128, &CR32 } };
	std::vector<hlasm_plugin::parser_library::checking::one_operand*> test_equ_true_two{ std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &n_16, &n_8, &n_128, &empty, &CR32 } };
	std::vector<hlasm_plugin::parser_library::checking::one_operand*> test_equ_false{ std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &A1, &n_256, &n_256, &n_256, &n_18 } };

	std::vector<hlasm_plugin::parser_library::checking::one_operand*> test_exitctl_true{ std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &SOURCE, &n_0, &min1024, &n_2147483647 } };
	std::vector<hlasm_plugin::parser_library::checking::one_operand*> test_exitctl_false_one{ std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &SOURCES, &n_15 } };
	std::vector<hlasm_plugin::parser_library::checking::one_operand*> test_exitctl_false_two{ std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &LISTING, &n_21474836489, &n_128 } };

	hlasm_plugin::parser_library::checking::complex_operand extrn_part_operand{ "PART", std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &test_symbol_one, &test_symbol_two } };
	std::vector<hlasm_plugin::parser_library::checking::one_operand*> test_extrn_true_one{ std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &test_symbol_one, &test_symbol_two } };
	std::vector<hlasm_plugin::parser_library::checking::one_operand*> test_extrn_true_two{ std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &extrn_part_operand } };

	std::vector<hlasm_plugin::parser_library::checking::one_operand*> test_ictl_true_one{ std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &n_1, &n_71, &n_16 } };
	std::vector<hlasm_plugin::parser_library::checking::one_operand*> test_ictl_true_two{ std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &n_5 } };
	std::vector<hlasm_plugin::parser_library::checking::one_operand*> test_ictl_false_one{ std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &n_16, &n_40, &n_16 } };
	std::vector<hlasm_plugin::parser_library::checking::one_operand*> test_ictl_false_two{ std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &test_string } };

	std::vector<hlasm_plugin::parser_library::checking::one_operand*> test_iseq_true{ std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &n_40, &n_40 } };
	std::vector<hlasm_plugin::parser_library::checking::one_operand*> test_iseq_false{ std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &n_40, &n_35 } };

	std::vector<hlasm_plugin::parser_library::checking::one_operand*> test_mnote_true_one{ std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &n_128, &error_mess } };
	std::vector<hlasm_plugin::parser_library::checking::one_operand*> test_mnote_true_two{ std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &star, &error_mess} };
	std::vector<hlasm_plugin::parser_library::checking::one_operand*> test_mnote_false_one{ std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &n_256, &error_mess } };
	std::vector<hlasm_plugin::parser_library::checking::one_operand*> test_mnote_false_two{ std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &star, &error_mess_w } };

	std::vector<hlasm_plugin::parser_library::checking::one_operand*> test_opsyn_true{ std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &STH } };

	std::vector<hlasm_plugin::parser_library::checking::one_operand*> test_org_true_one{ std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &n_1024, &n_2048, &n_16} };
	std::vector<hlasm_plugin::parser_library::checking::one_operand*> test_org_true_two{ std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &n_16, &empty, &n_16 } };
	std::vector<hlasm_plugin::parser_library::checking::one_operand*> test_org_false{ std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &n_1024, &n_1025, &n_16 } };

	std::vector<hlasm_plugin::parser_library::checking::one_operand*> test_stack_true_one { std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &PRINT, &USING, &ACONTROL, &NOPRINT } };
	std::vector<hlasm_plugin::parser_library::checking::one_operand*> test_stack_true_two { std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &USING } };
	std::vector<hlasm_plugin::parser_library::checking::one_operand*> test_stack_false_one { std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &USING, &PRINT, &ACONTROL, &PRINT, &NOPRINT} };
	std::vector<hlasm_plugin::parser_library::checking::one_operand*> test_stack_false_two { std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &empty, &NOPRINT } };

	std::vector<hlasm_plugin::parser_library::checking::one_operand*> test_print_true{ std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &OFF, &NOGEN, &DATA, &MCALL, &NOMCALL, &MSOURCE, &NOUHEAD, &NOUHEAD, &NOPRINT } };

	std::vector<hlasm_plugin::parser_library::checking::one_operand*> test_punch_true{ std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &lorem1 } };
	std::vector<hlasm_plugin::parser_library::checking::one_operand*> test_punch_false_one{ std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &lorem2 } };
	std::vector<hlasm_plugin::parser_library::checking::one_operand*> test_punch_false_two{ std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &lorem3 } };

	std::vector<hlasm_plugin::parser_library::checking::one_operand*> test_rmode_true_one{ std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &n_64 } };
	std::vector<hlasm_plugin::parser_library::checking::one_operand*> test_rmode_true_two{ std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &ANY } };

	hlasm_plugin::parser_library::checking::complex_operand using_first_true_operand{ "", std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &n_2048, &n_4096 } };
	hlasm_plugin::parser_library::checking::complex_operand using_first_false_operand{ "", std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &n_1024, &n_1024 } };
	std::vector<hlasm_plugin::parser_library::checking::one_operand*> test_using_true_one{ std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &n_256, &n_7 } };
	std::vector<hlasm_plugin::parser_library::checking::one_operand*> test_using_true_two{ std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &n_2048, &n_15, &n_12, &n_12, &n_0 } };
	std::vector<hlasm_plugin::parser_library::checking::one_operand*> test_using_true_three{ std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &using_first_true_operand, &n_5 } };
	std::vector<hlasm_plugin::parser_library::checking::one_operand*> test_using_false_one{ std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &n_5, &min1 } };
	std::vector<hlasm_plugin::parser_library::checking::one_operand*> test_using_false_two{ std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &using_first_false_operand, &n_8 } };

	hlasm_plugin::parser_library::checking::complex_operand xattr_attributes_operand{ "ATTR", std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &test_label } };
	hlasm_plugin::parser_library::checking::complex_operand xattr_linkage_true_operand{ "LINKAGE", std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &XPLINK } };
	hlasm_plugin::parser_library::checking::complex_operand xattr_psect_operand{ "PSECT", std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &test_name } };
	hlasm_plugin::parser_library::checking::complex_operand xattr_reference_true_operand{ "REFERENCE", std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &DIRECT, &CODE } };
	hlasm_plugin::parser_library::checking::complex_operand xattr_scope_operand{ "SCOPE", std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &X} };
	hlasm_plugin::parser_library::checking::complex_operand xattr_reference_false_operand{ "REFERENCE", std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &DATA, &CODE } };
	hlasm_plugin::parser_library::checking::complex_operand xattr_linkage_false_operand{ "LINKAGE", std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &OS, &CODE } };
	std::vector<hlasm_plugin::parser_library::checking::one_operand*> test_xattr_true{ std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &xattr_attributes_operand, &xattr_linkage_true_operand, &xattr_psect_operand, &xattr_reference_true_operand, &xattr_scope_operand } };
	std::vector<hlasm_plugin::parser_library::checking::one_operand*> test_xattr_false_one{ std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &xattr_reference_false_operand } };
	std::vector<hlasm_plugin::parser_library::checking::one_operand*> test_xattr_false_two{ std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &xattr_linkage_false_operand } };

	hlasm_plugin::parser_library::checking::complex_operand process_codepage_true_operand{ "CP", std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &n_1148X47C } };
	hlasm_plugin::parser_library::checking::complex_operand process_codepage_false_operand{ "CP", std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &n_63978X47C } };
	hlasm_plugin::parser_library::checking::complex_operand process_compat_true_operand{ "CPAT", std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &TRANSDT, &NOCASE, &LITTYPE, &NOSYSLIST, &TRANSDT, &CASE } };
	hlasm_plugin::parser_library::checking::complex_operand msg_true_operand{ "MSG", std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &n_4 } };
	hlasm_plugin::parser_library::checking::complex_operand mnote_true_operand{ "MNOTE", std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &n_6 } };
	hlasm_plugin::parser_library::checking::complex_operand maxxers_true_operand{ "MAXXERS", std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &n_256 } };
	hlasm_plugin::parser_library::checking::complex_operand process_fail_true_operand{ "FAIL", std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &NOMSG, &maxxers_true_operand, &NOMNOTE, &mnote_true_operand, &msg_true_operand } };
	hlasm_plugin::parser_library::checking::complex_operand maxxers_false_operand{ "MAXXERS", std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &n_16 } };
	hlasm_plugin::parser_library::checking::complex_operand process_fail_false_operand{ "FAIL", std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &maxxers_false_operand } };
	hlasm_plugin::parser_library::checking::complex_operand process_flag_true_operand{ "FLAG", std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &n_128, &EXLITW, &NOSUBSTR, &PAGE0, &NOCONT } };
	hlasm_plugin::parser_library::checking::complex_operand process_flag_false_operand{ "FLAG", std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &n_256 } };
	hlasm_plugin::parser_library::checking::complex_operand process_info_true_operand{ "INFO", std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &n_20180918 } };
	hlasm_plugin::parser_library::checking::complex_operand process_info_false_operand{ "INFO", std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &n_20180931 } };
	hlasm_plugin::parser_library::checking::complex_operand process_machine_true_operand { "MAC", std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &ZSERIES7, &LIST } };
	hlasm_plugin::parser_library::checking::complex_operand process_mxref_true_operand { "MX", std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &SOURCE } };
	hlasm_plugin::parser_library::checking::complex_operand process_mxref_false_operand { "MX", std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &SOURCE, &FULL } };
	hlasm_plugin::parser_library::checking::complex_operand process_optable_true_operand { "OP", std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &ESA, &NOLIST } };
	hlasm_plugin::parser_library::checking::complex_operand process_pcontrol_true_operand{ "PC", std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &MCALL, &ON, &OFF, &NOUHEAD, &NOGEN, &UHEAD } };
	hlasm_plugin::parser_library::checking::complex_operand process_profile_true_operand{ "PROF", std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &test_name } };
	hlasm_plugin::parser_library::checking::complex_operand process_sectalgn_true_operand{ "SECTALGN", std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &n_8 } };
	hlasm_plugin::parser_library::checking::complex_operand process_sectalgn_false_operand{ "SECTALGN", std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &n_8192 } };
	hlasm_plugin::parser_library::checking::complex_operand process_suprwarn_true_operand{ "NOSUP", std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &n_066, &n_318 } };
	hlasm_plugin::parser_library::checking::complex_operand process_typecheck_true_operand{ "TYPECHECK", std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &MAG, &NOREG } };
	hlasm_plugin::parser_library::checking::complex_operand process_typecheck_false_operand{ "TYPECHECK", std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &NOMAP } };
	hlasm_plugin::parser_library::checking::complex_operand process_warn_true_operand{ "WARN", std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &n_15 } };
	hlasm_plugin::parser_library::checking::complex_operand process_limit_true_operand{ "LIMIT", std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &XF00 } };
	hlasm_plugin::parser_library::checking::complex_operand process_using_true_operand{ "USING", std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &process_warn_true_operand, &NOMAP, &process_limit_true_operand, &NOWARN } };
	hlasm_plugin::parser_library::checking::complex_operand process_limit_false_operand{ "LIMIT", std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &XFK0 } };
	hlasm_plugin::parser_library::checking::complex_operand process_using_false_operand{ "USING", std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &process_limit_false_operand } };
	hlasm_plugin::parser_library::checking::complex_operand process_xref_true_one_operand{ "XREF", std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &FULL } };
	hlasm_plugin::parser_library::checking::complex_operand process_xref_true_two_operand{ "XREF", std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &SHORT, &UNREFS, &SHORT } };
	hlasm_plugin::parser_library::checking::complex_operand process_xref_false_operand{ "XREF", std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &FULL, &UNREFS } };
	hlasm_plugin::parser_library::checking::complex_operand overriden_operand { "OVERRIDE", std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &NOFAIL, &process_xref_true_two_operand, &FOLD, &process_sectalgn_true_operand, &NOCOMPAT, &NOSUPRWARN, &process_pcontrol_true_operand,
		&NORA2, &ERASE, &process_info_true_operand, &LIBMAC, &process_optable_true_operand, &NORENT} };
	std::vector<hlasm_plugin::parser_library::checking::one_operand*> test_process_true_three{ std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &overriden_operand } };

	std::vector<hlasm_plugin::parser_library::checking::one_operand*> test_process_true_one { std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &NOALIGN, &BATCH, &DBCS, &DX, &NOESD, &ILMA, &INFO, &NOMX, &NOPC, &DISK, &NOPROF, &RLD, &RX, &SEG, &NOTEST, &THR, &NOTC, &NOUS, &NOWORKFILE, &NOXREF } };

	std::vector<hlasm_plugin::parser_library::checking::one_operand*> test_process_true_two{ std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &process_codepage_true_operand, &process_compat_true_operand, &process_flag_true_operand, &process_machine_true_operand, &process_mxref_true_operand,
		&process_profile_true_operand, &process_suprwarn_true_operand, &process_typecheck_true_operand, &process_using_true_operand, &process_xref_true_one_operand } };
	std::vector<hlasm_plugin::parser_library::checking::one_operand*> test_process_false_one{ std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &process_codepage_false_operand } };
	std::vector<hlasm_plugin::parser_library::checking::one_operand*> test_process_false_two{std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &process_fail_false_operand } };
	std::vector<hlasm_plugin::parser_library::checking::one_operand*> test_process_false_three{ std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &process_flag_false_operand } };
	std::vector<hlasm_plugin::parser_library::checking::one_operand*> test_process_false_four{ std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &process_info_false_operand } };
	std::vector<hlasm_plugin::parser_library::checking::one_operand*> test_process_false_five{ std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &process_mxref_false_operand } };
	std::vector<hlasm_plugin::parser_library::checking::one_operand*> test_process_false_six{ std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &process_sectalgn_false_operand } };
	std::vector<hlasm_plugin::parser_library::checking::one_operand*> test_process_false_seven{ std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &process_typecheck_false_operand } };
	std::vector<hlasm_plugin::parser_library::checking::one_operand*> test_process_false_eight{ std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &process_using_false_operand } };
	std::vector<hlasm_plugin::parser_library::checking::one_operand*> test_process_false_nine{ std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ &process_xref_false_operand } };

	std::vector<hlasm_plugin::parser_library::checking::one_operand*> test_no_operand_true { std::vector<hlasm_plugin::parser_library::checking::one_operand*>
	{ } };*/
};

std::vector<one_operand *> unique_to_raw(const std::vector <std::unique_ptr<one_operand> > & to_convert)
{
	std::vector<one_operand *> converted;
	converted.resize(to_convert.size());
	for (size_t i = 0; i < to_convert.size(); ++i)
	{
		converted[i] = to_convert[i].get();
	}
	return converted;
}

TEST_F(instruction_test, process)
{
	EXPECT_FALSE(checker.check("*PROCESS", unique_to_raw(test_no_operand_true)));
	EXPECT_TRUE(checker.check("*PROCESS", unique_to_raw(test_process_true_one)));
	EXPECT_TRUE(checker.check("*PROCESS", unique_to_raw(test_process_true_two)));
	EXPECT_TRUE(checker.check("*PROCESS", unique_to_raw(test_process_true_three)));
	EXPECT_FALSE(checker.check("*PROCESS", unique_to_raw(test_process_false_one)));
	EXPECT_FALSE(checker.check("*PROCESS", unique_to_raw(test_process_false_two)));
	EXPECT_FALSE(checker.check("*PROCESS", unique_to_raw(test_process_false_three)));
	EXPECT_FALSE(checker.check("*PROCESS", unique_to_raw(test_process_false_four)));
	EXPECT_FALSE(checker.check("*PROCESS", unique_to_raw(test_process_false_five)));
	EXPECT_FALSE(checker.check("*PROCESS", unique_to_raw(test_process_false_six)));
	EXPECT_FALSE(checker.check("*PROCESS", unique_to_raw(test_process_false_seven)));
	EXPECT_FALSE(checker.check("*PROCESS", unique_to_raw(test_process_false_eight)));
	EXPECT_FALSE(checker.check("*PROCESS", unique_to_raw(test_process_false_nine)));
}

TEST_F(instruction_test, no_operand)
{
	EXPECT_TRUE(checker.check("LOCTR", unique_to_raw(test_no_operand_true)));
	EXPECT_FALSE(checker.check("LOCTR", unique_to_raw(test_acontrol_true)));
}

TEST_F(instruction_test, adata)
{
	EXPECT_FALSE(checker.check("ADATA", unique_to_raw(test_no_operand_true)));
	EXPECT_TRUE(checker.check("ADATA", unique_to_raw(test_adata_true_one)));
	EXPECT_TRUE(checker.check("ADATA", unique_to_raw(test_adata_true_two)));
	EXPECT_FALSE(checker.check("ADATA", unique_to_raw(test_adata_false)));
}

TEST_F(instruction_test, acontrol)
{
	EXPECT_FALSE(checker.check("ACONTROL", unique_to_raw(test_no_operand_true)));
	EXPECT_TRUE(checker.check("ACONTROL", unique_to_raw(test_acontrol_true)));
}

TEST_F(instruction_test, ainsert)
{
	EXPECT_TRUE(checker.check("AINSERT", unique_to_raw(test_ainsert_true_one)));
	EXPECT_TRUE(checker.check("AINSERT", unique_to_raw(test_ainsert_true_two)));
	EXPECT_FALSE(checker.check("AINSERT", unique_to_raw(test_ainsert_false)));
	EXPECT_FALSE(checker.check("AINSERT", unique_to_raw(test_no_operand_true)));
}

TEST_F(instruction_test, alias)
{
	EXPECT_FALSE(checker.check("ALIAS", unique_to_raw(test_alias_false)));
	EXPECT_TRUE(checker.check("ALIAS", unique_to_raw(test_alias_true_one)));
	EXPECT_TRUE(checker.check("ALIAS", unique_to_raw(test_alias_true_two)));
	EXPECT_FALSE(checker.check("ALIAS", unique_to_raw(test_acontrol_true)));
}

TEST_F(instruction_test, amode)
{
	EXPECT_FALSE(checker.check("AMODE", unique_to_raw(test_no_operand_true)));
	EXPECT_TRUE(checker.check("AMODE", unique_to_raw(test_amode_true)));
	EXPECT_FALSE(checker.check("AMODE", unique_to_raw(test_alias_true_one)));
}

TEST_F(instruction_test, cattr)
{
	EXPECT_FALSE(checker.check("CATTR", unique_to_raw(test_no_operand_true)));
	EXPECT_TRUE(checker.check("CATTR", unique_to_raw(test_cattr_true)));
}

TEST_F(instruction_test, expression)
{
	EXPECT_TRUE(checker.check("CEJECT", unique_to_raw(test_no_operand_true)));
	EXPECT_FALSE(checker.check("CEJECT", unique_to_raw(test_acontrol_true)));
	EXPECT_TRUE(checker.check("CEJECT", unique_to_raw(test_expression_true)));
}

TEST_F(instruction_test, ccw)
{
	EXPECT_FALSE(checker.check("CCW", unique_to_raw(test_no_operand_true)));
	EXPECT_TRUE(checker.check("CCW", unique_to_raw(test_ccw_true)));
	EXPECT_FALSE(checker.check("CCW", unique_to_raw(test_expression_true)));
	EXPECT_FALSE(checker.check("CCW", unique_to_raw(test_acontrol_true)));
}

TEST_F(instruction_test, cnop)
{
	EXPECT_FALSE(checker.check("CNOP", unique_to_raw(test_no_operand_true)));
	EXPECT_TRUE(checker.check("CNOP", unique_to_raw(test_cnop_true)));
	EXPECT_FALSE(checker.check("CNOP", unique_to_raw(test_cnop_one_false)));
	EXPECT_FALSE(checker.check("CNOP", unique_to_raw(test_cnop_two_false)));
	EXPECT_FALSE(checker.check("CNOP", unique_to_raw(test_acontrol_true)));
}

TEST_F(instruction_test, copy)
{
	EXPECT_FALSE(checker.check("COPY", unique_to_raw(test_no_operand_true)));
	EXPECT_TRUE(checker.check("COPY", unique_to_raw(test_copy_true)));
	EXPECT_FALSE(checker.check("COPY", unique_to_raw(test_acontrol_true)));
}

TEST_F(instruction_test, data)
{
	EXPECT_FALSE(checker.check("DXD", unique_to_raw(test_no_operand_true)));
	EXPECT_TRUE(checker.check("DXD", unique_to_raw(test_data_true_one)));
	EXPECT_TRUE(checker.check("DXD", unique_to_raw(test_data_true_two)));
}

TEST_F(instruction_test, drop)
{
	EXPECT_TRUE(checker.check("DROP", unique_to_raw(test_no_operand_true)));
	EXPECT_TRUE(checker.check("DROP", unique_to_raw(test_drop_true_one)));
	EXPECT_TRUE(checker.check("DROP", unique_to_raw(test_drop_true_two)));
	EXPECT_FALSE(checker.check("DROP", unique_to_raw(test_drop_false)));
}

TEST_F(instruction_test, end)
{
	EXPECT_TRUE(checker.check("END", unique_to_raw(test_no_operand_true)));
	EXPECT_TRUE(checker.check("END", unique_to_raw(test_end_true_one)));
	EXPECT_TRUE(checker.check("END", unique_to_raw(test_end_true_two)));
	EXPECT_FALSE(checker.check("END", unique_to_raw(test_end_false)));
}

TEST_F(instruction_test, entry)
{
	EXPECT_FALSE(checker.check("ENTRY", unique_to_raw(test_no_operand_true)));
	EXPECT_TRUE(checker.check("ENTRY", unique_to_raw(test_data_true_one)));
	EXPECT_TRUE(checker.check("ENTRY", unique_to_raw(test_ainsert_true_one)));
	EXPECT_FALSE(checker.check("ENTRY", unique_to_raw(test_end_true_one)));
}

TEST_F(instruction_test, equ)
{
	EXPECT_FALSE(checker.check("EQU", unique_to_raw(test_no_operand_true)));
	EXPECT_TRUE(checker.check("EQU", unique_to_raw(test_equ_true_one)));
	EXPECT_TRUE(checker.check("EQU", unique_to_raw(test_equ_true_two)));
	EXPECT_FALSE(checker.check("EQU", unique_to_raw(test_equ_false)));
}

TEST_F(instruction_test, exitctl)
{
	EXPECT_FALSE(checker.check("EXITCTL", unique_to_raw(test_no_operand_true)));
	EXPECT_TRUE(checker.check("EXITCTL", unique_to_raw(test_exitctl_true)));
	EXPECT_FALSE(checker.check("EXITCTL", unique_to_raw(test_exitctl_false_one)));
	EXPECT_FALSE(checker.check("EXITCTL", unique_to_raw(test_exitctl_false_one)));
}

TEST_F(instruction_test, external)
{
	EXPECT_FALSE(checker.check("EXTRN", unique_to_raw(test_no_operand_true)));
	EXPECT_TRUE(checker.check("EXTRN", unique_to_raw(test_extrn_true_one)));
	EXPECT_TRUE(checker.check("EXTRN", unique_to_raw(test_extrn_true_two)));
}

TEST_F(instruction_test, ictl)
{
	EXPECT_FALSE(checker.check("ICTL", unique_to_raw(test_no_operand_true)));
	EXPECT_TRUE(checker.check("ICTL", unique_to_raw(test_ictl_true_one)));
	EXPECT_TRUE(checker.check("ICTL", unique_to_raw(test_ictl_true_two)));
	EXPECT_FALSE(checker.check("ICTL", unique_to_raw(test_ictl_false_one)));
	EXPECT_FALSE(checker.check("ICTL", unique_to_raw(test_ictl_false_two)));
}

TEST_F(instruction_test, iseq)
{
	EXPECT_TRUE(checker.check("ISEQ", unique_to_raw(test_no_operand_true)));
	EXPECT_TRUE(checker.check("ISEQ", unique_to_raw(test_iseq_true)));
	EXPECT_FALSE(checker.check("ISEQ", unique_to_raw(test_iseq_false)));
	EXPECT_FALSE(checker.check("ISEQ", unique_to_raw(test_ainsert_true_one)));
	EXPECT_FALSE(checker.check("ISEQ", unique_to_raw(test_extrn_true_one)));
}

TEST_F(instruction_test, mnote)
{
	EXPECT_FALSE(checker.check("MNOTE", unique_to_raw(test_no_operand_true)));
	EXPECT_TRUE(checker.check("MNOTE", unique_to_raw(test_mnote_true_one)));
	EXPECT_TRUE(checker.check("MNOTE", unique_to_raw(test_mnote_true_two)));
	EXPECT_FALSE(checker.check("MNOTE", unique_to_raw(test_mnote_false_one)));
	EXPECT_FALSE(checker.check("MNOTE", unique_to_raw(test_mnote_false_two)));
}

TEST_F(instruction_test, opsyn)
{
	EXPECT_TRUE(checker.check("OPSYN", unique_to_raw(test_no_operand_true)));
	EXPECT_TRUE(checker.check("OPSYN", unique_to_raw(test_opsyn_true)));
	EXPECT_FALSE(checker.check("OPSYN", unique_to_raw(test_mnote_true_one)));
	EXPECT_FALSE(checker.check("OPSYN", unique_to_raw(test_extrn_true_two)));
}

TEST_F(instruction_test, org)
{
	EXPECT_TRUE(checker.check("ORG", unique_to_raw(test_no_operand_true)));
	EXPECT_TRUE(checker.check("ORG", unique_to_raw(test_org_true_one)));
	EXPECT_TRUE(checker.check("ORG", unique_to_raw(test_org_true_two)));
	EXPECT_FALSE(checker.check("ORG", unique_to_raw(test_org_false)));
	EXPECT_FALSE(checker.check("ORG", unique_to_raw(test_equ_true_two)));
}

TEST_F(instruction_test, stack)
{
	EXPECT_FALSE(checker.check("POP", unique_to_raw(test_no_operand_true)));
	EXPECT_TRUE(checker.check("POP", unique_to_raw(test_stack_true_one)));
	EXPECT_TRUE(checker.check("POP", unique_to_raw(test_stack_true_two)));
	EXPECT_FALSE(checker.check("POP", unique_to_raw(test_stack_false_one)));
	EXPECT_FALSE(checker.check("POP", unique_to_raw(test_stack_false_two)));
	EXPECT_FALSE(checker.check("POP", unique_to_raw(test_extrn_true_two)));
}

TEST_F(instruction_test, print)
{
	EXPECT_FALSE(checker.check("PRINT", unique_to_raw(test_no_operand_true)));
	EXPECT_TRUE(checker.check("PRINT", unique_to_raw(test_print_true)));
	EXPECT_FALSE(checker.check("PRINT", unique_to_raw(test_stack_true_one)));
	EXPECT_FALSE(checker.check("PRINT", unique_to_raw(test_extrn_true_two)));
}

TEST_F(instruction_test, punch)
{
	EXPECT_FALSE(checker.check("PUNCH", unique_to_raw(test_no_operand_true)));
	EXPECT_TRUE(checker.check("PUNCH", unique_to_raw(test_punch_true)));
	EXPECT_FALSE(checker.check("PUNCH", unique_to_raw(test_punch_false_one)));
	EXPECT_FALSE(checker.check("PUNCH", unique_to_raw(test_punch_false_two)));
	EXPECT_FALSE(checker.check("PUNCH", unique_to_raw(test_stack_true_one)));
	EXPECT_FALSE(checker.check("PUNCH", unique_to_raw(test_extrn_true_two)));
}

TEST_F(instruction_test, rmode)
{
	EXPECT_FALSE(checker.check("RMODE", unique_to_raw(test_no_operand_true)));
	EXPECT_TRUE(checker.check("RMODE", unique_to_raw(test_rmode_true_one)));
	EXPECT_TRUE(checker.check("RMODE", unique_to_raw(test_rmode_true_two)));
	EXPECT_FALSE(checker.check("RMODE", unique_to_raw(test_amode_true)));
	EXPECT_FALSE(checker.check("RMODE", unique_to_raw(test_extrn_true_two)));
}

TEST_F(instruction_test, title)
{
	EXPECT_FALSE(checker.check("TITLE", unique_to_raw(test_no_operand_true)));
	EXPECT_TRUE(checker.check("TITLE", unique_to_raw(test_punch_true)));
	EXPECT_FALSE(checker.check("TITLE", unique_to_raw(test_punch_false_two)));
	EXPECT_FALSE(checker.check("TITLE", unique_to_raw(test_punch_false_one)));
	EXPECT_FALSE(checker.check("TITLE", unique_to_raw(test_amode_true)));
	EXPECT_FALSE(checker.check("TITLE", unique_to_raw(test_extrn_true_two)));
}

TEST_F(instruction_test, using_instr)
{
	EXPECT_FALSE(checker.check("USING", unique_to_raw(test_no_operand_true)));
	EXPECT_FALSE(checker.check("USING", unique_to_raw(test_extrn_true_two)));
	EXPECT_TRUE(checker.check("USING", unique_to_raw(test_using_true_one)));
	EXPECT_TRUE(checker.check("USING", unique_to_raw(test_using_true_two)));
	EXPECT_TRUE(checker.check("USING", unique_to_raw(test_using_true_three)));
	EXPECT_FALSE(checker.check("USING", unique_to_raw(test_using_false_one)));
	EXPECT_FALSE(checker.check("USING", unique_to_raw(test_using_false_two)));
	EXPECT_FALSE(checker.check("USING", unique_to_raw(test_rmode_true_one)));
}

TEST_F(instruction_test, xattr)
{
	EXPECT_FALSE(checker.check("XATTR", unique_to_raw(test_no_operand_true)));
	EXPECT_TRUE(checker.check("XATTR", unique_to_raw(test_xattr_true)));
	EXPECT_FALSE(checker.check("XATTR", unique_to_raw(test_xattr_false_one)));
	EXPECT_FALSE(checker.check("XATTR", unique_to_raw(test_xattr_false_two)));
	EXPECT_FALSE(checker.check("XATTR", unique_to_raw(test_extrn_true_two)));
	EXPECT_FALSE(checker.check("XATTR", unique_to_raw(test_acontrol_true)));
}
