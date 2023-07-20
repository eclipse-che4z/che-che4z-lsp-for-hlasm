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

 //rules for operand field
parser grammar operand_field_rules;

//////////////////////////////////////// mach

op_rem_body_mach
	: SPACE+ op_list_mach remark_o 
	{
		auto remarks = $remark_o.value ? remark_list{*$remark_o.value} : remark_list{};
		auto line_range = provider.get_range($op_list_mach.ctx->getStart(),$remark_o.ctx->getStop());
		collector.set_operand_remark_field(std::move($op_list_mach.operands), std::move(remarks), line_range);
	} EOF
	| SPACE+ model_op remark_o 
	{
		operand_ptr op;
		std::vector<operand_ptr> operands;
		if($model_op.chain_opt)
			op = std::make_unique<model_operand>(std::move(*$model_op.chain_opt),static_cast<lexing::token_stream*>(_input)->get_line_limits(),provider.get_range( $model_op.ctx));
		else
			op = std::make_unique<semantics::empty_operand>(provider.get_range( $model_op.ctx));
		operands.push_back(std::move(op));
		auto remarks = $remark_o.value ? remark_list{*$remark_o.value} : remark_list{};
		auto line_range = provider.get_range($model_op.ctx->getStart(),$remark_o.ctx->getStop());
		collector.set_operand_remark_field(std::move(operands), std::move(remarks), line_range);
	} EOF
	| {collector.set_operand_remark_field(provider.get_range(_localctx));} EOF;

op_list_mach returns [std::vector<operand_ptr> operands]
	: operand_mach {$operands.push_back(std::move($operand_mach.op));} (comma operand_mach {$operands.push_back(std::move($operand_mach.op));})*	
	;

operand_mach returns [operand_ptr op]
	: mach_op							{$op = std::move($mach_op.op);}
	|									{$op = std::make_unique<semantics::empty_operand>(provider.get_empty_range( _localctx->getStart()));};

//////////////////////////////////////// dat

op_rem_body_dat
	: SPACE+ op_list_dat remark_o 
	{
		auto remarks = $remark_o.value ? remark_list{*$remark_o.value} : remark_list{};
		auto line_range = provider.get_range($op_list_dat.ctx->getStart(),$remark_o.ctx->getStop());
		collector.set_operand_remark_field(std::move($op_list_dat.operands), std::move(remarks), line_range);
	} EOF
	| SPACE+ model_op remark_o 
	{
		operand_ptr op;
		std::vector<operand_ptr> operands;
		if($model_op.chain_opt)
			op = std::make_unique<model_operand>(std::move(*$model_op.chain_opt),static_cast<lexing::token_stream*>(_input)->get_line_limits(),provider.get_range( $model_op.ctx));
		else
			op = std::make_unique<semantics::empty_operand>(provider.get_range( $model_op.ctx));
		operands.push_back(std::move(op));
		auto remarks = $remark_o.value ? remark_list{*$remark_o.value} : remark_list{};
		auto line_range = provider.get_range($model_op.ctx->getStart(),$remark_o.ctx->getStop());
		collector.set_operand_remark_field(std::move(operands), std::move(remarks), line_range);
	} EOF
	| {collector.set_operand_remark_field(provider.get_range(_localctx));} EOF;

op_list_dat returns [std::vector<operand_ptr> operands]
	: {auto lit_restore = disable_literals();} operand_dat {$operands.push_back(std::move($operand_dat.op));} (comma operand_dat {$operands.push_back(std::move($operand_dat.op));})*
	;

operand_dat returns [operand_ptr op]
	: dat_op							{$op = std::move($dat_op.op);}
	|									{$op = std::make_unique<semantics::empty_operand>(provider.get_empty_range( _localctx->getStart()));};

//////////////////////////////////////// asm

op_rem_body_asm
	: SPACE+ op_list_asm remark_o 
	{
		auto remarks = $remark_o.value ? remark_list{*$remark_o.value} : remark_list{};
		auto line_range = provider.get_range($op_list_asm.ctx->getStart(),$remark_o.ctx->getStop());
		collector.set_operand_remark_field(std::move($op_list_asm.operands), std::move(remarks), line_range);
	} EOF
	| SPACE+ model_op remark_o 
	{
		operand_ptr op;
		std::vector<operand_ptr> operands;
		if($model_op.chain_opt)
			op = std::make_unique<model_operand>(std::move(*$model_op.chain_opt),static_cast<lexing::token_stream*>(_input)->get_line_limits(),provider.get_range( $model_op.ctx));
		else
			op = std::make_unique<semantics::empty_operand>(provider.get_range( $model_op.ctx));
		operands.push_back(std::move(op));
		auto remarks = $remark_o.value ? remark_list{*$remark_o.value} : remark_list{};
		auto line_range = provider.get_range($model_op.ctx->getStart(),$remark_o.ctx->getStop());
		collector.set_operand_remark_field(std::move(operands), std::move(remarks), line_range);
	} EOF
	| {collector.set_operand_remark_field(provider.get_range(_localctx));} EOF;

op_list_asm returns [std::vector<operand_ptr> operands]
	: operand_asm {$operands.push_back(std::move($operand_asm.op));} (comma operand_asm {$operands.push_back(std::move($operand_asm.op));})*
	;

operand_asm returns [operand_ptr op]
	: asm_op							{$op = std::move($asm_op.op);}
	|									{$op = std::make_unique<semantics::empty_operand>(provider.get_empty_range( _localctx->getStart()));};


/////////////

op_rem_body_ignored
	: .*? EOF;

op_rem_body_deferred
	: 
	{
		collector.set_operand_remark_field(provider.get_empty_range(_localctx->getStart()));
	} EOF
	| SPACE+ deferred_op_rem EOF
	{
		auto r = provider.get_range(_input->get($SPACE.index+1),_input->get(_input->size()-1));
		collector.set_operand_remark_field(_input->getText(misc::Interval($SPACE.index+1,_input->size()-1)),std::move($deferred_op_rem.var_list),std::move($deferred_op_rem.remarks),r, static_cast<hlasm_plugin::parser_library::lexing::token*>(_input->get($SPACE.index+1))->get_logical_column());
	} EOF;

op_rem_body_noop
	: remark_o 
	{
		collector.set_operand_remark_field(operand_list{},$remark_o.value ? remark_list{*$remark_o.value} : remark_list{}, provider.get_range( $remark_o.ctx));
	} EOF;

//////////////////////////////////////// mach_r

op_rem_body_mach_r returns [op_rem line]
	: op_list_mach remark_o 
	{
		$line.operands = std::move($op_list_mach.operands);
		$line.remarks = $remark_o.value ? remark_list{*$remark_o.value} : remark_list{};
	} EOF
	| model_op remark_o 
	{
		operand_ptr op;
		if($model_op.chain_opt)
			op = std::make_unique<model_operand>(std::move(*$model_op.chain_opt),static_cast<lexing::token_stream*>(_input)->get_line_limits(),provider.get_range( $model_op.ctx));
		else
			op = std::make_unique<semantics::empty_operand>(provider.get_range( $model_op.ctx));
		$line.operands.push_back(std::move(op));
		$line.remarks = $remark_o.value ? remark_list{*$remark_o.value} : remark_list{};
	} EOF
	| EOF;

//////////////////////////////////////// dat_r

op_rem_body_dat_r returns [op_rem line]
	: op_list_dat remark_o 
	{
		$line.operands = std::move($op_list_dat.operands);
		$line.remarks = $remark_o.value ? remark_list{*$remark_o.value} : remark_list{};
	} EOF
	| model_op remark_o 
	{
		operand_ptr op;
		if($model_op.chain_opt)
			op = std::make_unique<model_operand>(std::move(*$model_op.chain_opt),static_cast<lexing::token_stream*>(_input)->get_line_limits(),provider.get_range( $model_op.ctx));
		else
			op = std::make_unique<semantics::empty_operand>(provider.get_range( $model_op.ctx));
		$line.operands.push_back(std::move(op));
		$line.remarks = $remark_o.value ? remark_list{*$remark_o.value} : remark_list{};
	} EOF
	| EOF;

//////////////////////////////////////// asm_r

op_rem_body_asm_r returns [op_rem line]
	: op_list_asm remark_o 
	{
		$line.operands = std::move($op_list_asm.operands);
		$line.remarks = $remark_o.value ? remark_list{*$remark_o.value} : remark_list{};
	} EOF
	| model_op remark_o 
	{
		operand_ptr op;
		if($model_op.chain_opt)
			op = std::make_unique<model_operand>(std::move(*$model_op.chain_opt),static_cast<lexing::token_stream*>(_input)->get_line_limits(),provider.get_range( $model_op.ctx));
		else
			op = std::make_unique<semantics::empty_operand>(provider.get_range( $model_op.ctx));
		$line.operands.push_back(std::move(op));
		$line.remarks = $remark_o.value ? remark_list{*$remark_o.value} : remark_list{};
	} EOF
	| EOF;

//////////////////////////////////////// mac_r

op_rem_body_mac_r returns [op_rem line]
	:
	SPACE* EOF
	|
	op_rem_body_alt_mac
	{
		$line = std::move($op_rem_body_alt_mac.line);
	} EOF;

op_rem_body_noop_r
	: remark_o EOF;