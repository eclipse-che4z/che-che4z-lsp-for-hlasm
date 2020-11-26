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

 //rules for macro operand
parser grammar macro_operand_rules; 


mac_op returns [operand_ptr op]
	: mac_preproc
	{
		$op = std::make_unique<macro_operand_string>($mac_preproc.ctx->getText(),provider.get_range($mac_preproc.ctx));
	};

mac_op_o returns [operand_ptr op] 
	: mac_entry?							
	{
		if($mac_entry.ctx)
			$op = std::make_unique<macro_operand_chain>(std::move($mac_entry.chain),provider.get_range($mac_entry.ctx));
		else
			$op = std::make_unique<semantics::empty_operand>(provider.original_range);
	};

macro_ops returns [operand_list list] 
	: mac_op_o  {$list.push_back(std::move($mac_op_o.op));} (comma mac_op_o {$list.push_back(std::move($mac_op_o.op));})* EOLLN EOF;



mac_preproc: mac_preproc_c+;

mac_preproc_c returns [vs_ptr vs]
	: asterisk
	| minus
	| plus
	| LT
	| GT
	| slash
	| equals
	| VERTICAL
	| IDENTIFIER											{collector.add_hl_symbol(token_info(provider.get_range($IDENTIFIER), hl_scopes::operand));}
	| NUM													{collector.add_hl_symbol(token_info(provider.get_range($NUM), hl_scopes::operand));}
	| ORDSYMBOL												{collector.add_hl_symbol(token_info(provider.get_range($ORDSYMBOL), hl_scopes::operand));}
	| dot									
	| AMPERSAND ORDSYMBOL									
	{
		auto r = provider.get_range($AMPERSAND,$ORDSYMBOL);
		$vs = std::make_unique<basic_variable_symbol>(hlasm_ctx->ids().add($ORDSYMBOL->getText()), std::vector<ca_expr_ptr>(), r);
		collector.add_lsp_symbol(hlasm_ctx->ids().add($ORDSYMBOL->getText()),r,symbol_type::var);
		collector.add_hl_symbol(token_info(r,hl_scopes::var_symbol));
	}
	| AMPERSAND LPAR
	| AMPERSAND AMPERSAND
	| lpar
	| rpar
	| attr
	| def_string;

mac_str_ch returns [concat_point_ptr point]
	: common_ch_v									{$point = std::move($common_ch_v.point);}
	| SPACE											{$point = std::make_unique<char_str_conc>($SPACE->getText());}//here next are for deferred
	| LPAR											{$point = std::make_unique<char_str_conc>("(");}
	| RPAR											{$point = std::make_unique<char_str_conc>(")");}
	| COMMA											{$point = std::make_unique<char_str_conc>(",");};

mac_str_b returns [concat_chain chain]
	:
	| tmp=mac_str_b mac_str_ch						{$tmp.chain.push_back(std::move($mac_str_ch.point)); $chain = std::move($tmp.chain);};

mac_str returns [concat_chain chain]
	: ap1=APOSTROPHE mac_str_b ap2=(APOSTROPHE|ATTR)				
	{
		$chain.push_back(std::make_unique<char_str_conc>("'"));
		$chain.insert($chain.end(), std::make_move_iterator($mac_str_b.chain.begin()), std::make_move_iterator($mac_str_b.chain.end()));
		$chain.push_back(std::make_unique<char_str_conc>("'"));
		collector.add_hl_symbol(token_info(provider.get_range($ap1,$ap2),hl_scopes::string)); 
	};

mac_ch returns [concat_chain chain]
	: common_ch_v									{$chain.push_back(std::move($common_ch_v.point));
													auto token = $common_ch_v.ctx->getStart();
													if (token->getType() == lexing::lexer::Tokens::ORDSYMBOL && $common_ch_v.ctx->getStop()->getType() == lexing::lexer::Tokens::ORDSYMBOL)
														collector.add_lsp_symbol(hlasm_ctx->ids().add(token->getText()),provider.get_range(token),symbol_type::ord);
													;}
	| ATTR											{$chain.push_back(std::make_unique<char_str_conc>("'"));}
	| mac_str										{$chain = std::move($mac_str.chain);}
	| mac_sublist									{$chain.push_back(std::move($mac_sublist.point));};

mac_ch_c returns [concat_chain chain]
	:
	| tmp=mac_ch_c mac_ch							
	{
		$chain = std::move($tmp.chain);
		$chain.insert($chain.end(), std::make_move_iterator($mac_ch.chain.begin()), std::make_move_iterator($mac_ch.chain.end()));
	};

mac_entry returns [concat_chain chain]
	: mac_ch										{$chain = std::move($mac_ch.chain);}
	| literal										{$chain.push_back(std::make_unique<char_str_conc>($literal.ctx->getText()));}
	| ORDSYMBOL EQUALS literal					
	{
		collector.add_hl_symbol(token_info(provider.get_range($ORDSYMBOL), hl_scopes::operand));
		$chain.push_back(std::make_unique<char_str_conc>($ORDSYMBOL->getText()));
		$chain.push_back(std::make_unique<char_str_conc>("="));
		$chain.push_back(std::make_unique<char_str_conc>($literal.ctx->getText()));
	}
	| tmp=mac_entry mac_ch							
	{
		$chain = std::move($tmp.chain);
		$chain.insert($chain.end(), std::make_move_iterator($mac_ch.chain.begin()), std::make_move_iterator($mac_ch.chain.end()));
	};
	finally
	{concatenation_point::clear_concat_chain($chain);}

mac_sublist_b_c returns [concat_chain chain]
	: mac_ch_c										{$chain = std::move($mac_ch_c.chain);}
	| literal										{$chain.push_back(std::make_unique<char_str_conc>($literal.ctx->getText()));};

mac_sublist_b returns [std::vector<concat_chain> chains]
	: mac_sublist_b_c										{$chains.push_back(std::move($mac_sublist_b_c.chain));}
	| tmp=mac_sublist_b comma mac_sublist_b_c			
	{
		$chains = std::move($tmp.chains);
		$chains.push_back(std::move($mac_sublist_b_c.chain));
	};

mac_sublist returns [concat_point_ptr point]
	: lpar mac_sublist_b rpar						{ $point = std::make_unique<sublist_conc>(std::move($mac_sublist_b.chains)); };