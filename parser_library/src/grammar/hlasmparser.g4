parser grammar hlasmparser;

@header
{
	#include "../parser_impl.h"

	namespace hlasm_plugin::parser_library::generated
	{
		using namespace hlasm_plugin::parser_library;
		using namespace hlasm_plugin::parser_library::semantics;
		using namespace hlasm_plugin::parser_library::context;
	}

	/* disables unreferenced parameter (_localctx) warning */
	#ifdef _MSC_VER
		#pragma warning(push)
		#pragma warning(disable: 4100)
	#endif
}

@footer 
{
	#ifdef _MSC_VER
		#pragma warning(pop)
	#endif
}

options {
    tokenVocab = lex;
	superClass = parser_impl;
}

@members 
{
	#define last_text() _input->LT(-1)->getText()
	#define text(token) token->getText()

	using expr_ptr = std::unique_ptr<hlasm_plugin::parser_library::semantics::expression>;
	using expression = hlasm_plugin::parser_library::semantics::expression;
	using arith_expr = hlasm_plugin::parser_library::semantics::arithmetic_expression;
	using char_expr = hlasm_plugin::parser_library::semantics::character_expression;
	using keyword_expr = hlasm_plugin::parser_library::semantics::keyword_expression;

	
}

program : /* ictl? process_instruction* */ program_block  EOF;
/************************************************************************************************************************************************************************
ictl: { _input->LT(2)->getText() == "ICTL" }? SPACE ORDSYMBOL SPACE ictl_begin EOLLN{ lexer->set_ictl(); };

ictl_begin: IDENTIFIER ictl_end? 
				{ 
					size_t idx = 0;
					auto val = std::stoi($IDENTIFIER.text, &idx);
					if(idx > 0 || !lexer->set_begin(val))
						throw RecognitionException("invalid ICTL parameter value", this, _input, _localctx, $IDENTIFIER); 
				}
			;

ictl_end: COMMA IDENTIFIER ictl_continue 
				{ 
					size_t idx = 0;
					auto val = std::stoi($IDENTIFIER.text, &idx);
					if(idx > 0 || !lexer->set_end(val))
						throw RecognitionException("invalid ICTL parameter value", this, _input, _localctx, $IDENTIFIER); 
				}
			;

ictl_continue:  { lexer->set_continuation_enabled(false); }

                | COMMA IDENTIFIER 
					{ 
						size_t idx = 0;
						auto val = std::stoi($IDENTIFIER.text, &idx);
						if(idx > 0 || !lexer->set_continue(val))
							throw RecognitionException("invalid ICTL parameter value", this, _input, _ctx, $IDENTIFIER); 
					}
				;

process_instruction: PROCESS SPACE (assembler_options
									| (ORDSYMBOL { $ORDSYMBOL.text == "OVERRIDE" }? LPAR assembler_options RPAR)) EOLLN;


assembler_options: assembler_option (COMMA assembler_option)*;

assembler_option: identifier (LPAR (list | codepage | machine | identifier)? RPAR)?;

list: list_item (COMMA list_item)*;

list_item: identifier (LPAR (identifier | string)+? RPAR)?;

machine: identifier (MINUS identifier)? (COMMA identifier)?;

codepage: identifier VERTICAL identifier string;

identifier: VERTICAL; //just something

****************************************************************************************************************************************************/









program_block: (instruction_statement EOLLN)*;

instruction_statement
	: { analyzer.in_lookahead()}? lookahead_instruction_statement										{analyzer.process_statement();}
	| {!analyzer.in_lookahead()}? ordinary_instruction_statement										{analyzer.process_statement();}
	| SPACE*																							{analyzer.process_statement();};


ordinary_instruction_statement: label SPACE instruction operands_and_remarks;


//lookahead rules*************************************************************************************************************************************

lookahead_instruction_statement
	: look_statement												{analyzer.set_statement_field(symbol_range::get_range($look_statement.ctx));}	//nothing interesting
	| look_label_o SPACE ORDSYMBOL SPACE*							{analyzer.set_instruction_field($ORDSYMBOL->getText());}						//macro mend
	| look_label_o SPACE ORDSYMBOL SPACE+ word (SPACE ~EOLLN*)?																						//copy, macro mend
	{
		analyzer.set_instruction_field($ORDSYMBOL->getText());
		analyzer.set_operand_remark_field($word.ctx->getText());
	}
	| seq_symbol ~EOLLN*											{analyzer.set_label_field(std::move($seq_symbol.ss));};			//seq

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

//lookahead rules*************************************************************************************************************************************



//label rules************************************************************

label
	: l_char_string											{analyzer.set_label_field(std::move($l_char_string.value),$l_char_string.ctx);}		
	| l_char_string_sp l_char_string_o						
	{
		$l_char_string_sp.value.append(std::move($l_char_string_o.value));
		analyzer.set_label_field(std::move($l_char_string_sp.value));
	}																				
	| l_model												{analyzer.set_label_field(std::move($l_model.conc_list));}	//model stmt rule with no space
    | l_model_sp											{analyzer.set_label_field(std::move($l_model_sp.conc_list));}	//model stmt rule with possible space
	|														{analyzer.set_label_field();};

//label rules************************************************************

//label subrules*********************************************************

l_common_rules returns [std::vector<concat_point_ptr> conc_list]
	: l_string_v_apo_sp l_string_v
	{
		$conc_list = std::move($l_string_v_apo_sp.conc_list);
		$conc_list.insert($conc_list.end(), std::make_move_iterator($l_string_v.conc_list.begin()), std::make_move_iterator($l_string_v.conc_list.end()));
	}
	| l_string_poss_space_c l_string_v
	{
		$conc_list.push_back(std::make_unique<char_str>(std::move($l_string_poss_space_c.value)));
		$conc_list.insert($conc_list.end(), std::make_move_iterator($l_string_v.conc_list.begin()), std::make_move_iterator($l_string_v.conc_list.end()));
	}
	| l_string_v_apo_sp l_string
	{
		$l_string_v_apo_sp.conc_list.push_back(std::make_unique<char_str>(std::move($l_string.value)));
		$conc_list = std::move($l_string_v_apo_sp.conc_list);
	};

l_model_sp returns [std::vector<concat_point_ptr> conc_list]
	: l_string_v_apo_sp											{$conc_list = std::move($l_string_v_apo_sp.conc_list);}
	| l_common_rules											{$conc_list = std::move($l_common_rules.conc_list);}
	| l_common_rules l_string_v_apo
	{
		$conc_list = std::move($l_common_rules.conc_list);
		$conc_list.insert($conc_list.end(), std::make_move_iterator($l_string_v_apo.conc_list.begin()), std::make_move_iterator($l_string_v_apo.conc_list.end()));
	}
	| l_common_rules l_string_no_space_c
	{
		$conc_list = std::move($l_common_rules.conc_list);
		$conc_list.push_back(std::make_unique<char_str>(std::move($l_string_no_space_c.value)));
	}
	| l_string_poss_space_c l_string l_string_v_apo
	{
		$conc_list.push_back(std::make_unique<char_str>(std::move($l_string_poss_space_c.value)));
		$conc_list.push_back(std::make_unique<char_str>(std::move($l_string.value)));
		$conc_list.insert($conc_list.end(), std::make_move_iterator($l_string_v_apo.conc_list.begin()), std::make_move_iterator($l_string_v_apo.conc_list.end()));
	};

l_model returns [std::vector<concat_point_ptr> conc_list]
	: l_string_v			
	{
		$conc_list = std::move($l_string_v.conc_list);
	}
	| l_string l_string_v_apo
	{
		$conc_list.push_back(std::make_unique<char_str>(std::move($l_string.value)));
		$conc_list.insert($conc_list.end(), std::make_move_iterator($l_string_v_apo.conc_list.begin()), std::make_move_iterator($l_string_v_apo.conc_list.end()));
	}
	| l_string_v l_string_v_apo
	{
		$conc_list = std::move($l_string_v.conc_list);
		$conc_list.insert($conc_list.end(), std::make_move_iterator($l_string_v_apo.conc_list.begin()), std::make_move_iterator($l_string_v_apo.conc_list.end()));
	}
	| l_string_v l_string_no_space_c
	{
		$conc_list = std::move($l_string_v.conc_list);
		$conc_list.push_back(std::make_unique<char_str>(std::move($l_string_no_space_c.value)));
	};



common_ch returns [std::string value]
	: ASTERISK												{$value = "*";}
	| MINUS													{$value = "-";}
	| PLUS													{$value = "+";}
	| LT													{$value = "<";}
	| GT													{$value = ">";}
	| SLASH													{$value = "/";}
	| EQUALS												{$value = "=";}
	| AMPERSAND AMPERSAND									{$value = "&&";}
	| VERTICAL												{$value = "|";}
	| IDENTIFIER											{$value = $IDENTIFIER->getText();}
	| ORDSYMBOL												{$value = $ORDSYMBOL->getText();}
	| DOT													{$value = ".";};

l_ch returns [std::string value]
	: common_ch												{$value = std::move($common_ch.value);}
	| COMMA													{$value = ",";}
	| LPAR													{$value = "(";}
	| RPAR													{$value = ")";};

common_ch_v returns [concat_point_ptr point]
	: ASTERISK												{$point = std::make_unique<char_str>("*");}
	| MINUS													{$point = std::make_unique<char_str>("-");}
	| PLUS													{$point = std::make_unique<char_str>("+");}
	| LT													{$point = std::make_unique<char_str>("<");}
	| GT													{$point = std::make_unique<char_str>(">");}
	| SLASH													{$point = std::make_unique<char_str>("/");}
	| EQUALS												{$point = std::make_unique<char_str>("=");}
	| AMPERSAND AMPERSAND									{$point = std::make_unique<char_str>("&&");}
	| VERTICAL												{$point = std::make_unique<char_str>("|");}
	| IDENTIFIER											{$point = std::make_unique<char_str>(std::move($IDENTIFIER->getText()));}
	| ORDSYMBOL												{$point = std::make_unique<char_str>(std::move($ORDSYMBOL->getText()));}
	| DOT													{$point = std::make_unique<dot>();}												
	| var_symbol											{$point = std::make_unique<var_sym>(std::move($var_symbol.vs));};

l_ch_v returns [concat_point_ptr point]
	: common_ch_v											{$point = std::move($common_ch_v.point);}
	| COMMA													{$point = std::make_unique<char_str>(",");}
	| LPAR													{$point = std::make_unique<char_str>("(");}
	| RPAR													{$point = std::make_unique<char_str>(")");};

l_str_v returns [std::vector<concat_point_ptr> conc_list]
	:														
	| tmp=l_str_v l_ch_v									{$conc_list=std::move($tmp.conc_list);};

l_string returns [std::string value]
	: l_ch													{$value = std::move($l_ch.value);}
	| str=l_string l_ch										{$value.append(std::move($str.value)); $value.append(std::move($l_ch.value));};

l_string_v returns [std::vector<concat_point_ptr> conc_list]
	: l_string_o var_symbol l_str_v							
	{
		$conc_list.push_back(std::make_unique<char_str>(std::move($l_string_o.value))); 
		$conc_list.push_back(std::make_unique<var_sym>(std::move($var_symbol.vs)));
		$conc_list.insert($conc_list.end(), std::make_move_iterator($l_str_v.conc_list.begin()), std::make_move_iterator($l_str_v.conc_list.end()));
	};

l_string_o returns [std::string value]
	: l_string												{$value = std::move($l_string.value);}				
	| ;

l_string_no_space_c returns [std::string value]
	: APOSTROPHE str1=l_string_o APOSTROPHE str2=l_string_o			
	{
		$value.append("'"); $value.append(std::move($str1.value)); $value.append("'"); $value.append(std::move($str2.value));
	}
	| tmp=l_string_no_space_c APOSTROPHE str1=l_string_o APOSTROPHE str2=l_string_o
	{
		$value = std::move($tmp.value); $value.append("'"); $value.append(std::move($str1.value)); $value.append("'"); $value.append(std::move($str2.value));
	};

l_string_no_space_v returns [std::vector<concat_point_ptr> conc_list]
	: APOSTROPHE l_string_o APOSTROPHE l_string_v
	{
		std::string tmp("'"); tmp.append(std::move($l_string_o.value)); tmp.append("'");
		$conc_list.push_back(std::make_unique<char_str>(std::move(tmp)));
		$conc_list.insert($conc_list.end(), std::make_move_iterator($l_string_v.conc_list.begin()), std::make_move_iterator($l_string_v.conc_list.end()));
	}
	| APOSTROPHE l_string_v APOSTROPHE l_string_o
	{
		$conc_list.push_back(std::make_unique<char_str>("'"));
		$conc_list.insert($conc_list.end(), std::make_move_iterator($l_string_v.conc_list.begin()), std::make_move_iterator($l_string_v.conc_list.end()));
		$conc_list.push_back(std::make_unique<char_str>("'"));
		$conc_list.push_back(std::make_unique<char_str>(std::move($l_string_o.value)));
	}
	| APOSTROPHE str1=l_string_v APOSTROPHE str2=l_string_v
	{
		$conc_list.push_back(std::make_unique<char_str>("'"));
		$conc_list.insert($conc_list.end(), std::make_move_iterator($str1.conc_list.begin()), std::make_move_iterator($str1.conc_list.end()));
		$conc_list.push_back(std::make_unique<char_str>("'"));
		$conc_list.insert($conc_list.end(), std::make_move_iterator($str2.conc_list.begin()), std::make_move_iterator($str2.conc_list.end()));
	};

l_string_no_space_u returns [std::vector<concat_point_ptr> conc_list]
	: l_string_no_space_v													{$conc_list = std::move($l_string_no_space_v.conc_list);}
	| APOSTROPHE str1=l_string_o APOSTROPHE str2=l_string_o
	{
		std::string tmp("'"); tmp.append(std::move($str1.value)); tmp.append("'");  tmp.append(std::move($str2.value));
		$conc_list.push_back(std::make_unique<char_str>(std::move(tmp)));
	};

l_string_no_space_u_c returns [std::vector<concat_point_ptr> conc_list]
	:  
	| cl1=l_string_no_space_u_c cl2=l_string_no_space_u
	{
		$conc_list = std::move($cl1.conc_list);
		$conc_list.insert($conc_list.end(), std::make_move_iterator($cl2.conc_list.begin()), std::make_move_iterator($cl2.conc_list.end()));	
	};

l_string_no_space_c_o returns [std::string value]
	: l_string_no_space_c														{$value = std::move($l_string_no_space_c.value);}
	| ;

l_string_v_apo returns [std::vector<concat_point_ptr> conc_list]
	: cl1=l_string_no_space_c_o  cl2=l_string_no_space_v cl3=l_string_no_space_u_c
	{
		$conc_list.push_back(std::make_unique<char_str>(std::move($cl1.value)));
		$conc_list.insert($conc_list.end(), std::make_move_iterator($cl2.conc_list.begin()), std::make_move_iterator($cl2.conc_list.end()));	
		$conc_list.insert($conc_list.end(), std::make_move_iterator($cl3.conc_list.begin()), std::make_move_iterator($cl3.conc_list.end()));
	};




l_sp_ch returns [std::string value] //l_ch with SPACE
	: l_ch															{$value = std::move($l_ch.value);}
	| SPACE															{$value = " ";}; 					
l_sp_ch_v returns [concat_point_ptr point]
	: l_sp_ch														{$point = std::make_unique<char_str>(std::move($l_sp_ch.value));}
	| var_symbol													{$point = std::make_unique<var_sym>(std::move($var_symbol.vs));};

l_sp_str_v returns [std::vector<concat_point_ptr> conc_list]
	:		
	| tmp=l_sp_str_v l_sp_ch_v											{$conc_list = std::move($tmp.conc_list); $conc_list.push_back(std::move($l_sp_ch_v.point)); };

l_sp_string returns [std::string value]
	: 
	| tmp=l_sp_string l_sp_ch											{$value=std::move($tmp.value); $value.append(std::move($l_sp_ch.value));};

l_sp_string_v returns [std::vector<concat_point_ptr> conc_list]
	: l_sp_string var_symbol l_sp_str_v
	{
		$conc_list.push_back(std::make_unique<char_str>(std::move($l_sp_string.value))); 
		$conc_list.push_back(std::make_unique<var_sym>(std::move($var_symbol.vs)));
		$conc_list.insert($conc_list.end(), std::make_move_iterator($l_sp_str_v.conc_list.begin()), std::make_move_iterator($l_sp_str_v.conc_list.end()));
	};


l_string_poss_space_c returns [std::string value]
	: APOSTROPHE l_sp_string APOSTROPHE										{$value.append("'"); $value.append(std::move($l_sp_string.value)); $value.append("'");}
	| tmp=l_string_poss_space_c APOSTROPHE l_sp_string APOSTROPHE			{$value=std::move($tmp.value); $value.append("'"); $value.append(std::move($l_sp_string.value)); $value.append("'");};

l_string_poss_space_c_o returns [std::string value]
	: l_string_poss_space_c													{$value = std::move($l_string_poss_space_c.value);}
	| ;

l_string_poss_space_u returns [std::vector<concat_point_ptr> conc_list]
	: APOSTROPHE l_sp_string APOSTROPHE										
	{
		std::string tmp("'"); tmp.append(std::move($l_sp_string.value)); tmp.append("'"); 
		$conc_list.push_back(std::make_unique<char_str>(std::move(tmp)));
	}
	| APOSTROPHE l_sp_string_v APOSTROPHE
	{
		$conc_list.push_back(std::make_unique<char_str>("'"));
		$conc_list.insert($conc_list.end(), std::make_move_iterator($l_sp_string_v.conc_list.begin()), std::make_move_iterator($l_sp_string_v.conc_list.end()));
		$conc_list.push_back(std::make_unique<char_str>("'"));
	};

l_string_poss_space_u_c returns [std::vector<concat_point_ptr> conc_list]
	: tmp=l_string_poss_space_u_c l_string_poss_space_u							
	{
		$conc_list = std::move($tmp.conc_list); 
		$conc_list.insert($conc_list.end(), std::make_move_iterator($l_string_poss_space_u.conc_list.begin()), std::make_move_iterator($l_string_poss_space_u.conc_list.end()));
	}
	| ;

l_string_v_apo_sp returns [std::vector<concat_point_ptr> conc_list]
	: cl1=l_string_poss_space_c_o APOSTROPHE cl2=l_sp_string_v APOSTROPHE cl3=l_string_poss_space_u_c
	{
		$cl1.value.append("'");
		$cl2.conc_list.push_back(std::make_unique<char_str>("'"));
		$conc_list.push_back(std::make_unique<char_str>(std::move($cl1.value)));
		$conc_list.insert($conc_list.end(), std::make_move_iterator($cl2.conc_list.begin()), std::make_move_iterator($cl2.conc_list.end()));
		$conc_list.insert($conc_list.end(), std::make_move_iterator($cl3.conc_list.begin()), std::make_move_iterator($cl3.conc_list.end()));
	};





l_a_ch returns [std::string value]		//l_ch with apo
	: l_ch										{$value = std::move($l_ch.value);}
	| APOSTROPHE								{$value = "'";};


l_a_string returns [std::string value]
	: tmp=l_a_string l_a_ch							{$value = std::move($tmp.value); $value.append($l_a_ch.value);}
	| ;

l_char_string returns [std::string value]		//does not begin with apo
	: l_ch l_a_string								{$value = std::move($l_ch.value); $value.append(std::move($l_a_string.value));};

l_char_string_o returns [std::string value]
	: l_char_string									{$value = std::move($l_char_string.value);}
	| ;


l_char_string_sp returns [std::string value]
	: APOSTROPHE l_sp_string APOSTROPHE							{$value = "'"; $value.append(std::move($l_sp_string.value)); $value.append("'");}
	| tmp=l_char_string_sp APOSTROPHE l_sp_string APOSTROPHE	{$value = std::move($tmp.value); $value.append("'"); $value.append(std::move($l_sp_string.value)); $value.append("'");};

//label subrules*********************************************************





//instruction rules******************************************************

instruction returns [id_index instr]
	: ORDSYMBOL													{analyzer.set_instruction_field(std::move($ORDSYMBOL->getText()));}
	| macro_name												{analyzer.set_instruction_field(std::move($macro_name.value));}
	| l_string_v			/*model*/							{analyzer.set_instruction_field(std::move($l_string_v.conc_list));};

//instruction rules******************************************************

//instruction subrules***************************************************

macro_name_b returns [std::string value]
	: l_ch													{$value = std::move($l_ch.value);}
	| tmp=macro_name_b l_ch									{$value = std::move($tmp.value); $value.append(std::move($l_ch.value));};

macro_name returns [std::string value]
	: ORDSYMBOL macro_name_b								{$value = std::move($ORDSYMBOL->getText()); $value.append(std::move($macro_name_b.value));};

//instruction subrules***************************************************






remark_ch: DOT|ASTERISK|MINUS|PLUS|LT|GT|COMMA|LPAR|RPAR|SLASH|EQUALS|AMPERSAND|APOSTROPHE|IDENTIFIER|VERTICAL|ORDSYMBOL|SPACE;

remark
	: remark_ch*;

remark_o returns [symbol_guard<symbol_range> range_g]
	: SPACE remark							{$range_g.value = symbol_range::get_range($remark.ctx); $range_g.valid = true;}
	|										{$range_g.valid = false;};


operands_model returns [op_rem line]													//rule for variable substitution
	: op_rem_body				{$line = std::move($op_rem_body.line);};

operands_and_remarks
	: { analyzer.current_instruction().has_no_ops }? SPACE+ remark								/*noop instr*/
	{
		analyzer.set_operand_remark_field({},{symbol_range::get_range($remark.ctx)});
	}
	| {!analyzer.current_instruction().has_no_ops }? SPACE+ op_rem_body
	{
		analyzer.set_operand_remark_field(std::move($op_rem_body.line.operands),std::move($op_rem_body.line.remarks));
	}
	|														{analyzer.set_operand_remark_field({},{});};

op_rem_body returns [op_rem line]
	: {!analyzer.current_instruction().has_alt_format}? op_list_comma_o operand remark_o
	{
		$op_list_comma_o.operands.push_back(std::move($operand.op));
		$line.operands = std::move($op_list_comma_o.operands);
		if($remark_o.range_g) $line.remarks.push_back($remark_o.range_g.value);
	}
	| { analyzer.current_instruction().has_alt_format}? op_rem_body_alt
	{
		$line = std::move($op_rem_body_alt.line);
	}
	| ;

op_rem_body_alt returns [op_rem line]
	: op_list_comma_o operand_not_empty remark_o				
	{
		$op_list_comma_o.operands.push_back(std::move($operand_not_empty.op)); 
		$line.operands = std::move($op_list_comma_o.operands); 
		if($remark_o.range_g) $line.remarks.push_back($remark_o.range_g.value);
	}
	| op_list_comma cont												
	{
		$line.operands = std::move($op_list_comma.operands);
		$line.operands.insert($line.operands.end(), std::make_move_iterator($cont.line.operands.begin()), std::make_move_iterator($cont.line.operands.end()));
		$line.remarks = std::move($cont.line.remarks);
	};


cont returns [op_rem line]
	: {enable_continuation();} cont_body {disable_continuation();}						{$line = std::move($cont_body.line);};

cont_body returns [op_rem line]
	: remark_o																					
	{ 
		if($remark_o.range_g) $line.remarks.push_back($remark_o.range_g.value);
		auto tmp = std::make_unique<empty_operand>(); tmp->range = symbol_range::get_empty_range($remark_o.ctx->getStart());
		$line.operands.push_back(std::move(tmp));
	}
	| SPACE r1=remark CONTINUATION /*empty op*/ sp=SPACE {disable_continuation();} r2=remark				
	{
		$line.remarks.push_back(symbol_range::get_range($r1.ctx)); 
		auto tmp = std::make_unique<empty_operand>(); tmp->range = symbol_range::get_empty_range($sp);
		$line.operands.push_back(std::move(tmp));
		$line.remarks.push_back(symbol_range::get_range($r2.ctx));
	}
	| SPACE remark CONTINUATION {disable_continuation();} next=op_rem_body_alt
	{
		$line.remarks.push_back(symbol_range::get_range($remark.ctx));
		$line.remarks.insert($line.remarks.end(),std::make_move_iterator($next.line.remarks.begin()),std::make_move_iterator($next.line.remarks.end()));
		$line.operands = std::move($next.line.operands);
	};

op_list_comma_o returns [std::vector<operand_ptr> operands]
	: op_list_comma												{$operands = std::move($op_list_comma.operands);}
	| ;

op_list_comma returns [std::vector<operand_ptr> operands]
	: operand COMMA												{$operands.push_back(std::move($operand.op));}
	| tmp=op_list_comma operand COMMA							{$tmp.operands.push_back(std::move($operand.op)); $operands = std::move($tmp.operands);};

operand returns [operand_ptr op]
	: operand_not_empty					{$op = std::move($operand_not_empty.op);}
	|									{$op = std::make_unique<empty_operand>(); $op->range = symbol_range::get_empty_range(_localctx->getStart());};


operand_not_empty returns [operand_ptr op]
	: {analyzer.current_instruction().type != instruction_type::CA && analyzer.current_instruction().type != instruction_type::MAC}? op_string_v		
	{
		$op = std::make_unique<model_operand>(std::move($op_string_v.conc_list)); $op->range = symbol_range::get_range($op_string_v.ctx);
	}
	| {analyzer.current_instruction().type == instruction_type::MACH}? mach_op																	
	{
		$op = std::make_unique<machine_operand>([=](){return $mach_op.ctx->getText();}); $op->range = symbol_range::get_range($mach_op.ctx);
	}
	| {analyzer.current_instruction().type == instruction_type::ASM}? asm_op																	
	{
		$op = std::make_unique<empty_operand>(); $op->range = symbol_range::get_range($asm_op.ctx);
	}
	| {analyzer.current_instruction().type == instruction_type::DAT}? data_def																	
	{
		$op = std::make_unique<empty_operand>(); $op->range = symbol_range::get_range($data_def.ctx);
	}
	| {analyzer.current_instruction().type == instruction_type::CA}? ca_op																		
	{
		$op = std::move($ca_op.op); $op->range = symbol_range::get_range($ca_op.ctx);
	}
	| {analyzer.current_instruction().type == instruction_type::MAC}? mac_op																	
	{
		$op = std::make_unique<empty_operand>(); $op->range = symbol_range::get_range($mac_op.ctx);
	}
	| {analyzer.current_instruction().type == instruction_type::MACH || analyzer.current_instruction().type == instruction_type::ASM}? mach_expr		
	{
		$op = std::make_unique<machine_operand>([=](){return $mach_expr.ctx->getText();}); $op->range = symbol_range::get_range($mach_expr.ctx);
	};


mach_op: mach_expr_p LPAR mach_expr RPAR 
	| mach_expr_p LPAR mach_expr COMMA mach_expr RPAR		
	| mach_expr_p LPAR COMMA mach_expr RPAR;

asm_op: string
	| id LPAR id_comma_c RPAR;

ca_op returns [operand_ptr op]
	: LPAR expr RPAR seq_symbol								{$op = std::make_unique<ca_operand>(std::move($seq_symbol.ss),std::move($expr.e));}
	| seq_symbol											{$op = std::make_unique<ca_operand>(std::move($seq_symbol.ss));}
	| expr_p
	{
		if($expr_p.vs_g) 
		{
			auto tmp = std::make_unique<ca_operand>(std::move($expr_p.vs_g.value)); tmp->expression =  std::move($expr_p.e);
			$op = std::move(tmp);
		}
		else $op = std::make_unique<ca_operand>(std::move($expr_p.e));
	};

mac_op:  mac_entry;


mach_expr: mach_expr_p
	| EQUALS data_def;



mach_expr_p: mach_expr_s
	| PLUS mach_expr_s
	| MINUS mach_expr_s;

mach_expr_s: mach_expr_s PLUS mach_term_c
	| mach_expr_s MINUS mach_term_c
	| mach_term_c;

mach_term_c: mach_term
	| mach_term_c SLASH mach_term
	| mach_term_c ASTERISK mach_term;

mach_term: LPAR mach_expr RPAR
	| ASTERISK
	| {!is_self_def()}? data_attribute
	| id
	| { is_self_def()}? ORDSYMBOL string;



expr returns [expr_ptr e]
	: SPACE expr_p_space_c								{ $e = expression::evaluate(std::move($expr_p_space_c.exprs)); }
	| expr_p_space_c									{ $e = expression::evaluate(std::move($expr_p_space_c.exprs)); };

expr_p returns [expr_ptr e, symbol_guard<var_sym> vs_g]
	: expr_s											{ $e = std::move($expr_s.e); $vs_g = std::move($expr_s.vs_g);}
	| PLUS expr_p										{ $e = +*$expr_p.e; $vs_g.valid = false;}
	| MINUS expr_p										{ $e = -*$expr_p.e; $vs_g.valid = false;};

expr_s returns [expr_ptr e, symbol_guard<var_sym> vs_g]
	: tmp=expr_s PLUS term_c							{ $e = *$tmp.e + *$term_c.e; $vs_g.valid = false;}
	| tmp=expr_s MINUS term_c							{ $e = *$tmp.e - *$term_c.e; $vs_g.valid = false;}
	| term_c											{ $e = std::move($term_c.e); $vs_g = std::move($term_c.vs_g);};

term_c returns [expr_ptr e, symbol_guard<var_sym> vs_g]
	: term												{ $e = std::move($term.e); $vs_g = std::move($term.vs_g);}
	| tmp=term_c SLASH term								{ $e = *$tmp.e / *$term.e; $vs_g.valid = false;}
	| tmp=term_c ASTERISK term							{ $e = *$tmp.e * *$term.e; $vs_g.valid = false;};

term returns [expr_ptr e, symbol_guard<var_sym> vs_g]
	: LPAR expr RPAR									{ $e = std::move($expr.e); }
	| var_symbol										{$vs_g.value = std::move($var_symbol.vs); $vs_g.valid =true;}
	| ca_string											{ $e = std::move($ca_string.e); }
	| {!is_self_def()}? data_attribute					{ $e = std::move($data_attribute.e); }
	| { is_self_def()}? ORDSYMBOL string				{ $e = expression::self_defining_term($ORDSYMBOL->getText(), $string.value, false);  /*TODO:DBCS*/}
	| id subscript										
	{ 
		if(!$subscript.exprs_g) $e = expression::resolve_ord_symbol($id.name); /*TODO:DBCS*/ 
		else 
		{
			if($subscript.exprs_g.value.size()==1) $e = $subscript.exprs_g.value[0]->unary_operation($id.name); 
			else if($subscript.exprs_g.value.size()==2) $e = $subscript.exprs_g.value[0]->binary_operation($id.name, $subscript.exprs_g.value[1]);
		}
		$vs_g.value = var_sym(std::move($id.name),std::move($subscript.exprs_g.value),symbol_range::get_range($id.ctx->getStart(),$subscript.ctx->getStop())); $vs_g.valid = true;
	};

expr_p_comma_c returns [std::vector<expr_ptr> exprs]
	: expr_p											{ $exprs.push_back(std::move($expr_p.e)); }
	| exs=expr_p_comma_c COMMA expr_p					{ $exs.exprs.push_back(std::move($expr_p.e)); $exprs = std::move($exs.exprs); };
	
expr_p_space_c returns [std::deque<expr_ptr> exprs]
	: expr_p											{ $exprs.push_back(std::move($expr_p.e)); }
	| exs=expr_p_space_c SPACE expr_p					{ $exs.exprs.push_back(std::move($expr_p.e)); $exprs = std::move($exs.exprs); }
	;


id_ch returns [std::string value]
	: IDENTIFIER								{$value = std::move($IDENTIFIER->getText());} 
	| ORDSYMBOL									{$value = std::move($ORDSYMBOL->getText());};

id_ch_c returns [std::string value]
	: id_ch										{$value = std::move($id_ch.value);}
	| tmp=id_ch_c id_ch							{$tmp.value.append(std::move($id_ch.value));  $value = std::move($tmp.value);};


id returns [std::string name, std::string using_qualifier]
	: id_no_dot									{$name = std::move($id_no_dot.value);}
	| id_no_dot DOT id_no_dot					{$name = std::move($id_no_dot.value); $using_qualifier = std::move($id_no_dot.value);};

id_no_dot returns [std::string value]
	: id_ch_c									{$value = std::move($id_ch_c.value);};


opt_dot returns [std::string value]
	: DOT id_ch_c								{$value = std::move($id_ch_c.value);}
	| ;

id_comma_c: id
	|  id COMMA id_comma_c;


op_ch returns [std::string value]
	: common_ch								{$value = std::move($common_ch.value);}
	| LPAR									{$value = "(";}
	| RPAR									{$value = ")";};

op_ch_c returns [std::string value]
	:
	| tmp=op_ch_c op_ch							{$value = std::move($tmp.value); $value.append($op_ch.value);};

op_ch_v returns [concat_point_ptr point]
	: common_ch_v							{$point = std::move($common_ch_v.point);}
	| LPAR									{$point = std::make_unique<char_str>("(");}
	| RPAR									{$point = std::make_unique<char_str>(")");};

op_ch_v_c returns [std::vector<concat_point_ptr> conc_list]
	:
	| tmp=op_ch_v_c op_ch_v						{$tmp.conc_list.push_back(std::move($op_ch_v.point)); $conc_list = std::move($tmp.conc_list);};

op_string_v returns [std::vector<concat_point_ptr> conc_list]
	: op_ch_c var_symbol cl=op_ch_v_c
	{
		$conc_list.push_back(std::make_unique<char_str>(std::move($op_ch_c.value)));
		$conc_list.push_back(std::make_unique<var_sym>(std::move($var_symbol.vs)));
		$conc_list.insert($conc_list.end(), std::make_move_iterator($op_ch_v_c.conc_list.begin()), std::make_move_iterator($op_ch_v_c.conc_list.end()));
	};


data_attribute returns [expr_ptr e]
	: ORDSYMBOL APOSTROPHE EQUALS data_def				{ $e = std::make_unique<arith_expr>(0); /* TODO */ }
	| ORDSYMBOL APOSTROPHE string						{ $e = std::make_unique<arith_expr>(0); /* TODO */ }
	| ORDSYMBOL APOSTROPHE var_symbol					{ $e = std::make_unique<arith_expr>(0); /* TODO */ }
	| ORDSYMBOL APOSTROPHE id							{ $e = std::make_unique<arith_expr>(0); /* TODO */ }
	;

seq_symbol returns [seq_sym ss]
	: DOT id									{$ss.name = std::move($id.name);$ss.loc.line = $DOT->getLine(); $ss.loc.offset = $DOT->getStartIndex();};



subscript returns [symbol_guard<std::vector<expr_ptr>> exprs_g]
	: LPAR expr_p_comma_c RPAR								{$exprs_g.valid=true; $exprs_g.value = std::move($expr_p_comma_c.exprs);}
	|														{$exprs_g.valid=false;};



created_set_body returns [concat_point_ptr point]
	: ORDSYMBOL												{$point = std::make_unique<char_str>(std::move($ORDSYMBOL->getText()));}
	| IDENTIFIER											{$point = std::make_unique<char_str>(std::move($IDENTIFIER->getText()));}
	| var_symbol_p											{$point = std::make_unique<var_sym>(std::move($var_symbol_p.vs));}
	| DOT													{$point = std::make_unique<dot>();};

created_set_body_c returns [std::vector<concat_point_ptr> concat_list]
	: cl=created_set_body													{$concat_list.push_back(std::move($cl.point));}
	| clc=created_set_body_c cl=created_set_body							{$clc.concat_list.push_back(std::move($cl.point)); $concat_list =std::move($clc.concat_list);};

created_set_symbol returns [var_sym vs]
	: AMPERSAND LPAR clc=created_set_body_c RPAR subscript				
	{
		$vs.name = analyzer.concatenate(std::move($clc.concat_list));
		$vs.subscript = std::move($subscript.exprs_g.value);
		$vs.range = symbol_range::get_range($AMPERSAND,$subscript.ctx->getStop());
	};			

var_symbol_p returns [var_sym vs]
	: AMPERSAND id_no_dot tmp=subscript							
	{
		auto id = std::move($id_no_dot.value); 
		auto r = symbol_range::get_range($AMPERSAND,$tmp.ctx->getStop()); 
		$vs = std::move(var_sym(id, std::move($tmp.exprs_g.value), r));
	};

var_symbol returns [var_sym vs]
	: var_symbol_p 													{$vs = std::move($var_symbol_p.vs); }
	| created_set_symbol 											{$vs = std::move($created_set_symbol.vs); };




dupl_factor: LPAR mach_expr RPAR			//absolute
	|;

modifer: id_no_dot
	   | ;

prog_type_and_modifier: LPAR  ORDSYMBOL string RPAR modifer		// self def term - dec/char/hex/bin ORDSYMBOL + modifier
		| LPAR id_no_dot RPAR modifer;									// symbol + modifier

nominal_value: string 
	| LPAR mach_expr_comma_c RPAR;

nominal_value_o: nominal_value
	|;

mach_expr_comma_c: mach_expr
	| mach_expr_comma_c COMMA mach_expr;


data_def: dupl_factor id_no_dot prog_type_and_modifier nominal_value
	| dupl_factor id_no_dot nominal_value_o;




mac_str_ch returns [concat_point_ptr point]
	: common_ch_v									{$point = std::move($common_ch_v.point);}
	| SPACE											{$point = std::make_unique<char_str>(" ");};

mac_str_b returns [std::vector<concat_point_ptr> conc_list]
	:
	| tmp=mac_str_b mac_str_ch						{$tmp.conc_list.push_back(std::move($mac_str_ch.point)); $conc_list = std::move($tmp.conc_list);};

mac_str returns [std::vector<concat_point_ptr> conc_list]
	: APOSTROPHE mac_str_b APOSTROPHE				
	{
		$conc_list.push_back(std::make_unique<char_str>("'"));
		$conc_list.insert($conc_list.end(), std::make_move_iterator($mac_str_b.conc_list.begin()), std::make_move_iterator($mac_str_b.conc_list.end()));
		$conc_list.push_back(std::make_unique<char_str>("'"));
	};

mac_ch returns [std::vector<concat_point_ptr> conc_list]
	: common_ch_v									{$conc_list.push_back(std::move($common_ch_v.point));}
	| mac_str										{$conc_list = std::move($mac_str.conc_list);}
	| mac_sublist									{$conc_list.push_back(std::move($mac_sublist.point));};

mac_ch_c returns [std::vector<concat_point_ptr> conc_list]
	:
	| tmp=mac_ch_c mac_ch							
	{
		$conc_list = std::move($tmp.conc_list);
		$conc_list.insert($conc_list.end(), std::make_move_iterator($mac_ch.conc_list.begin()), std::make_move_iterator($mac_ch.conc_list.end()));
	};

mac_entry returns [std::vector<concat_point_ptr> conc_list]
	: mac_ch										{$conc_list = std::move($mac_ch.conc_list);}
	| tmp=mac_entry mac_ch							
	{
		$conc_list = std::move($tmp.conc_list);
		$conc_list.insert($conc_list.end(), std::make_move_iterator($mac_ch.conc_list.begin()), std::make_move_iterator($mac_ch.conc_list.end()));
	};

mac_sublist_b returns [std::vector<concat_point_ptr> conc_list]
	: mac_ch_c										{$conc_list = std::move($mac_ch_c.conc_list);}
	| tmp=mac_sublist_b COMMA mac_ch_c			
	{
		$conc_list = std::move($tmp.conc_list);
		$conc_list.insert($conc_list.end(), std::make_move_iterator($mac_ch_c.conc_list.begin()), std::make_move_iterator($mac_ch_c.conc_list.end()));
	};

mac_sublist returns [concat_point_ptr point]
	: LPAR mac_sublist_b RPAR						{$point = std::make_unique<sublist>(std::move($mac_sublist_b.conc_list));};


//not adn not and

string_ch returns [std::string value]
	: l_sp_ch							{$value = std::move($l_sp_ch.value);}
	| APOSTROPHE APOSTROPHE				{$value = "'";};

string_ch_c returns [std::string value]
	:
	| tmp=string_ch_c string_ch				{$value = std::move($tmp.value); $value.append($string_ch.value);};

string_ch_v returns [concat_point_ptr point]
	: l_sp_ch_v							{$point = std::move($l_sp_ch_v.point);}
	| APOSTROPHE APOSTROPHE				{$point = std::make_unique<char_str>("'");};

string_ch_v_c returns [std::vector<concat_point_ptr> conc_list]
	:
	| cl=string_ch_v_c string_ch_v		{$cl.conc_list.push_back(std::move($string_ch_v.point)); $conc_list = std::move($cl.conc_list);};



string returns [std::string value]
	: APOSTROPHE string_ch_c APOSTROPHE					{$value = std::move($string_ch_c.value);};


ca_dupl_factor returns [int32_t df]
	: LPAR expr_p RPAR																{ $df = $expr_p.e->get_numeric_value(); }			
	|																				{ $df=1; };

substring returns [expr_ptr start, expr_ptr end]
	: LPAR e1=expr_p COMMA e2=expr_p RPAR											{ $start = std::move($e1.e); $end = std::move($e2.e); }
	| LPAR expr_p COMMA ASTERISK RPAR
	| LPAR expr_p RPAR																{ $start = std::move($expr_p.e); }			
	| ;

ca_string_b returns [std::unique_ptr<char_expr> e]
	: ca_dupl_factor APOSTROPHE string_ch_v_c APOSTROPHE substring
	{ 
		auto tmp = analyzer.concatenate(std::move($string_ch_v_c.conc_list));
		auto ex = std::make_unique<char_expr>(std::move(tmp));
		$e = ex->substring($ca_dupl_factor.df, $substring.start, $substring.end);
	};

ca_string returns [std::unique_ptr<char_expr> e]
	: ca_string_b												{$e = std::move($ca_string_b.e);}
	| tmp=ca_string DOT ca_string_b								{$e = $tmp.e->append(std::move($ca_string_b.e));};








expr_statement returns [std::string test_str]
	: expr
	{ 
		$test_str.append($expr.e->get_str_val());
	}
	| tmp=expr_statement EOLLN expr
	{ 
		$tmp.test_str.append("\n");
		$tmp.test_str.append($expr.e->get_str_val());
		$test_str = std::move($tmp.test_str);
	};

expr_test returns [std::string test_str]
	:  expr_statement EOLLN EOF					{$test_str = std::move($expr_statement.test_str);};
