parser grammar instruction_field_rules;

instruction returns [id_index instr]
	: l_string_v			/*model*/							
	{
		collector.set_instruction_field(std::move($l_string_v.chain),provider.get_range( $l_string_v.ctx));
		process_instruction();
	}
	| macro_name												
	{
		collector.set_instruction_field(
			parse_identifier(std::move($macro_name.value),provider.get_range($macro_name.ctx)),
			provider.get_range( $macro_name.ctx));
		process_instruction();
	}
	| ORDSYMBOL													
	{
		collector.add_lsp_symbol({$ORDSYMBOL->getText(),provider.get_range( $ORDSYMBOL),symbol_type::instruction});
		collector.set_instruction_field(
			parse_identifier(std::move($ORDSYMBOL->getText()),provider.get_range($ORDSYMBOL)),
			provider.get_range($ORDSYMBOL));
		process_instruction();
	}
	| bad_instr
	{
		collector.add_lsp_symbol({$bad_instr.ctx->getText(),provider.get_range( $bad_instr.ctx),symbol_type::instruction});
		collector.set_instruction_field(
			ctx->ids().add($bad_instr.ctx->getText()),
			provider.get_range($bad_instr.ctx));
		process_instruction();
	};

macro_name_b returns [std::string value]
	: l_ch													{$value = std::move($l_ch.value);}
	| tmp=macro_name_b l_ch									{$value = std::move($tmp.value); $value.append(std::move($l_ch.value));};

macro_name returns [std::string value]
	: ORDSYMBOL macro_name_b								{$value = std::move($ORDSYMBOL->getText()); $value.append(std::move($macro_name_b.value));};

bad_instr
	: IDENTIFIER ~(SPACE|EOLLN)*;