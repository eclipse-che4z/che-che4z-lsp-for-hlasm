parser grammar assembler_operand_rules; 

asm_op returns [operand_ptr op]
	: id lpar asm_op_comma_c rpar
	{
		$op = std::make_unique<complex_assembler_operand>(
			*$id.name,std::move($asm_op_comma_c.asm_ops),
			provider.get_range($id.ctx->getStart(),$rpar.ctx->getStop())
			);
	}
	| lpar id1=end_instr_word comma id2=end_instr_word comma id3=end_instr_word rpar
	{
		std::vector<std::unique_ptr<complex_assembler_operand::component_value_t>> language_triplet;
		language_triplet.push_back(std::make_unique<complex_assembler_operand::string_value_t>(std::move($id1.value)));
		language_triplet.push_back(std::make_unique<complex_assembler_operand::string_value_t>(std::move($id2.value)));
		language_triplet.push_back(std::make_unique<complex_assembler_operand::string_value_t>(std::move($id3.value)));
		$op = std::make_unique<complex_assembler_operand>(
			"",std::move(language_triplet),
			provider.get_range($lpar.ctx->getStart(),$rpar.ctx->getStop())
		);
	}
	| lpar base=mach_expr comma end=mach_expr rpar
	{
		$op = std::make_unique<end_instr_assembler_operand>(
			std::move($base.m_e), 
			std::move($end.m_e),
			provider.get_range($lpar.ctx->getStart(),$rpar.ctx->getStop())
		);
	}
	| mach_expr
	{
		$op = std::make_unique<expr_assembler_operand>(std::move($mach_expr.m_e),$mach_expr.ctx->getText(),provider.get_range($mach_expr.ctx));
	}
	| string
	{
		$op = std::make_unique<string_assembler_operand>(std::move($string.value),provider.get_range($string.ctx));
	};

asm_op_inner returns [std::unique_ptr<complex_assembler_operand::component_value_t> op]
	: string														{ $op = std::make_unique<complex_assembler_operand::string_value_t>(std::move($string.value)); }	
	| id															{ $op = std::make_unique<complex_assembler_operand::string_value_t>(*$id.name); }
	| num															{ $op = std::make_unique<complex_assembler_operand::int_value_t>($num.value); }
	|																{ $op = std::make_unique<complex_assembler_operand::string_value_t>(""); };

asm_op_comma_c returns [std::vector<std::unique_ptr<complex_assembler_operand::component_value_t>> asm_ops]
	: asm_op_inner														{$asm_ops.push_back(std::move($asm_op_inner.op));}
	| tmp=asm_op_comma_c COMMA asm_op_inner								{$tmp.asm_ops.push_back(std::move($asm_op_inner.op)); $asm_ops = std::move($tmp.asm_ops);};	

end_instr_word returns [std::string value]
	: ORDSYMBOL															{$value = $ORDSYMBOL->getText();}
	| IDENTIFIER														{$value = $IDENTIFIER->getText();};