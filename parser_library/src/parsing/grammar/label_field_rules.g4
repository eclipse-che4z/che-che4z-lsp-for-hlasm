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
		auto label = $l_char_string.value;
		if (label.size()>1 && label[0] == '.')
		{
			collector.add_hl_symbol(token_info(r,hl_scopes::seq_symbol));
			auto id = parse_identifier(label.substr(1),r);
			collector.add_lsp_symbol(id,r,symbol_type::seq);

			collector.set_label_field({id,r},r);
		}
		else
		{
			collector.add_hl_symbol(token_info(r,hl_scopes::label));
			auto id = ctx->ids().add($l_char_string.value);
			collector.set_label_field(id,$l_char_string.ctx,r); 
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
		collector.add_hl_symbol(token_info(provider.get_range( $l_model.ctx),hl_scopes::label));
	}	//model stmt rule with no space
    | l_model_sp											
	{
		collector.set_label_field(std::move($l_model_sp.chain),provider.get_range( $l_model_sp.ctx));
		collector.add_hl_symbol(token_info(provider.get_range( $l_model_sp.ctx),hl_scopes::label));
	}	//model stmt rule with possible space
	|														
	{
		collector.set_label_field(provider.get_empty_range( _localctx->getStart()));
	};


l_common_rules returns [concat_chain chain]
	: l_string_v_apo_sp l_string_v
	{
		$chain = std::move($l_string_v_apo_sp.chain);
		$chain.insert($chain.end(), std::make_move_iterator($l_string_v.chain.begin()), std::make_move_iterator($l_string_v.chain.end()));
	}
	| l_string_poss_space_c l_string_v
	{
		$chain.push_back(std::make_unique<char_str_conc>(std::move($l_string_poss_space_c.value)));
		$chain.insert($chain.end(), std::make_move_iterator($l_string_v.chain.begin()), std::make_move_iterator($l_string_v.chain.end()));
	}
	| l_string_v_apo_sp l_string
	{
		$l_string_v_apo_sp.chain.push_back(std::make_unique<char_str_conc>(std::move($l_string.value)));
		$chain = std::move($l_string_v_apo_sp.chain);
	};

l_model_sp returns [concat_chain chain]
	: l_string_v_apo_sp											{$chain = std::move($l_string_v_apo_sp.chain);}
	| l_common_rules											{$chain = std::move($l_common_rules.chain);}
	| l_common_rules l_string_v_apo
	{
		$chain = std::move($l_common_rules.chain);
		$chain.insert($chain.end(), std::make_move_iterator($l_string_v_apo.chain.begin()), std::make_move_iterator($l_string_v_apo.chain.end()));
	}
	| l_common_rules l_string_no_space_c
	{
		$chain = std::move($l_common_rules.chain);
		$chain.push_back(std::make_unique<char_str_conc>(std::move($l_string_no_space_c.value)));
	}
	| l_string_poss_space_c l_string l_string_v_apo
	{
		$chain.push_back(std::make_unique<char_str_conc>(std::move($l_string_poss_space_c.value)));
		$chain.push_back(std::make_unique<char_str_conc>(std::move($l_string.value)));
		$chain.insert($chain.end(), std::make_move_iterator($l_string_v_apo.chain.begin()), std::make_move_iterator($l_string_v_apo.chain.end()));
	};
	finally
	{concatenation_point::clear_concat_chain($chain);}

l_model returns [concat_chain chain]
	: l_string_v			
	{
		$chain = std::move($l_string_v.chain);
	}
	| l_string l_string_v_apo
	{
		$chain.push_back(std::make_unique<char_str_conc>(std::move($l_string.value)));
		$chain.insert($chain.end(), std::make_move_iterator($l_string_v_apo.chain.begin()), std::make_move_iterator($l_string_v_apo.chain.end()));
	}
	| l_string_v l_string_v_apo
	{
		$chain = std::move($l_string_v.chain);
		$chain.insert($chain.end(), std::make_move_iterator($l_string_v_apo.chain.begin()), std::make_move_iterator($l_string_v_apo.chain.end()));
	}
	| l_string_v l_string_no_space_c
	{
		$chain = std::move($l_string_v.chain);
		$chain.push_back(std::make_unique<char_str_conc>(std::move($l_string_no_space_c.value)));
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
	| EQUALS												{$value = "=";}
	| COMMA													{$value = ",";}
	| LPAR													{$value = "(";}
	| RPAR													{$value = ")";};

common_ch_v returns [concat_point_ptr point]
	: ASTERISK												{$point = std::make_unique<char_str_conc>("*");}
	| MINUS													{$point = std::make_unique<char_str_conc>("-");}
	| PLUS													{$point = std::make_unique<char_str_conc>("+");}
	| LT													{$point = std::make_unique<char_str_conc>("<");}
	| GT													{$point = std::make_unique<char_str_conc>(">");}
	| SLASH													{$point = std::make_unique<char_str_conc>("/");}
	| EQUALS												{$point = std::make_unique<equals_conc>();}
	| AMPERSAND AMPERSAND									{$point = std::make_unique<char_str_conc>("&&");}
	| VERTICAL												{$point = std::make_unique<char_str_conc>("|");}
	| IDENTIFIER											{$point = std::make_unique<char_str_conc>($IDENTIFIER->getText());}
	| NUM													{$point = std::make_unique<char_str_conc>($NUM->getText());}
	| ORDSYMBOL												{$point = std::make_unique<char_str_conc>($ORDSYMBOL->getText());}
	| DOT													{$point = std::make_unique<dot_conc>();}											
	| var_symbol											{$point = std::make_unique<var_sym_conc>(std::move($var_symbol.vs));};

l_ch_v returns [concat_point_ptr point]
	: common_ch_v											{$point = std::move($common_ch_v.point);}
	| EQUALS												{$point = std::make_unique<char_str_conc>("=");}
	| COMMA													{$point = std::make_unique<char_str_conc>(",");}
	| LPAR													{$point = std::make_unique<char_str_conc>("(");}
	| RPAR													{$point = std::make_unique<char_str_conc>(")");};

l_str_v returns [concat_chain chain]
	:														
	| tmp=l_str_v l_ch_v									{$tmp.chain.push_back(std::move($l_ch_v.point)); $chain=std::move($tmp.chain);};

l_string returns [std::string value]
	: l_ch													{$value = std::move($l_ch.value);}
	| str=l_string l_ch										{$value.append(std::move($str.value)); $value.append(std::move($l_ch.value));};

l_string_v returns [concat_chain chain]
	: l_string_o var_symbol l_str_v							
	{
		$chain.push_back(std::make_unique<char_str_conc>(std::move($l_string_o.value))); 
		$chain.push_back(std::make_unique<var_sym_conc>(std::move($var_symbol.vs)));
		$chain.insert($chain.end(), std::make_move_iterator($l_str_v.chain.begin()), std::make_move_iterator($l_str_v.chain.end()));
	};

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

l_string_no_space_v returns [concat_chain chain]
	: l_apo l_string_o l_apo l_string_v
	{
		std::string tmp("'"); tmp.append(std::move($l_string_o.value)); tmp.append("'");
		$chain.push_back(std::make_unique<char_str_conc>(std::move(tmp)));
		$chain.insert($chain.end(), std::make_move_iterator($l_string_v.chain.begin()), std::make_move_iterator($l_string_v.chain.end()));
	}
	| l_apo l_string_v l_apo l_string_o
	{
		$chain.push_back(std::make_unique<char_str_conc>("'"));
		$chain.insert($chain.end(), std::make_move_iterator($l_string_v.chain.begin()), std::make_move_iterator($l_string_v.chain.end()));
		$chain.push_back(std::make_unique<char_str_conc>("'"));
		$chain.push_back(std::make_unique<char_str_conc>(std::move($l_string_o.value)));
	}
	| l_apo str1=l_string_v l_apo str2=l_string_v
	{
		$chain.push_back(std::make_unique<char_str_conc>("'"));
		$chain.insert($chain.end(), std::make_move_iterator($str1.chain.begin()), std::make_move_iterator($str1.chain.end()));
		$chain.push_back(std::make_unique<char_str_conc>("'"));
		$chain.insert($chain.end(), std::make_move_iterator($str2.chain.begin()), std::make_move_iterator($str2.chain.end()));
	};

l_string_no_space_u returns [concat_chain chain]
	: l_string_no_space_v													{$chain = std::move($l_string_no_space_v.chain);}
	| l_apo str1=l_string_o l_apo str2=l_string_o
	{
		std::string tmp("'"); tmp.append(std::move($str1.value)); tmp.append("'");  tmp.append(std::move($str2.value));
		$chain.push_back(std::make_unique<char_str_conc>(std::move(tmp)));
	};

l_string_no_space_u_c returns [concat_chain chain]
	:  
	| cl1=l_string_no_space_u_c cl2=l_string_no_space_u
	{
		$chain = std::move($cl1.chain);
		$chain.insert($chain.end(), std::make_move_iterator($cl2.chain.begin()), std::make_move_iterator($cl2.chain.end()));	
	};

l_string_no_space_c_o returns [std::string value]
	: l_string_no_space_c														{$value = std::move($l_string_no_space_c.value);}
	| ;

l_string_v_apo returns [concat_chain chain]
	: cl1=l_string_no_space_c_o  cl2=l_string_no_space_v cl3=l_string_no_space_u_c
	{
		$chain.push_back(std::make_unique<char_str_conc>(std::move($cl1.value)));
		$chain.insert($chain.end(), std::make_move_iterator($cl2.chain.begin()), std::make_move_iterator($cl2.chain.end()));	
		$chain.insert($chain.end(), std::make_move_iterator($cl3.chain.begin()), std::make_move_iterator($cl3.chain.end()));
	};




l_sp_ch returns [std::string value] //l_ch with SPACE
	: l_ch															{$value = std::move($l_ch.value);}
	| SPACE															{$value = $SPACE->getText();}; 					
l_sp_ch_v returns [concat_point_ptr point]
	: l_ch_v														{$point = std::move($l_ch_v.point);}
	| SPACE															{$point = std::make_unique<char_str_conc>($SPACE->getText());};

l_sp_str_v returns [concat_chain chain]
	:		
	| tmp=l_sp_str_v l_sp_ch_v											{$chain = std::move($tmp.chain); $chain.push_back(std::move($l_sp_ch_v.point)); };

l_sp_string returns [std::string value]
	: 
	| tmp=l_sp_string l_sp_ch											{$value=std::move($tmp.value); $value.append(std::move($l_sp_ch.value));};

l_sp_string_v returns [concat_chain chain]
	: l_sp_string var_symbol l_sp_str_v
	{
		$chain.push_back(std::make_unique<char_str_conc>(std::move($l_sp_string.value))); 
		$chain.push_back(std::make_unique<var_sym_conc>(std::move($var_symbol.vs)));
		$chain.insert($chain.end(), std::make_move_iterator($l_sp_str_v.chain.begin()), std::make_move_iterator($l_sp_str_v.chain.end()));
	};


l_string_poss_space_c returns [std::string value]
	: l_apo l_sp_string l_apo										{$value.append("'"); $value.append(std::move($l_sp_string.value)); $value.append("'");}
	| tmp=l_string_poss_space_c l_apo l_sp_string l_apo			{$value=std::move($tmp.value); $value.append("'"); $value.append(std::move($l_sp_string.value)); $value.append("'");};

l_string_poss_space_c_o returns [std::string value]
	: l_string_poss_space_c													{$value = std::move($l_string_poss_space_c.value);}
	| ;

l_string_poss_space_u returns [concat_chain chain]
	: l_apo l_sp_string l_apo										
	{
		std::string tmp("'"); tmp.append(std::move($l_sp_string.value)); tmp.append("'"); 
		$chain.push_back(std::make_unique<char_str_conc>(std::move(tmp)));
	}
	| l_apo l_sp_string_v l_apo
	{
		$chain.push_back(std::make_unique<char_str_conc>("'"));
		$chain.insert($chain.end(), std::make_move_iterator($l_sp_string_v.chain.begin()), std::make_move_iterator($l_sp_string_v.chain.end()));
		$chain.push_back(std::make_unique<char_str_conc>("'"));
	};

l_string_poss_space_u_c returns [concat_chain chain]
	: tmp=l_string_poss_space_u_c l_string_poss_space_u							
	{
		$chain = std::move($tmp.chain); 
		$chain.insert($chain.end(), std::make_move_iterator($l_string_poss_space_u.chain.begin()), std::make_move_iterator($l_string_poss_space_u.chain.end()));
	}
	| ;

l_string_v_apo_sp returns [concat_chain chain]
	: cl1=l_string_poss_space_c_o l_apo cl2=l_sp_string_v l_apo cl3=l_string_poss_space_u_c
	{
		$cl1.value.append("'");
		$cl2.chain.push_back(std::make_unique<char_str_conc>("'"));
		$chain.push_back(std::make_unique<char_str_conc>(std::move($cl1.value)));
		$chain.insert($chain.end(), std::make_move_iterator($cl2.chain.begin()), std::make_move_iterator($cl2.chain.end()));
		$chain.insert($chain.end(), std::make_move_iterator($cl3.chain.begin()), std::make_move_iterator($cl3.chain.end()));
	};





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

