parser grammar operand_field_rules;

model_operands returns [op_rem line]													//rule for variable substitution
	: op_rem_body				{$line = std::move($op_rem_body.line);};

operands_and_remarks
	: { deferred() }? SPACE deferred_op_rem					                 		//TODO make special grammar for defered, for now mac_entry
	{
		auto r = provider.get_range( $deferred_op_rem.ctx);
		collector.set_operand_remark_field(std::move($deferred_op_rem.ctx->getText()),r);
	} 
	| {!deferred()}? operands_and_remarks_nd
	|
	{
		collector.set_operand_remark_field(provider.get_empty_range( _localctx->getStart()));
	};

operands_and_remarks_nd
	: { no_op() ||  UNKNOWN() }? SPACE+ remark								/*noop instr*/
	{
		collector.set_operand_remark_field(std::vector<operand_ptr>(),{provider.get_range( $remark.ctx)}, provider.get_range( $remark.ctx));
	}
	| {!no_op() && !UNKNOWN() }? SPACE+ op_rem_body
	{
		collector.set_operand_remark_field(std::move($op_rem_body.line.operands),std::move($op_rem_body.line.remarks), provider.get_range( $op_rem_body.ctx));
	};

/*
deferred_entry 
	: op_ch_v_c
	| AMPERSAND SPACE; 

deferred_op_rem returns [concat_chain chain] 
	: deferred_entry		{$chain = std::move($deferred_entry.chain);}
	| deferred_entry comma ch=deferred_op_rem
	{
		$chain = std::move($deferred_entry.chain);
		$chain.push_back(std::make_unique<char_str>(","));
		$chain.insert($chain.end(), std::make_move_iterator($ch.chain.begin()), std::make_move_iterator($ch.chain.end()));
	};
*/

deferred_entry 
	: op_ch_v
	| AMPERSAND SPACE; 

deferred_op_rem
	: deferred_entry deferred_op_rem
	| ;

op_rem_body returns [op_rem line]
	: {!alt_format()}? op_list_comma remark_o
	{
		$line.operands = std::move($op_list_comma.operands);
		$line.operands.push_back(std::make_unique<semantics::empty_operand>(provider.get_range($op_list_comma.ctx)));
		if($remark_o.range_opt) $line.remarks.push_back(*$remark_o.range_opt);
	}
	| {!alt_format()}? op_list_comma_o operand_not_empty remark_o
	{
		$op_list_comma_o.operands.push_back(std::move($operand_not_empty.op));
		$line.operands = std::move($op_list_comma_o.operands);
		if($remark_o.range_opt) $line.remarks.push_back(*$remark_o.range_opt);
	}
	| {!alt_format()}? model_op
	{
		auto op = std::make_unique<model_operand>(std::move($model_op.chain),provider.get_range( $model_op.ctx)); 
		$line.operands.push_back(std::move(op));
	}
	| { alt_format()}? op_rem_body_alt
	{
		$line = std::move($op_rem_body_alt.line);
	}
	| ;

op_rem_body_alt returns [op_rem line]
	: alt_op_list_comma_o alt_operand_not_empty remark_o				
	{
		$alt_op_list_comma_o.operands.push_back(std::move($alt_operand_not_empty.op)); 
		$line.operands = std::move($alt_op_list_comma_o.operands); 
		if($remark_o.range_opt) $line.remarks.push_back(*$remark_o.range_opt);
	}
	| alt_op_list_comma cont												
	{
		$line.operands = std::move($alt_op_list_comma.operands);
		$line.operands.insert($line.operands.end(), std::make_move_iterator($cont.line.operands.begin()), std::make_move_iterator($cont.line.operands.end()));
		$line.remarks = std::move($cont.line.remarks);
	};


cont returns [op_rem line]
	: {enable_continuation();} cont_body {disable_continuation();}						{$line = std::move($cont_body.line);};

cont_body returns [op_rem line]
	: remark_o																					
	{ 
		if($remark_o.range_opt) $line.remarks.push_back(*$remark_o.range_opt);
		auto tmp = std::make_unique<semantics::empty_operand>(provider.get_empty_range( $remark_o.ctx->getStart()));
		$line.operands.push_back(std::move(tmp));
	}
	| SPACE r1=remark CONTINUATION /*empty op*/ sp=SPACE {disable_continuation();} r2=remark				
	{
		$line.remarks.push_back(provider.get_range( $r1.ctx)); 
		auto tmp = std::make_unique<semantics::empty_operand>(provider.get_empty_range( $sp));
		$line.operands.push_back(std::move(tmp));
		$line.remarks.push_back(provider.get_range( $r2.ctx));
	}
	| SPACE remark CONTINUATION {disable_continuation();} next=op_rem_body_alt
	{
		$line.remarks.push_back(provider.get_range( $remark.ctx));
		$line.remarks.insert($line.remarks.end(),std::make_move_iterator($next.line.remarks.begin()),std::make_move_iterator($next.line.remarks.end()));
		$line.operands = std::move($next.line.operands);
	};

alt_op_list_comma_o returns [std::vector<operand_ptr> operands]
	: alt_op_list_comma													{$operands = std::move($alt_op_list_comma.operands);}
	| ;

alt_op_list_comma returns [std::vector<operand_ptr> operands]
	: alt_operand comma													{$operands.push_back(std::move($alt_operand.op)); }
	| tmp=alt_op_list_comma alt_operand comma							{$tmp.operands.push_back(std::move($alt_operand.op)); $operands = std::move($tmp.operands); };

alt_operand returns [operand_ptr op]
	: alt_operand_not_empty					{$op = std::move($alt_operand_not_empty.op);}
	|										{$op = std::make_unique<semantics::empty_operand>(provider.get_empty_range( _localctx->getStart()));};


alt_operand_not_empty returns [operand_ptr op]
	: {CA() && alt_format()}? ca_op																		
	{
		$op = std::move($ca_op.op);
	}
	| {MAC() && alt_format()}? mac_op																	
	{
		$op = std::move($mac_op.op); 
	};

op_list_comma_o returns [std::vector<operand_ptr> operands]
	: op_list_comma												{$operands = std::move($op_list_comma.operands);}
	| ;

op_list_comma returns [std::vector<operand_ptr> operands]
	: operand comma												{$operands.push_back(std::move($operand.op)); }
	| tmp=op_list_comma operand comma							{$tmp.operands.push_back(std::move($operand.op)); $operands = std::move($tmp.operands); };

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
