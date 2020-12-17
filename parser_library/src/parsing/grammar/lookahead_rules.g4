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

look_lab_instr  returns [std::optional<std::string> op_text, range op_range]
	: seq_symbol ~EOLLN* EOLLN
	{
		collector.set_label_field($seq_symbol.ss,provider.get_range($seq_symbol.ctx));
		collector.set_instruction_field(provider.get_range($seq_symbol.ctx));
		collector.set_operand_remark_field(provider.get_range($seq_symbol.ctx));
		//ctx->set_source_indices(statement_start().file_offset, statement_end().file_offset, statement_end().file_line);
		set_source_indices($seq_symbol.ctx->getStart(), $EOLLN);
	}
	| ORDSYMBOL? SPACE instruction operand_field_rest EOLLN
	{
		if ($ORDSYMBOL)
		{
			auto r = provider.get_range($ORDSYMBOL);
			auto id = ctx->ids().add($ORDSYMBOL->getText());
			collector.set_label_field(id,nullptr,r); 
		}
		//ctx->set_source_indices(statement_start().file_offset, statement_end().file_offset, statement_end().file_line);
		
		if ($ORDSYMBOL)
			set_source_indices($ORDSYMBOL, $EOLLN);
		else
			set_source_indices($SPACE, $EOLLN);
		
		$op_text = $operand_field_rest.ctx->getText();
		$op_range = provider.get_range($operand_field_rest.ctx);
	}
	| bad_look EOLLN
	{
		collector.set_label_field(provider.get_range(_localctx));
		collector.set_instruction_field(provider.get_range(_localctx));
		collector.set_operand_remark_field(provider.get_range(_localctx));
		//ctx->set_source_indices(statement_start().file_offset, statement_end().file_offset, statement_end().file_line);
		set_source_indices($bad_look.ctx->getStart(), $EOLLN);
	}
	| EOF	{finished_flag=true;};

bad_look
	: ~(ORDSYMBOL|DOT|SPACE|EOLLN) ~EOLLN*
	| DOT ~(ORDSYMBOL|EOLLN) ~EOLLN*
	| ORDSYMBOL ~(SPACE|EOLLN) ~EOLLN*
	| ORDSYMBOL SPACE?
	| SPACE
	| DOT
	| ;

lookahead_operands_and_remarks
	: SPACE+ lookahead_operand_list remark_o
	{
		range r = provider.get_range($lookahead_operand_list.ctx);
		collector.set_operand_remark_field(std::move($lookahead_operand_list.operands), std::vector<range>(), r);
	} EOLLN EOF
	| SPACE?
	{
		range r = provider.get_range(_localctx);
		collector.set_operand_remark_field(operand_list(), std::vector<range>(), r);
	} EOLLN EOF;

lookahead_operand_list returns [operand_list operands]
	: operand											{$operands.push_back(std::move($operand.op));}
	| tmp=lookahead_operand_list COMMA operand			{$tmp.operands.push_back(std::move($operand.op)); $operands = std::move($tmp.operands);};

lookahead_ignored_part
	: ~EOLLN*;
