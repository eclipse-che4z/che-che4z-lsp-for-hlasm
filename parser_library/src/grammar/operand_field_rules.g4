parser grammar operand_field_rules;

model_operands returns [op_rem line]													//rule for variable substitution
	: op_rem_body				{$line = std::move($op_rem_body.line);};

operands_and_remarks
	: { ignored()}? (~EOLLN)*
	| {!ignored()}? operands_and_remarks_ni;

operands_and_remarks_ni
	: { deferred()}? SPACE {enable_hidden();} deferred_op_rem					                 		
	{
		auto r = provider.get_range( $deferred_op_rem.ctx);
		collector.set_operand_remark_field(std::move($deferred_op_rem.ctx->getText()),std::move($deferred_op_rem.remarks),r);
		disable_hidden();
	} 
	| {!deferred()}? operands_and_remarks_nd
	|
	{
		collector.set_operand_remark_field(provider.get_empty_range( _localctx->getStart()));
	};

operands_and_remarks_nd
	: { no_op() ||  UNKNOWN() }? SPACE* remark_o								/*noop instr*/
	{
		collector.set_operand_remark_field(operand_list{},$remark_o.value ? remark_list{*$remark_o.value} : remark_list{}, provider.get_range( $remark_o.ctx));
	}
	| {!no_op() && !UNKNOWN() }? SPACE+ op_rem_body
	{
		collector.set_operand_remark_field(std::move($op_rem_body.line.operands),std::move($op_rem_body.line.remarks), provider.get_range( $op_rem_body.ctx));
	};



op_rem_body returns [op_rem line]
	: {!alt_format()}? op_list_comma remark_o
	{
		$line.operands = std::move($op_list_comma.operands);
		$line.operands.push_back(std::make_unique<semantics::empty_operand>(provider.get_range($op_list_comma.ctx->getStop())));
		$line.remarks = $remark_o.value ? remark_list{*$remark_o.value} : remark_list{};
	}
	| {!alt_format()}? op_list_comma_o last_operand_not_empty remark_o
	{
		$op_list_comma_o.operands.push_back(std::move($last_operand_not_empty.op));
		$line.operands = std::move($op_list_comma_o.operands);
		$line.remarks = $remark_o.value ? remark_list{*$remark_o.value} : remark_list{};
	}
	| {!alt_format()}? model_op remark_o
	{
		operand_ptr op;
		if($model_op.chain_opt)
			op = std::make_unique<model_operand>(std::move(*$model_op.chain_opt),provider.get_range( $model_op.ctx)); 
		else
			op = std::make_unique<semantics::empty_operand>(provider.get_range( $model_op.ctx)); 
		$line.operands.push_back(std::move(op));
		$line.remarks = $remark_o.value ? remark_list{*$remark_o.value} : remark_list{};
	}
	| { alt_format()}? op_rem_body_alt
	{
		$line = std::move($op_rem_body_alt.line);
	}
	| remark_o
	{
		$line.remarks = $remark_o.value ? remark_list{*$remark_o.value} : remark_list{};
	};

op_rem_body_alt returns [op_rem line]
	: alt_op_list_comma_o alt_operand_not_empty remark_o
	{
		$alt_op_list_comma_o.operands.push_back(std::move($alt_operand_not_empty.op)); 
		$line.operands = std::move($alt_op_list_comma_o.operands); 
		$line.remarks = $remark_o.value ? remark_list{*$remark_o.value} : remark_list{};
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
	| remark_o CONTINUATION {disable_continuation();} next=op_rem_body_alt
	{
		$line.remarks = $remark_o.value ? remark_list{*$remark_o.value} : remark_list{};
		$line.remarks.insert($line.remarks.end(),std::make_move_iterator($next.line.remarks.begin()),std::make_move_iterator($next.line.remarks.end()));
		$line.operands = std::move($next.line.operands);
	};

alt_op_list_comma_o returns [std::vector<operand_ptr> operands]
	:
	| alt_op_list_comma													{$operands = std::move($alt_op_list_comma.operands);};

alt_op_list_comma returns [std::vector<operand_ptr> operands]
	: alt_operand comma													{$operands.push_back(std::move($alt_operand.op)); }
	| tmp=alt_op_list_comma alt_operand comma							{$tmp.operands.push_back(std::move($alt_operand.op)); $operands = std::move($tmp.operands); };

alt_operand returns [operand_ptr op]
	: alt_operand_not_empty					{$op = std::move($alt_operand_not_empty.op);}
	|										{$op = std::make_unique<semantics::empty_operand>(provider.get_empty_range( _localctx->getStart()));};


alt_operand_not_empty returns [operand_ptr op]
	: {CA()}? ca_op																		
	{
		$op = std::move($ca_op.op);
	}
	| {MAC()}? mac_op																	
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

last_operand_not_empty returns [operand_ptr op]
	: {MACH()}? mach_op																	
	{
		$op = std::move($mach_op.op);
	}
	| {ASM()}? asm_op																	
	{
		$op = std::move($asm_op.op);
	}
	| {ASM()}? string (ORDSYMBOL|IDENTIFIER)
	{
		$op = std::make_unique<string_assembler_operand>(std::move($string.value),provider.get_range($string.ctx));
		auto diag = diagnostic_s{ diagnostic_op::warning_A300_op_apostrophes_missing("", provider.get_range($string.ctx)) };
		diag.file_name = ctx->opencode_file_name();
		add_diagnostic(diag);
	}
	| {DAT()}? data_def
	{
		$op = std::make_unique<data_def_operand>(std::move($data_def.value),provider.get_range($data_def.ctx));
	};
