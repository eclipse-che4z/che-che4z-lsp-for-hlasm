parser grammar lookahead_rules; 

lookahead_instruction_statement
	: look_statement	//nothing interesting										
	{
		collector.set_label_field(provider.get_range($look_statement.ctx));
		collector.set_instruction_field(provider.get_range($look_statement.ctx));
		collector.set_operand_remark_field(provider.get_range($look_statement.ctx));
		process_instruction();
	}	
	| look_label_o SPACE ORDSYMBOL SPACE*	//macro mend						
	{
		collector.set_label_field(provider.get_range($look_label_o.ctx));
		collector.set_instruction_field(
			parse_identifier(std::move($ORDSYMBOL->getText()),provider.get_range( $ORDSYMBOL)),
			provider.get_range( $ORDSYMBOL));
		collector.set_operand_remark_field(provider.get_range($ORDSYMBOL));
		process_instruction();
	}
	| look_label_o SPACE ORDSYMBOL SPACE+ word (SPACE ~EOLLN*)?		//copy, macro mend
	{
		collector.set_label_field(provider.get_range($look_label_o.ctx));
		collector.set_instruction_field(
			parse_identifier(std::move($ORDSYMBOL->getText()),provider.get_range( $ORDSYMBOL)),
			provider.get_range( $ORDSYMBOL));
		operand_list ops;
		auto text = $word.ctx->getText();
		auto r = provider.get_range( $word.ctx);
		auto id = parse_identifier(text,r);
		ops.push_back(
			std::make_unique<expr_assembler_operand>(
				std::make_unique<mach_expr_symbol>(id,r),text,r
			)
		);
		collector.set_operand_remark_field(std::move(ops),{},r); 
		process_instruction();
	}
	| seq_symbol ~EOLLN*	//seq										
	{
		collector.set_label_field(std::move($seq_symbol.ss),provider.get_range($seq_symbol.ctx));
		collector.set_instruction_field(provider.get_range($seq_symbol.ctx));
		collector.set_operand_remark_field(provider.get_range($seq_symbol.ctx));
		process_instruction();
	};	

look_statement
	: look_label SPACE*
	| look_label_o SPACE look_instr (SPACE (~EOLLN)*)?;

look_label
	: ~(SPACE|EOLLN) ~(SPACE|EOLLN) ~(SPACE|EOLLN)+
	| DOT ~(ORDSYMBOL|SPACE|EOLLN)
	| ~(DOT|EOLLN|SPACE) ~(SPACE|EOLLN)
	| ~(SPACE|EOLLN);

look_label_o
	: look_label
	|;

look_instr
	:  ~(SPACE|EOLLN) (~(SPACE|EOLLN))+
	| ~(ORDSYMBOL|SPACE|EOLLN);

word: (~(SPACE|EOLLN))+;
