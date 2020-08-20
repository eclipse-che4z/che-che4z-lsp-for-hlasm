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

op_rem_body_mach returns [op_rem line, range line_range]
	: SPACE+ op_list_mach remark_o 
	{
		$line.operands = std::move($op_list_mach.operands);
		$line.remarks = $remark_o.value ? remark_list{*$remark_o.value} : remark_list{};
		$line_range = provider.get_range($op_list_mach.ctx->getStart(),$remark_o.ctx->getStop());
	} EOLLN EOF
	| SPACE+ model_op remark_o 
	{
		operand_ptr op;
		if($model_op.chain_opt)
			op = std::make_unique<model_operand>(std::move(*$model_op.chain_opt),provider.get_range( $model_op.ctx)); 
		else
			op = std::make_unique<semantics::empty_operand>(provider.get_range( $model_op.ctx)); 
		$line.operands.push_back(std::move(op));
		$line.remarks = $remark_o.value ? remark_list{*$remark_o.value} : remark_list{};
		$line_range = provider.get_range($model_op.ctx->getStart(),$remark_o.ctx->getStop());
	} EOLLN EOF
	| {$line_range = provider.get_range(_localctx);} EOLLN EOF;

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

op_rem_body_dat returns [op_rem line, range line_range]
	: SPACE+ op_list_dat remark_o 
	{
		$line.operands = std::move($op_list_dat.operands);
		$line.remarks = $remark_o.value ? remark_list{*$remark_o.value} : remark_list{};
		$line_range = provider.get_range($op_list_dat.ctx->getStart(),$remark_o.ctx->getStop());
	} EOLLN EOF
	| SPACE+ model_op remark_o 
	{
		operand_ptr op;
		if($model_op.chain_opt)
			op = std::make_unique<model_operand>(std::move(*$model_op.chain_opt),provider.get_range( $model_op.ctx)); 
		else
			op = std::make_unique<semantics::empty_operand>(provider.get_range( $model_op.ctx)); 
		$line.operands.push_back(std::move(op));
		$line.remarks = $remark_o.value ? remark_list{*$remark_o.value} : remark_list{};
		$line_range = provider.get_range($model_op.ctx->getStart(),$remark_o.ctx->getStop());
	} EOLLN EOF
	| {$line_range = provider.get_range(_localctx);} EOLLN EOF;

op_list_dat returns [std::vector<operand_ptr> operands]
	: operand_dat (comma operand_dat)*												
	{
		auto& result = $operands;
		auto operands = $ctx->operand_dat();
		result.reserve(operands.size());
		for(auto&op:operands)
		result.push_back(std::move(op->op));
	};

operand_dat returns [operand_ptr op]
	: dat_op							{$op = std::move($dat_op.op);}
	|									{$op = std::make_unique<semantics::empty_operand>(provider.get_empty_range( _localctx->getStart()));};

//////////////////////////////////////// asm

op_rem_body_asm returns [op_rem line, range line_range]
	: SPACE+ op_list_asm remark_o 
	{
		$line.operands = std::move($op_list_asm.operands);
		$line.remarks = $remark_o.value ? remark_list{*$remark_o.value} : remark_list{};
		$line_range = provider.get_range($op_list_asm.ctx->getStart(),$remark_o.ctx->getStop());
	} EOLLN EOF
	| SPACE+ model_op remark_o 
	{
		operand_ptr op;
		if($model_op.chain_opt)
			op = std::make_unique<model_operand>(std::move(*$model_op.chain_opt),provider.get_range( $model_op.ctx)); 
		else
			op = std::make_unique<semantics::empty_operand>(provider.get_range( $model_op.ctx)); 
		$line.operands.push_back(std::move(op));
		$line.remarks = $remark_o.value ? remark_list{*$remark_o.value} : remark_list{};
		$line_range = provider.get_range($model_op.ctx->getStart(),$remark_o.ctx->getStop());
	} EOLLN EOF
	| {$line_range = provider.get_range(_localctx);} EOLLN EOF;

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

op_rem_body_ca returns [op_rem line, range line_range]
	: SPACE+ op_rem_body_alt_ca 
	{
		$line = std::move($op_rem_body_alt_ca.line);
		$line_range = provider.get_range($op_rem_body_alt_ca.ctx);
	} EOLLN EOF
	| remark_o 
	{
		$line.remarks = $remark_o.value ? remark_list{*$remark_o.value} : remark_list{};
		$line_range = provider.get_range($remark_o.ctx);
	} EOLLN EOF
	| {$line_range = provider.get_range(_localctx);} EOLLN EOF;

op_rem_body_alt_ca returns [op_rem line]
	: alt_op_list_comma_ca cont_ca												
	{
		$line.operands = std::move($alt_op_list_comma_ca.operands);
		$line.operands.insert($line.operands.end(), std::make_move_iterator($cont_ca.line.operands.begin()), std::make_move_iterator($cont_ca.line.operands.end()));
		$line.remarks = std::move($cont_ca.line.remarks);
	}
	| alt_op_list_comma_o_ca ca_op remark_o
	{
		$alt_op_list_comma_o_ca.operands.push_back(std::move($ca_op.op)); 
		$line.operands = std::move($alt_op_list_comma_o_ca.operands); 
		$line.remarks = $remark_o.value ? remark_list{*$remark_o.value} : remark_list{};
	};
	

cont_ca returns [op_rem line]
	: {enable_continuation();} cont_body_ca {disable_continuation();}						{$line = std::move($cont_body_ca.line);};

cont_body_ca returns [op_rem line]
	: remark_o																					
	{ 
		auto tmp = std::make_unique<semantics::empty_operand>(provider.get_empty_range( $remark_o.ctx->getStart()));
		$line.operands.push_back(std::move(tmp));
		$line.remarks = $remark_o.value ? remark_list{*$remark_o.value} : remark_list{};
	}
	| r1=remark_o CONTINUATION {disable_continuation();} /*empty op*/ r2=remark_o			
	{
		if($r1.value) $line.remarks.push_back(*$r1.value); 
		auto tmp = std::make_unique<semantics::empty_operand>(range(provider.get_range( $r2.ctx).start));
		$line.operands.push_back(std::move(tmp));
		if($r2.value) $line.remarks.push_back(*$r2.value); 
	}
	| remark_o CONTINUATION {disable_continuation();} next=op_rem_body_alt_ca
	{
		$line.remarks = $remark_o.value ? remark_list{*$remark_o.value} : remark_list{};
		$line.remarks.insert($line.remarks.end(),std::make_move_iterator($next.line.remarks.begin()),std::make_move_iterator($next.line.remarks.end()));
		$line.operands = std::move($next.line.operands);
	};

alt_op_list_comma_o_ca returns [std::vector<operand_ptr> operands]
	:
	| alt_op_list_comma_ca													{$operands = std::move($alt_op_list_comma_ca.operands);};

alt_op_list_comma_ca returns [std::vector<operand_ptr> operands]
	: alt_operand_ca comma													{$operands.push_back(std::move($alt_operand_ca.op)); }
	| tmp=alt_op_list_comma_ca alt_operand_ca comma							{$tmp.operands.push_back(std::move($alt_operand_ca.op)); $operands = std::move($tmp.operands); };

alt_operand_ca returns [operand_ptr op]
	: ca_op					{$op = std::move($ca_op.op);}
	|						{$op = std::make_unique<semantics::empty_operand>(provider.get_empty_range( _localctx->getStart()));};

//////////////////////////////////////// mac

op_rem_body_mac returns [op_rem line, range line_range]
	: SPACE+ op_rem_body_alt_mac
	{
		parse_macro_operands($op_rem_body_alt_mac.line);
		$line = std::move($op_rem_body_alt_mac.line);
		$line_range = provider.get_range($op_rem_body_alt_mac.ctx);
	} EOLLN EOF
	| remark_o 
	{
		$line.remarks = $remark_o.value ? remark_list{*$remark_o.value} : remark_list{};
		$line_range = provider.get_range($remark_o.ctx);
	} EOLLN EOF
	| {$line_range = provider.get_range(_localctx);} EOLLN EOF;

op_rem_body_alt_mac returns [op_rem line]
	: alt_op_list_comma_mac cont_mac												
	{
		$line.operands = std::move($alt_op_list_comma_mac.operands);
		$line.operands.insert($line.operands.end(), std::make_move_iterator($cont_mac.line.operands.begin()), std::make_move_iterator($cont_mac.line.operands.end()));
		$line.remarks = std::move($cont_mac.line.remarks);
	}
	| alt_op_list_comma_o_mac mac_op remark_o
	{
		$alt_op_list_comma_o_mac.operands.push_back(std::move($mac_op.op)); 
		$line.operands = std::move($alt_op_list_comma_o_mac.operands); 
		$line.remarks = $remark_o.value ? remark_list{*$remark_o.value} : remark_list{};
	};
	

cont_mac returns [op_rem line]
	: {enable_continuation();} cont_body_mac {disable_continuation();}						{$line = std::move($cont_body_mac.line);};

cont_body_mac returns [op_rem line]
	: remark_o																					
	{ 
		auto tmp = std::make_unique<semantics::empty_operand>(provider.get_empty_range( $remark_o.ctx->getStart()));
		$line.operands.push_back(std::move(tmp));
		$line.remarks = $remark_o.value ? remark_list{*$remark_o.value} : remark_list{};
	}
	| r1=remark_o CONTINUATION {disable_continuation();} /*empty op*/ r2=remark_o			
	{
		if($r1.value) $line.remarks.push_back(*$r1.value); 
		auto tmp = std::make_unique<semantics::empty_operand>(range(provider.get_range( $r2.ctx).start));
		$line.operands.push_back(std::move(tmp));
		if($r2.value) $line.remarks.push_back(*$r2.value); 
	}
	| remark_o CONTINUATION {disable_continuation();} next=op_rem_body_alt_mac
	{
		$line.remarks = $remark_o.value ? remark_list{*$remark_o.value} : remark_list{};
		$line.remarks.insert($line.remarks.end(),std::make_move_iterator($next.line.remarks.begin()),std::make_move_iterator($next.line.remarks.end()));
		$line.operands = std::move($next.line.operands);
	};

alt_op_list_comma_o_mac returns [std::vector<operand_ptr> operands]
	:
	| alt_op_list_comma_mac													{$operands = std::move($alt_op_list_comma_mac.operands);};

alt_op_list_comma_mac returns [std::vector<operand_ptr> operands]
	: alt_operand_mac comma													{$operands.push_back(std::move($alt_operand_mac.op)); }
	| tmp=alt_op_list_comma_mac alt_operand_mac comma							{$tmp.operands.push_back(std::move($alt_operand_mac.op)); $operands = std::move($tmp.operands); };

alt_operand_mac returns [operand_ptr op]
	: mac_op					{$op = std::move($mac_op.op);}
	|							{$op = std::make_unique<semantics::empty_operand>(provider.get_empty_range( _localctx->getStart()));};

/////////////

op_rem_body_ignored
	: (~EOLLN)* EOLLN EOF;

op_rem_body_deferred
	: 
	{
		collector.set_operand_remark_field(provider.get_empty_range(_localctx->getStart()));
		collector.add_operands_hl_symbols();
		collector.add_remarks_hl_symbols();
	} EOLLN EOF
	| SPACE {enable_hidden();} deferred_op_rem {disable_hidden();}
	{
		auto r = provider.get_range( $deferred_op_rem.ctx);
		collector.set_operand_remark_field($deferred_op_rem.ctx->getText(),std::move($deferred_op_rem.remarks),r);
		collector.add_operands_hl_symbols();
		collector.add_remarks_hl_symbols();
	} EOLLN EOF;

op_rem_body_noop
	: remark_o 
	{
		collector.set_operand_remark_field(operand_list{},$remark_o.value ? remark_list{*$remark_o.value} : remark_list{}, provider.get_range( $remark_o.ctx));
		collector.add_operands_hl_symbols();
		collector.add_remarks_hl_symbols();
	} EOLLN EOF;

//////////////////////////////////////// mach_r

op_rem_body_mach_r returns [op_rem line]
	: op_list_mach remark_o 
	{
		$line.operands = std::move($op_list_mach.operands);
		$line.remarks = $remark_o.value ? remark_list{*$remark_o.value} : remark_list{};
	} EOLLN EOF
	| model_op remark_o 
	{
		operand_ptr op;
		if($model_op.chain_opt)
			op = std::make_unique<model_operand>(std::move(*$model_op.chain_opt),provider.get_range( $model_op.ctx)); 
		else
			op = std::make_unique<semantics::empty_operand>(provider.get_range( $model_op.ctx)); 
		$line.operands.push_back(std::move(op));
		$line.remarks = $remark_o.value ? remark_list{*$remark_o.value} : remark_list{};
	} EOLLN EOF
	| EOLLN EOF;

//////////////////////////////////////// dat_r

op_rem_body_dat_r returns [op_rem line]
	: op_list_dat remark_o 
	{
		$line.operands = std::move($op_list_dat.operands);
		$line.remarks = $remark_o.value ? remark_list{*$remark_o.value} : remark_list{};
	} EOLLN EOF
	| model_op remark_o 
	{
		operand_ptr op;
		if($model_op.chain_opt)
			op = std::make_unique<model_operand>(std::move(*$model_op.chain_opt),provider.get_range( $model_op.ctx)); 
		else
			op = std::make_unique<semantics::empty_operand>(provider.get_range( $model_op.ctx)); 
		$line.operands.push_back(std::move(op));
		$line.remarks = $remark_o.value ? remark_list{*$remark_o.value} : remark_list{};
	} EOLLN EOF
	| EOLLN EOF;

//////////////////////////////////////// asm_r

op_rem_body_asm_r returns [op_rem line]
	: op_list_asm remark_o 
	{
		$line.operands = std::move($op_list_asm.operands);
		$line.remarks = $remark_o.value ? remark_list{*$remark_o.value} : remark_list{};
	} EOLLN EOF
	| model_op remark_o 
	{
		operand_ptr op;
		if($model_op.chain_opt)
			op = std::make_unique<model_operand>(std::move(*$model_op.chain_opt),provider.get_range( $model_op.ctx)); 
		else
			op = std::make_unique<semantics::empty_operand>(provider.get_range( $model_op.ctx)); 
		$line.operands.push_back(std::move(op));
		$line.remarks = $remark_o.value ? remark_list{*$remark_o.value} : remark_list{};
	} EOLLN EOF
	| EOLLN EOF;

//////////////////////////////////////// mac_r

op_rem_body_mac_r returns [op_rem line]
	: op_rem_body_alt_mac
	{
		parse_macro_operands($op_rem_body_alt_mac.line);
		$line = std::move($op_rem_body_alt_mac.line);
	} EOLLN EOF
	| remark_o 
	{
		$line.remarks = $remark_o.value ? remark_list{*$remark_o.value} : remark_list{};
	} EOLLN EOF
	| EOLLN EOF;

op_rem_body_noop_r
	: remark_o EOLLN EOF;