parser grammar lookahead_rules; 

lookahead_instruction_statement
	: lookahead_label SPACE lookahead_instruction lookahead_operands_and_remarks		//ord
	| seq_symbol ~EOLLN*																//seq										
	{
		collector.set_label_field(std::move($seq_symbol.ss),provider.get_range($seq_symbol.ctx));
		collector.set_instruction_field(provider.get_range($seq_symbol.ctx));
		collector.set_operand_remark_field(provider.get_range($seq_symbol.ctx));
		process_instruction();
	}
	| look_statement																	//nothing interesting										
	{
		collector.set_instruction_field(provider.get_range($look_statement.ctx));
		collector.set_operand_remark_field(provider.get_range($look_statement.ctx));
		process_instruction();
	};	


lookahead_operands_and_remarks
	: {!ignored()}? SPACE+ lookahead_operand_list (COMMA lookahead_ignored_part)?
	{
		collector.set_operand_remark_field(
			std::move($lookahead_operand_list.operands),
			{},
			provider.get_range($lookahead_operand_list.ctx));
	}
	| lookahead_ignored_part
	{
		collector.set_operand_remark_field(
			operand_list{},
			remark_list{},
			provider.get_range($lookahead_ignored_part.ctx));
	};

lookahead_operand_list returns [operand_list operands]
	: operand											{$operands.push_back(std::move($operand.op));}
	| tmp=lookahead_operand_list COMMA operand			{$tmp.operands.push_back(std::move($operand.op)); $operands = std::move($tmp.operands);};

lookahead_ignored_part
	: ~EOLLN*;


lookahead_instruction
	: ORDSYMBOL
	{
		collector.set_instruction_field(
			parse_identifier(std::move($ORDSYMBOL->getText()),provider.get_range($ORDSYMBOL)),
			provider.get_range($ORDSYMBOL));

		process_instruction();
	};

lookahead_label:
	ORDSYMBOL
	{
		auto r = provider.get_range($ORDSYMBOL);
		collector.set_label_field(parse_identifier($ORDSYMBOL->getText(),r),nullptr,r); 
	}
	| empty
	{
		collector.set_label_field(provider.get_range($empty.ctx)); 
	};
	
empty:;

look_statement
	: ~EOLLN*;

look_label
	: ~(SPACE|EOLLN) ~(SPACE|EOLLN) ~(SPACE|EOLLN)+
	| DOT ~(ORDSYMBOL|SPACE|EOLLN)
	| ~(DOT|EOLLN|SPACE) ~(SPACE|EOLLN)
	| ~(SPACE|EOLLN|ORDSYMBOL);

look_label_o
	: look_label
	| ;
