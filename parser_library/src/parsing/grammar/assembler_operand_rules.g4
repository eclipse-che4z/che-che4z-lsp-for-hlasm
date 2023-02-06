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
	:
	{ ALIAS() }? ORDSYMBOL string
	{
		auto range = provider.get_range($ORDSYMBOL,$string.ctx->getStop());
		collector.add_hl_symbol(token_info(provider.get_range($ORDSYMBOL),hl_scopes::self_def_type));
		$op = std::make_unique<expr_assembler_operand>(std::make_unique<mach_expr_default>(range),$ctx->getText(),range);
	}
	| id lpar asm_op_comma_c rpar
	{
		$op = std::make_unique<complex_assembler_operand>(
			$id.name.to_string(),std::move($asm_op_comma_c.asm_ops),
			provider.get_range($id.ctx->getStart(),$rpar.ctx->getStop())
			);
		collector.add_hl_symbol(token_info(provider.get_range($id.ctx),hl_scopes::operand));
	}
	|
	{ END() }? lpar id1=end_instr_word comma id2=end_instr_word comma id3=end_instr_word rpar
	{
		std::vector<std::unique_ptr<complex_assembler_operand::component_value_t>> language_triplet;
		range first_range = provider.get_range($id1.ctx);
		range second_range = provider.get_range($id2.ctx);
		range third_range = provider.get_range($id3.ctx);
		language_triplet.push_back(std::make_unique<complex_assembler_operand::string_value_t>($id1.value, first_range));
		language_triplet.push_back(std::make_unique<complex_assembler_operand::string_value_t>($id2.value, second_range));
		language_triplet.push_back(std::make_unique<complex_assembler_operand::string_value_t>($id3.value, third_range));
		collector.add_hl_symbol(token_info(first_range,hl_scopes::operand));
		collector.add_hl_symbol(token_info(second_range,hl_scopes::operand));
		collector.add_hl_symbol(token_info(third_range,hl_scopes::operand));
		$op = std::make_unique<complex_assembler_operand>(
			"",std::move(language_triplet),
			provider.get_range($lpar.ctx->getStart(),$rpar.ctx->getStop())
		);
	}
	| 
	lpar base=mach_expr comma end=mach_expr rpar
	{
		$op = std::make_unique<using_instr_assembler_operand>(
			std::move($base.m_e), 
			std::move($end.m_e),
			$base.text,
			$end.text,
			provider.get_range($lpar.ctx->getStart(),$rpar.ctx->getStop())
		);
	}
	| { !ALIAS() }? mach_expr
	{
		$op = std::make_unique<expr_assembler_operand>(std::move($mach_expr.m_e),utils::to_upper_copy($mach_expr.ctx->getText()),provider.get_range($mach_expr.ctx));
	}
	| string
	{
		$op = std::make_unique<string_assembler_operand>(std::move($string.value),provider.get_range($string.ctx));
	};

asm_op_inner returns [std::unique_ptr<complex_assembler_operand::component_value_t> op]
	: string
	{
		$op = std::make_unique<complex_assembler_operand::string_value_t>(
			std::move($string.value),
			provider.get_range($string.ctx));
	}
	| id
	{
		$op = std::make_unique<complex_assembler_operand::string_value_t>(
			$id.name.to_string(),
			provider.get_range($id.ctx));
		collector.add_hl_symbol(token_info(provider.get_range($id.ctx),hl_scopes::operand));
	}
	| num
	{
		$op = std::make_unique<complex_assembler_operand::int_value_t>($num.value, provider.get_range($num.ctx));
		collector.add_hl_symbol(token_info(provider.get_range($num.ctx),hl_scopes::operand));
	}
	|
	{
		$op = std::make_unique<complex_assembler_operand::string_value_t>("", provider.get_range(_localctx));
	}
	| id lpar asm_op_comma_c rpar									
	{ 
		$op = std::make_unique<complex_assembler_operand::composite_value_t>(
			$id.name.to_string(),
			std::move($asm_op_comma_c.asm_ops),
			provider.get_range($id.ctx->getStart(),$rpar.ctx->getStop()));
		collector.add_hl_symbol(token_info(provider.get_range($id.ctx),hl_scopes::operand));
	};

asm_op_comma_c returns [std::vector<std::unique_ptr<complex_assembler_operand::component_value_t>> asm_ops]
	: asm_op_inner														{$asm_ops.push_back(std::move($asm_op_inner.op));}
	| tmp=asm_op_comma_c comma asm_op_inner								{$tmp.asm_ops.push_back(std::move($asm_op_inner.op)); $asm_ops = std::move($tmp.asm_ops);};	

end_instr_word returns [std::string value]
	: (t=~(COMMA|CONTINUATION){$value.append($t.text);})+;
