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

operand returns [operand_ptr op]
	: operand_not_empty					{$op = std::move($operand_not_empty.op);}
	|									{$op = std::make_unique<semantics::empty_operand>(provider.get_empty_range( _localctx->getStart()));};


operand_not_empty returns [operand_ptr op]
	: {MACH()}? mach_op																	
	{
		$op = std::move($mach_op.op);
	}
	| {ASM()}? asm_op																	
	{
		$op = std::move($asm_op.op);
	}
	| {DAT()}? data_def
	{
		$op = std::make_unique<data_def_operand>(std::move($data_def.value),provider.get_range($data_def.ctx));
	};

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
			op = std::make_unique<model_operand>(std::move(*$model_op.chain_opt),provider.get_range( $model_op.ctx)); 
		else
			op = std::make_unique<semantics::empty_operand>(provider.get_range( $model_op.ctx)); 
		operands.push_back(std::move(op));
		auto remarks = $remark_o.value ? remark_list{*$remark_o.value} : remark_list{};
		auto line_range = provider.get_range($model_op.ctx->getStart(),$remark_o.ctx->getStop());
		collector.set_operand_remark_field(std::move(operands), std::move(remarks), line_range);
	} EOF
	| {collector.set_operand_remark_field(provider.get_range(_localctx));} EOF;

op_list_mach returns [std::vector<operand_ptr> operands]
	: operand_mach (comma operand_mach)*												
	{
		auto& result = $operands;
		auto operands = $ctx->operand_mach();
		result.reserve(operands.size());
		for(auto&op:operands)
		result.push_back(std::move(op->op));
	};

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
			op = std::make_unique<model_operand>(std::move(*$model_op.chain_opt),provider.get_range( $model_op.ctx)); 
		else
			op = std::make_unique<semantics::empty_operand>(provider.get_range( $model_op.ctx)); 
		operands.push_back(std::move(op));
		auto remarks = $remark_o.value ? remark_list{*$remark_o.value} : remark_list{};
		auto line_range = provider.get_range($model_op.ctx->getStart(),$remark_o.ctx->getStop());
		collector.set_operand_remark_field(std::move(operands), std::move(remarks), line_range);
	} EOF
	| {collector.set_operand_remark_field(provider.get_range(_localctx));} EOF;

op_list_dat returns [std::vector<operand_ptr> operands]
	: {disable_litarals();} operand_dat (comma operand_dat)*
	{
		auto& result = $operands;
		auto operands = $ctx->operand_dat();
		result.reserve(operands.size());
		for(auto&op:operands)
		result.push_back(std::move(op->op));
	};
	finally
	{enable_litarals();}

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
			op = std::make_unique<model_operand>(std::move(*$model_op.chain_opt),provider.get_range( $model_op.ctx)); 
		else
			op = std::make_unique<semantics::empty_operand>(provider.get_range( $model_op.ctx)); 
		operands.push_back(std::move(op));
		auto remarks = $remark_o.value ? remark_list{*$remark_o.value} : remark_list{};
		auto line_range = provider.get_range($model_op.ctx->getStart(),$remark_o.ctx->getStop());
		collector.set_operand_remark_field(std::move(operands), std::move(remarks), line_range);
	} EOF
	| {collector.set_operand_remark_field(provider.get_range(_localctx));} EOF;

op_list_asm returns [std::vector<operand_ptr> operands]
	: operand_asm (comma operand_asm)*												
	{
		auto& result = $operands;
		auto operands = $ctx->operand_asm();
		result.reserve(operands.size());
		for(auto&op:operands)
		result.push_back(std::move(op->op));
	};

operand_asm returns [operand_ptr op]
	: asm_op							{$op = std::move($asm_op.op);}
	|									{$op = std::make_unique<semantics::empty_operand>(provider.get_empty_range( _localctx->getStart()));};


//////////////////////////////////////// ca

op_rem_body_ca
	:
	SPACE* EOF {collector.set_operand_remark_field(provider.get_range(_localctx));}
	|
	SPACE+ op_rem_body_alt_ca
	{
		auto line_range = provider.get_range($op_rem_body_alt_ca.ctx);
		collector.set_operand_remark_field(std::move($op_rem_body_alt_ca.line.operands), std::move($op_rem_body_alt_ca.line.remarks), line_range);
	} EOF
	| remark_o 
	{
		auto remarks = $remark_o.value ? remark_list{*$remark_o.value} : remark_list{};
		auto line_range = provider.get_range($remark_o.ctx);
		collector.set_operand_remark_field(operand_list(), std::move(remarks), line_range);
	} EOF;

op_rem_body_alt_ca returns [op_rem line]
	:
	(
		(
			ca_op? comma
			{
				if ($ca_op.ctx && $ca_op.op)
					$line.operands.push_back(std::move($ca_op.op));
				else
					$line.operands.push_back(std::make_unique<semantics::empty_operand>(provider.get_empty_range($comma.ctx->getStart())));
			}
		)+
		{enable_continuation();}
		(
			r1=remark_o CONTINUATION
			{
				if($r1.value) $line.remarks.push_back(std::move(*$r1.value));
			}
		)?
		{disable_continuation();}
	)*
	(
		last_ca_op=ca_op? last_remark=remark_o
		{
			if ($last_ca_op.ctx)
				$line.operands.push_back(std::move($last_ca_op.op));
			else
				$line.operands.push_back(std::make_unique<semantics::empty_operand>(provider.get_empty_range($remark_o.ctx->getStart())));
			if ($last_remark.value)
				$line.remarks.push_back(std::move(*$last_remark.value));
		}
	);

//////////////////////////////////////// mac

op_rem_body_mac returns [op_rem line, range line_range]
	:
	SPACE* EOF {$line_range = provider.get_range(_localctx);}
	|
	SPACE+ op_rem_body_alt_mac
	{
		$line = std::move($op_rem_body_alt_mac.line);
		$line_range = provider.get_range($op_rem_body_alt_mac.ctx);
	} EOF
	| remark_o 
	{
		$line.remarks = $remark_o.value ? remark_list{*$remark_o.value} : remark_list{};
		$line_range = provider.get_range($remark_o.ctx);
	} EOF;

op_rem_body_alt_mac returns [op_rem line]
	:
	(
		(
			mac_op? comma
			{
				if ($mac_op.ctx && $mac_op.op)
					$line.operands.push_back(std::move($mac_op.op));
				else
					$line.operands.push_back(std::make_unique<semantics::empty_operand>(provider.get_empty_range($comma.ctx->getStart())));
			}
		)+
		{enable_continuation();}
		(
			r1=remark_o CONTINUATION
			{
				if($r1.value) $line.remarks.push_back(std::move(*$r1.value));
			}
		)?
		{disable_continuation();}
	)*
	(
		last_mac_op=mac_op? last_remark=remark_o
		{
			if ($last_mac_op.ctx)
				$line.operands.push_back(std::move($last_mac_op.op));
			if ($last_remark.value)
				$line.remarks.push_back(std::move(*$last_remark.value));
		}
	);

/////////////

op_rem_body_ignored
	: .*? EOF;

op_rem_body_deferred
	: 
	{
		collector.set_operand_remark_field(provider.get_empty_range(_localctx->getStart()));
	} EOF
	| SPACE deferred_op_rem EOF
	{
		auto r = provider.get_range(_input->get($SPACE.index+1),_input->get(_input->size()-1));
		collector.set_operand_remark_field(_input->getText(misc::Interval($SPACE.index+1,_input->size()-1)),std::move($deferred_op_rem.var_list),std::move($deferred_op_rem.remarks),r);
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
			op = std::make_unique<model_operand>(std::move(*$model_op.chain_opt),provider.get_range( $model_op.ctx)); 
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
			op = std::make_unique<model_operand>(std::move(*$model_op.chain_opt),provider.get_range( $model_op.ctx)); 
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
			op = std::make_unique<model_operand>(std::move(*$model_op.chain_opt),provider.get_range( $model_op.ctx)); 
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
	} EOF
	| remark_o 
	{
		$line.remarks = $remark_o.value ? remark_list{*$remark_o.value} : remark_list{};
	} EOF;

op_rem_body_noop_r
	: remark_o EOF;