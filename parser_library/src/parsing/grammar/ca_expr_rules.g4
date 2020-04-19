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

 //rules for CA expresssions, variable symbol, CA string
parser grammar ca_expr_rules; 


expr // returns [expr_ptr e]
	: SPACE* expr_p_space_c SPACE*;

expr_p returns [vs_ptr* vs_link = nullptr]
	: expr_s
	{
		$vs_link = std::move($expr_s.vs_link);
	}
	| plus tmp=expr_p
	| minus tmp=expr_p;

expr_s returns [vs_ptr* vs_link = nullptr]
	: t=term_c											
	{
		$vs_link = std::move($term_c.vs_link);
	}
	| tmp=expr_s minus term_c
	| tmp=expr_s plus term_c;

term_c returns [vs_ptr* vs_link = nullptr]
	: t=term												
	{
		$vs_link = std::move($term.vs_link);
	}
	| tmp=term_c slash term	
	| tmp=term_c asterisk term;

term returns [vs_ptr* vs_link = nullptr]
	: lpar expr rpar									
	| var_symbol
	{
		$vs_link = &$var_symbol.vs;
	}
	| ca_string											
	{ 
		collector.add_hl_symbol(token_info(provider.get_range( $ca_string.ctx),hl_scopes::string));
	}
	| data_attribute
	| {is_self_def()}? ORDSYMBOL string
	| num
	| id_sub
	{ 
		$vs_link = &$id_sub.vs;
	};

id_sub returns [vs_ptr vs]
	: id_no_dot subscript
	{ 
		$vs = std::make_unique<basic_var_sym>($id_no_dot.name,std::move($subscript.value),provider.get_range( $id_no_dot.ctx->getStart(),$subscript.ctx->getStop()));
	};

expr_p_comma_c returns [std::vector<ParserRuleContext*> ext]
	: expr_p
	{ 
		$ext.push_back($expr_p.ctx); 
	}
	| exs=expr_p_comma_c comma expr_p
	{ 
		$exs.ext.push_back($expr_p.ctx); 
		$ext = $exs.ext;
	};
	
expr_p_space_c returns [std::deque<ParserRuleContext*> ext]
 	: expr_p
	{ 
		$ext.push_back($expr_p.ctx); 
	}
	| exs=expr_p_space_c SPACE* expr_p
	{ 
		$exs.ext.push_back($expr_p.ctx); 
		$ext = $exs.ext;
	};
 


seq_symbol returns [seq_sym ss]
	: dot_ id_no_dot		
	{	
		$ss = seq_sym{$id_no_dot.name,provider.get_range( $dot_.ctx->getStart(),$id_no_dot.ctx->getStop())};
	};



subscript returns [std::vector<ParserRuleContext*> value]
	: lpar expr_p_comma_c rpar
	{
		$value = $expr_p_comma_c.ext;
	}
	| ;



created_set_body returns [concat_point_ptr point]
	: ORDSYMBOL												{$point = std::make_unique<char_str>($ORDSYMBOL->getText());}
	| IDENTIFIER											{$point = std::make_unique<char_str>($IDENTIFIER->getText());}
	| NUM													{$point = std::make_unique<char_str>($NUM->getText());}
	| var_symbol											{$point = std::move($var_symbol.vs);}
	| dot_													{$point = std::make_unique<dot>();};

created_set_body_c returns [concat_chain concat_list]
	: cl=created_set_body									{$concat_list.push_back(std::move($cl.point));}
	| clc=created_set_body_c cl=created_set_body			{$clc.concat_list.push_back(std::move($cl.point)); $concat_list =std::move($clc.concat_list);};

created_set_symbol returns [vs_ptr vs]
	: AMPERSAND lpar clc=created_set_body_c rpar subscript 	
	{
		$vs = std::make_unique<created_var_sym>(std::move($clc.concat_list),std::move($subscript.value),provider.get_range( $AMPERSAND,$subscript.ctx->getStop()));
	}
	| ampersand lpar rpar subscript; 	//empty set symbol err;			

var_symbol returns [vs_ptr vs]
	: AMPERSAND vs_id tmp=subscript								
	{
		auto id = $vs_id.name; 
		auto r = provider.get_range( $AMPERSAND,$tmp.ctx->getStop()); 
		$vs = std::make_unique<basic_var_sym>(id, std::move($tmp.value), r);
		collector.add_lsp_symbol(id,r,symbol_type::var);
		collector.add_hl_symbol(token_info(r,hl_scopes::var_symbol));
	}
	| created_set_symbol 									{$vs = std::move($created_set_symbol.vs);};

vs_id_ch_c
	: (NUM|ORDSYMBOL)+;

vs_id returns [id_index name = id_storage::empty_id]
	: ORDSYMBOL vs_id_ch_c
	{
		std::string tmp($ORDSYMBOL->getText());
		tmp.append($vs_id_ch_c.ctx->getText());
		$name = parse_identifier(std::move(tmp),provider.get_range($ORDSYMBOL,$vs_id_ch_c.ctx->getStop()));
	}
	| ORDSYMBOL									{$name = parse_identifier($ORDSYMBOL->getText(),provider.get_range($ORDSYMBOL));};



ca_dupl_factor // returns [int32_t df]
	: lpar expr_p rpar															
	|;

substring // returns [expr_ptr start, expr_ptr end]
	: lpar e1=expr_p comma e2=expr_p rpar
	| lpar expr_p comma ASTERISK rpar
	| lpar expr_p rpar
	|;

ca_string_b // returns [std::unique_ptr<char_expr> e]
	: ca_dupl_factor (apostrophe|attr) string_ch_v_c (apostrophe|attr) substring;

ca_string // returns [std::unique_ptr<char_expr> e]
	: ca_string_b											//	{$e = std::move($ca_string_b.e);}
	| tmp=ca_string dot_ ca_string_b;

string_ch_v returns [concat_point_ptr point]
	: l_sp_ch_v								{$point = std::move($l_sp_ch_v.point);}
	| (APOSTROPHE|ATTR) (APOSTROPHE|ATTR)	{$point = std::make_unique<char_str>("'");};

string_ch_v_c returns [concat_chain chain]
	:
	| cl=string_ch_v_c string_ch_v		{$cl.chain.push_back(std::move($string_ch_v.point)); $chain = std::move($cl.chain);};