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

 //rules for assembler operand
parser grammar assembler_operand_rules; 

asm_op returns [operand_ptr op]
	: id lpar asm_op_comma_c rpar
	{
		$op = std::make_unique<complex_assembler_operand>(
			*$id.name,std::move($asm_op_comma_c.asm_ops),
			provider.get_range($id.ctx->getStart(),$rpar.ctx->getStop())
			);
		collector.add_lsp_symbol($id.name,provider.get_range($id.ctx->getStart(),$rpar.ctx->getStop()),symbol_type::ord);
	}
	| lpar id1=end_instr_word comma id2=end_instr_word comma id3=end_instr_word rpar
	{

		std::vector<std::unique_ptr<complex_assembler_operand::component_value_t>> language_triplet;
		range first_range = provider.get_range($id1.ctx);
		range second_range = provider.get_range($id2.ctx);
		range third_range = provider.get_range($id3.ctx);
		language_triplet.push_back(std::make_unique<complex_assembler_operand::string_value_t>(std::move($id1.value), first_range));
		language_triplet.push_back(std::make_unique<complex_assembler_operand::string_value_t>(std::move($id2.value), second_range));
		language_triplet.push_back(std::make_unique<complex_assembler_operand::string_value_t>(std::move($id3.value), third_range));
		$op = std::make_unique<complex_assembler_operand>(
			"",std::move(language_triplet),
			provider.get_range($lpar.ctx->getStart(),$rpar.ctx->getStop())
		);
	}
	| lpar base=mach_expr comma end=mach_expr rpar
	{
		$op = std::make_unique<using_instr_assembler_operand>(
			std::move($base.m_e), 
			std::move($end.m_e),
			provider.get_range($lpar.ctx->getStart(),$rpar.ctx->getStop())
		);
	}
	| mach_expr
	{
		std::string upper_case = $mach_expr.ctx->getText();
		context::to_upper(upper_case);
		$op = std::make_unique<expr_assembler_operand>(std::move($mach_expr.m_e),upper_case,provider.get_range($mach_expr.ctx));
	}
	| string
	{
		$op = std::make_unique<string_assembler_operand>(std::move($string.value),provider.get_range($string.ctx));
	};

asm_op_inner returns [std::unique_ptr<complex_assembler_operand::component_value_t> op]
	: string														{ $op = std::make_unique<complex_assembler_operand::string_value_t>(std::move($string.value),
																			provider.get_range($string.ctx)); }	
	| id															{ $op = std::make_unique<complex_assembler_operand::string_value_t>(*$id.name,
																			provider.get_range($id.ctx));
																			collector.add_lsp_symbol($id.name,provider.get_range($id.ctx),symbol_type::ord); }	
	| num															{ $op = std::make_unique<complex_assembler_operand::int_value_t>($num.value,
																			provider.get_range($num.ctx)); }
	|																{ $op = std::make_unique<complex_assembler_operand::string_value_t>("", provider.get_range(_localctx)); }
	| id lpar asm_op_comma_c rpar									
	{ 
		$op = std::make_unique<complex_assembler_operand::composite_value_t>(
			*$id.name,
			std::move($asm_op_comma_c.asm_ops),
			provider.get_range($id.ctx->getStart(),$rpar.ctx->getStop()));
	};

asm_op_comma_c returns [std::vector<std::unique_ptr<complex_assembler_operand::component_value_t>> asm_ops]
	: asm_op_inner														{$asm_ops.push_back(std::move($asm_op_inner.op));}
	| tmp=asm_op_comma_c COMMA asm_op_inner								{$tmp.asm_ops.push_back(std::move($asm_op_inner.op)); $asm_ops = std::move($tmp.asm_ops);};	

end_instr_word returns [std::string value]
	: ORDSYMBOL															{$value = $ORDSYMBOL->getText();}
	| IDENTIFIER														{$value = $IDENTIFIER->getText();}
	| NUM																{$value = $NUM->getText();};