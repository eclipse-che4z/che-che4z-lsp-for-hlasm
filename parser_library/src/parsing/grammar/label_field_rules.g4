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

 //rules for label field
parser grammar label_field_rules;

label
	: l_char_string
	{
		range r = provider.get_range( $l_char_string.ctx);
		auto& label = $l_char_string.value;
		if (label.size() > 1 && label[0] == '.')
		{
			collector.add_hl_symbol(token_info(r,hl_scopes::seq_symbol));
			label.erase(0, 1);
			auto id = parse_identifier(std::move(label),r);

			collector.set_label_field({id,r},r);
		}
		else
		{
			if (!label.empty() && label[0] != '&')
				collector.add_hl_symbol(token_info(r,hl_scopes::label));
			auto id = add_id(label);
			collector.set_label_field(id,std::move(label),$l_char_string.ctx,r);
		}
	}
	| l_char_string_sp l_char_string_o
	{
		collector.add_hl_symbol(token_info(provider.get_range( $l_char_string_sp.ctx),hl_scopes::label));
		collector.add_hl_symbol(token_info(provider.get_range( $l_char_string_o.ctx),hl_scopes::label));
		$l_char_string_sp.value.append(std::move($l_char_string_o.value));
		auto r = provider.get_range( $l_char_string_sp.ctx->getStart(),$l_char_string_o.ctx->getStop());
		collector.set_label_field(std::move($l_char_string_sp.value),r);
	}
	| l_model
	{
		collector.set_label_field(std::move($l_model.chain),provider.get_range( $l_model.ctx));
	}	//model stmt rule with no space
	| l_model_sp
	{
		collector.set_label_field(std::move($l_model_sp.chain),provider.get_range( $l_model_sp.ctx));
	}	//model stmt rule with possible space
	|
	{
		collector.set_label_field(provider.get_empty_range( _localctx->getStart()));
	};


l_common_rules [concat_chain* chain]
	: l_string_v_apo_sp[$chain] l_string_v[$chain]
	| l_string_poss_space_c
	{
		$chain->emplace_back(char_str_conc(std::move($l_string_poss_space_c.value), provider.get_range($l_string_poss_space_c.ctx)));
	}
	l_string_v[$chain]
	| l_string_v_apo_sp[$chain] l_string
	{
		$chain->emplace_back(char_str_conc(std::move($l_string.value), provider.get_range($l_string.ctx)));
	};

l_model_sp returns [concat_chain chain]
	: l_string_v_apo_sp[&$chain]
	| l_common_rules[&$chain]
	| l_common_rules[&$chain] l_string_v_apo[&$chain]
	| l_common_rules[&$chain] l_string_no_space_c
	{
		$chain.emplace_back(char_str_conc(std::move($l_string_no_space_c.value), provider.get_range($l_string_no_space_c.ctx)));
	}
	| l_string_poss_space_c l_string
	{
		$chain.emplace_back(char_str_conc(std::move($l_string_poss_space_c.value), provider.get_range($l_string_poss_space_c.ctx)));
		$chain.emplace_back(char_str_conc(std::move($l_string.value), provider.get_range($l_string.ctx)));
	}
	l_string_v_apo[&$chain]
	;
	finally
	{concatenation_point::clear_concat_chain($chain);}

l_model returns [concat_chain chain]
	: l_string_v[&$chain]
	| l_string
	{
		$chain.emplace_back(char_str_conc(std::move($l_string.value), provider.get_range($l_string.ctx)));
	}
	l_string_v_apo[&$chain]
	| l_string_v[&$chain] l_string_v_apo[&$chain]
	| l_string_v[&$chain] l_string_no_space_c
	{
		$chain.emplace_back(char_str_conc(std::move($l_string_no_space_c.value), provider.get_range($l_string_no_space_c.ctx)));
	};
	finally
	{concatenation_point::clear_concat_chain($chain);}



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
	| NUM													{$value = $NUM->getText();}
	| ORDSYMBOL												{$value = $ORDSYMBOL->getText();}
	| DOT													{$value = ".";};

l_ch returns [std::string value]
	: common_ch												{$value = std::move($common_ch.value);}
	| COMMA													{$value = ",";}
	| LPAR													{$value = "(";}
	| RPAR													{$value = ")";};

common_ch_v [concat_chain* chain]
	: ASTERISK												{$chain->emplace_back(char_str_conc("*", provider.get_range($ASTERISK)));}
	| MINUS													{$chain->emplace_back(char_str_conc("-", provider.get_range($MINUS)));}
	| PLUS													{$chain->emplace_back(char_str_conc("+", provider.get_range($PLUS)));}
	| LT													{$chain->emplace_back(char_str_conc("<", provider.get_range($LT)));}
	| GT													{$chain->emplace_back(char_str_conc(">", provider.get_range($GT)));}
	| SLASH													{$chain->emplace_back(char_str_conc("/", provider.get_range($SLASH)));}
	| EQUALS												{$chain->emplace_back(equals_conc());}
	| VERTICAL												{$chain->emplace_back(char_str_conc("|", provider.get_range($VERTICAL)));}
	| IDENTIFIER											{$chain->emplace_back(char_str_conc($IDENTIFIER->getText(), provider.get_range($IDENTIFIER)));}
	| NUM													{$chain->emplace_back(char_str_conc($NUM->getText(), provider.get_range($NUM)));}
	| ORDSYMBOL												{$chain->emplace_back(char_str_conc($ORDSYMBOL->getText(), provider.get_range($ORDSYMBOL)));}
	| DOT													{$chain->emplace_back(dot_conc());}
	|
	(
		l=AMPERSAND r=AMPERSAND								{$chain->emplace_back(char_str_conc("&&", provider.get_range($l,$r)));}
		|
		var_symbol											{$chain->emplace_back(var_sym_conc(std::move($var_symbol.vs)));}
	)
	;

l_ch_v [concat_chain* chain]
	: common_ch_v[$chain]
	| COMMA													{$chain->emplace_back(char_str_conc(",", provider.get_range($COMMA)));}
	| LPAR													{$chain->emplace_back(char_str_conc("(", provider.get_range($LPAR)));}
	| RPAR													{$chain->emplace_back(char_str_conc(")", provider.get_range($RPAR)));};

l_string returns [std::string value]
	: l_ch													{$value = std::move($l_ch.value);}
	| str=l_string l_ch										{$value.append(std::move($str.value)); $value.append(std::move($l_ch.value));};

l_string_v [concat_chain* chain]
	: l_string_o var_symbol
	{
		$chain->emplace_back(char_str_conc(std::move($l_string_o.value), provider.get_range($l_string_o.ctx)));
		$chain->emplace_back(var_sym_conc(std::move($var_symbol.vs)));
	}
	(
		l_ch_v[$chain]
	)*
	;
	finally
	{concatenation_point::clear_concat_chain(*$chain);}

l_string_o returns [std::string value]
	: l_string												{$value = std::move($l_string.value);}				
	| ;

l_string_no_space_c returns [std::string value]
	: l_apo str1=l_string_o l_apo str2=l_string_o			
	{
		$value.append("'"); $value.append(std::move($str1.value)); $value.append("'"); $value.append(std::move($str2.value));
	}
	| tmp=l_string_no_space_c l_apo str1=l_string_o l_apo str2=l_string_o
	{
		$value = std::move($tmp.value); $value.append("'"); $value.append(std::move($str1.value)); $value.append("'"); $value.append(std::move($str2.value));
	};

l_string_no_space_v [concat_chain* chain]
	: l=l_apo l_string_o r=l_apo
	{
		std::string tmp("'"); tmp.append(std::move($l_string_o.value)); tmp.append("'");
		$chain->emplace_back(char_str_conc(std::move(tmp), provider.get_range($l.ctx->getStart(), $r.ctx->getStart())));
	}
	l_string_v[$chain]
	| l=l_apo
	{
		$chain->emplace_back(char_str_conc("'", provider.get_range($l.ctx->getStart())));
	}
	l_string_v[$chain] r=l_apo l_string_o
	{
		$chain->emplace_back(char_str_conc("'", provider.get_range($r.ctx->getStart())));
		$chain->emplace_back(char_str_conc(std::move($l_string_o.value), provider.get_range($l_string_o.ctx)));
	}
	| l=l_apo
	{
		$chain->emplace_back(char_str_conc("'", provider.get_range($l.ctx->getStart())));
	}
	str1=l_string_v[$chain] r=l_apo
	{
		$chain->emplace_back(char_str_conc("'", provider.get_range($r.ctx->getStart())));
	}
	str2=l_string_v[$chain]
	;

l_string_no_space_u [concat_chain* chain]
	: l_string_no_space_v[$chain]
	| l=l_apo str1=l_string_o r=l_apo str2=l_string_o
	{
		std::string tmp("'"); tmp.append(std::move($str1.value)); tmp.append("'");  tmp.append(std::move($str2.value));
		$chain->emplace_back(char_str_conc(std::move(tmp), provider.get_range($l.ctx->getStart(), $r.ctx->getStart())));
	};

l_string_no_space_u_c [concat_chain* chain]
	: (l_string_no_space_u[$chain])*
	;

l_string_no_space_c_o returns [std::string value]
	: l_string_no_space_c														{$value = std::move($l_string_no_space_c.value);}
	| ;

l_string_v_apo [concat_chain* chain]
	: cl1=l_string_no_space_c_o
	{
		$chain->emplace_back(char_str_conc(std::move($cl1.value), provider.get_range($l_string_no_space_c_o.ctx)));
	}
	cl2=l_string_no_space_v[$chain] cl3=l_string_no_space_u_c[$chain]
	;




l_sp_ch returns [std::string value] //l_ch with SPACE
	: l_ch															{$value = std::move($l_ch.value);}
	| SPACE															{$value = $SPACE->getText();}; 					
l_sp_ch_v [concat_chain* chain]
	: l_ch_v[$chain]
	| SPACE															{$chain->emplace_back(char_str_conc($SPACE->getText(), provider.get_range($SPACE)));};

l_sp_str_v [concat_chain* chain]
	:
	(
		l_sp_ch_v[$chain]
	)*
	;

l_sp_string returns [std::string value]
	: 
	| tmp=l_sp_string l_sp_ch											{$value=std::move($tmp.value); $value.append(std::move($l_sp_ch.value));};

l_sp_string_v [concat_chain* chain]
	: l_sp_string var_symbol
	{
		$chain->emplace_back(char_str_conc(std::move($l_sp_string.value), provider.get_range($l_sp_string.ctx)));
		$chain->emplace_back(var_sym_conc(std::move($var_symbol.vs)));
	}
	l_sp_str_v[$chain]
	;


l_string_poss_space_c returns [std::string value]
	: l_apo l_sp_string l_apo										{$value.append("'"); $value.append(std::move($l_sp_string.value)); $value.append("'");}
	| tmp=l_string_poss_space_c l_apo l_sp_string l_apo			{$value=std::move($tmp.value); $value.append("'"); $value.append(std::move($l_sp_string.value)); $value.append("'");};

l_string_poss_space_c_o returns [std::string value]
	: l_string_poss_space_c													{$value = std::move($l_string_poss_space_c.value);}
	| ;

l_string_poss_space_u [concat_chain* chain]
	: l=l_apo l_sp_string r=l_apo										
	{
		std::string tmp("'"); tmp.append(std::move($l_sp_string.value)); tmp.append("'"); 
		$chain->emplace_back(char_str_conc(std::move(tmp), provider.get_range($l.ctx->getStart(), $r.ctx->getStart())));
	}
	| l=l_apo
	{
		$chain->emplace_back(char_str_conc("'", provider.get_range($l.ctx->getStart())));
	}
	l_sp_string_v[$chain] r=l_apo
	{
		$chain->emplace_back(char_str_conc("'", provider.get_range($r.ctx->getStart())));
	};

l_string_poss_space_u_c [concat_chain* chain]
	: (l_string_poss_space_u[$chain])*
	;

l_string_v_apo_sp [concat_chain* chain]
	: cl1=l_string_poss_space_c_o l=l_apo
	{
		$cl1.value.append("'");
		$chain->emplace_back(char_str_conc(std::move($cl1.value), provider.get_range($l_string_poss_space_c_o.ctx->getStart(), $l.ctx->getStart())));
	}
	cl2=l_sp_string_v[$chain] r=l_apo
	{
		$chain->emplace_back(char_str_conc("'", provider.get_range($r.ctx->getStart())));
	}
	cl3=l_string_poss_space_u_c[$chain]
	;





l_a_ch returns [std::string value]		//l_ch with apo
	: l_ch										{$value = std::move($l_ch.value);}
	| l_apo								{$value = "'";};


l_a_string returns [std::string value]
	: tmp=l_a_string l_a_ch							{$value = std::move($tmp.value); $value.append($l_a_ch.value);}
	| ;

l_char_string returns [std::string value]		//does not begin with apo
	: l_ch l_a_string								{$value = std::move($l_ch.value); $value.append(std::move($l_a_string.value));};

l_char_string_o returns [std::string value]
	: l_char_string									{$value = std::move($l_char_string.value);}
	| ;


l_char_string_sp returns [std::string value]
	: l_apo l_sp_string l_apo							{$value = "'"; $value.append(std::move($l_sp_string.value)); $value.append("'");}
	| tmp=l_char_string_sp l_apo l_sp_string l_apo	{$value = std::move($tmp.value); $value.append("'"); $value.append(std::move($l_sp_string.value)); $value.append("'");};


l_apo
	: APOSTROPHE
	| ATTR;

