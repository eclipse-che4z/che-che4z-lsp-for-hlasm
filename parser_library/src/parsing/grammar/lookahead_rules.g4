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

 //rules for lookahead statement
parser grammar lookahead_rules; 

look_lab_instr  returns [std::optional<std::string> op_text, range op_range, size_t op_logical_column = 0]
	: DOT s=ORDSYMBOL (SPACE instr=ORDSYMBOL? lookahead_operand_field_rest)?
	{
		auto seq_symbol = seq_sym{parse_identifier($s->getText(),provider.get_range($s)),provider.get_range($DOT, $s)};
		collector.set_label_field(seq_symbol,seq_symbol.symbol_range);
		if ($instr && $lookahead_operand_field_rest.valid)
		{
			collector.set_instruction_field(parse_identifier($instr->getText(),provider.get_range($instr)),provider.get_range($instr));
			collector.set_operand_remark_field(provider.get_range($lookahead_operand_field_rest.ctx));
			$op_text = get_context_text($lookahead_operand_field_rest.ctx);
			$op_range = provider.get_range($lookahead_operand_field_rest.ctx);
			$op_logical_column = static_cast<hlasm_plugin::parser_library::lexing::token*>($lookahead_operand_field_rest.start)->get_logical_column();
		}
		else
		{
			collector.set_instruction_field(seq_symbol.symbol_range);
			collector.set_operand_remark_field(seq_symbol.symbol_range);
		}
	}
	| lab=ORDSYMBOL SPACE instr=ORDSYMBOL lookahead_operand_field_rest
	{
		collector.set_label_field(add_id($lab->getText()),$lab->getText(),nullptr,provider.get_range($lab));
		if ($instr && $lookahead_operand_field_rest.valid)
		{
			collector.set_instruction_field(parse_identifier($instr->getText(),provider.get_range($instr)),provider.get_range($instr));
			collector.set_operand_remark_field(provider.get_range($lookahead_operand_field_rest.ctx));
			$op_text = get_context_text($lookahead_operand_field_rest.ctx);
			$op_range = provider.get_range($lookahead_operand_field_rest.ctx);
			$op_logical_column = static_cast<hlasm_plugin::parser_library::lexing::token*>($lookahead_operand_field_rest.start)->get_logical_column();
		}
		else
		{
			collector.set_instruction_field(provider.get_empty_range($SPACE));
			collector.set_operand_remark_field(provider.get_empty_range($SPACE));
		}
	}
	| SPACE instr=ORDSYMBOL lookahead_operand_field_rest
	{
		collector.set_label_field(provider.get_empty_range($SPACE));
		if ($instr && $lookahead_operand_field_rest.valid)
		{
			collector.set_instruction_field(parse_identifier($instr->getText(),provider.get_range($instr)),provider.get_range($instr));
			collector.set_operand_remark_field(provider.get_range($lookahead_operand_field_rest.ctx));
			$op_text = get_context_text($lookahead_operand_field_rest.ctx);
			$op_range = provider.get_range($lookahead_operand_field_rest.ctx);
			$op_logical_column = static_cast<hlasm_plugin::parser_library::lexing::token*>($lookahead_operand_field_rest.start)->get_logical_column();
		}
		else
		{
			collector.set_instruction_field(provider.get_empty_range($SPACE));
			collector.set_operand_remark_field(provider.get_empty_range($SPACE));
		}
	}
	;
	catch[RecognitionException&]
	{
		collector.set_label_field(provider.get_empty_range(_input->LT(1)));
		collector.set_instruction_field(provider.get_empty_range(_input->LT(1)));
		collector.set_operand_remark_field(provider.get_empty_range(_input->LT(1)));
	}

lookahead_operand_field_rest returns [bool valid = false]
	: SPACE (~EOF)* {$valid=true;}
	| EOF {$valid=true;}
	|
	;

lookahead_operands_and_remarks_asm
	: SPACE+
	(
		lookahead_operand_list_asm
		{
			range r = provider.get_range($lookahead_operand_list_asm.ctx);
			collector.set_operand_remark_field(std::move($lookahead_operand_list_asm.operands), std::vector<range>(), r);
		}
		|
		EOF
		{
			range r = provider.get_range(_localctx);
			collector.set_operand_remark_field(operand_list(), std::vector<range>(), r);
		}
	)
	| EOF
	{
		range r = provider.get_range(_localctx);
		collector.set_operand_remark_field(operand_list(), std::vector<range>(), r);
	};

lookahead_operands_and_remarks_dat
	: SPACE+
	(
		data_def
		{
			operand_list operands;
			operands.push_back(std::make_unique<data_def_operand>(std::move($data_def.value),provider.get_range($data_def.ctx)));
			range r = provider.get_range($data_def.ctx);
			collector.set_operand_remark_field(std::move(operands), std::vector<range>(), r);
		}
		|
		EOF
		{
			range r = provider.get_range(_localctx);
			collector.set_operand_remark_field(operand_list(), std::vector<range>(), r);
		}
	)
	| EOF
	{
		range r = provider.get_range(_localctx);
		collector.set_operand_remark_field(operand_list(), std::vector<range>(), r);
	};

lookahead_operand_list_asm returns [operand_list operands] locals [bool failed = false]
	: f=lookahead_operand_asm[&$failed]
	{
		{$operands.push_back(std::move($f.op));}
	}
	(
		COMMA n=lookahead_operand_asm[&$failed]
		{
			if($failed)
				$operands.push_back(std::make_unique<semantics::empty_operand>(provider.get_range($n.ctx)));
			else
				$operands.push_back(std::move($n.op));
		}
	)*;

lookahead_operand_asm[bool* failed] returns [operand_ptr op]
	:
	(
		asm_op
		{
			$op = std::move($asm_op.op);
		}
		|
		{$op = std::make_unique<semantics::empty_operand>(provider.get_empty_range( _localctx->getStart()));}
	)
	(
		(~(COMMA|SPACE|EOF))+
		{
			*$failed = true;
			$op = std::make_unique<semantics::empty_operand>(provider.get_empty_range( _localctx->getStart()));
		}
	)?
	;
