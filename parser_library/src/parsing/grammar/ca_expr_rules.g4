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

expr returns [ca_expr_ptr ca_expr]
	: begin=expr_s								
	{
		$ca_expr = std::move($begin.ca_expr);
	} 
	((plus|minus) next=expr_s
	{
		auto r = provider.get_range($begin.ctx->getStart(), $next.ctx->getStop());
		if ($plus.ctx)
			$ca_expr = std::make_unique<ca_basic_binary_operator<ca_add>>(std::move($ca_expr), std::move($next.ca_expr), r);
		else
			$ca_expr = std::make_unique<ca_basic_binary_operator<ca_sub>>(std::move($ca_expr), std::move($next.ca_expr), r);
		$plus.ctx = nullptr;
	}
	)*;

expr_s returns [ca_expr_ptr ca_expr]
	: begin=term_c								
	{
		$ca_expr = std::move($begin.ca_expr);
	} 
	((slash|asterisk) next=term_c
	{
		auto r = provider.get_range($begin.ctx->getStart(), $next.ctx->getStop());
		if ($slash.ctx)
			$ca_expr = std::make_unique<ca_basic_binary_operator<ca_div>>(std::move($ca_expr), std::move($next.ca_expr), r);
		else
			$ca_expr = std::make_unique<ca_basic_binary_operator<ca_mul>>(std::move($ca_expr), std::move($next.ca_expr), r);
		$slash.ctx = nullptr;
	}
	)*;

term_c returns [ca_expr_ptr ca_expr]
	: term
	{
		$ca_expr = std::move($term.ca_expr);
	}
	| plus tmp=term_c
	{
		auto r = provider.get_range($plus.ctx->getStart(), $tmp.ctx->getStop());
		$ca_expr = std::make_unique<ca_plus_operator>(std::move($tmp.ca_expr), r);
	}
	| minus tmp=term_c
	{
		auto r = provider.get_range($minus.ctx->getStart(), $tmp.ctx->getStop());
		$ca_expr = std::make_unique<ca_minus_operator>(std::move($tmp.ca_expr), r);
	};

term returns [ca_expr_ptr ca_expr]
	: expr_list
	{
		$ca_expr = std::move($expr_list.ca_expr);
	}
	| var_symbol
	{
		auto r = provider.get_range($var_symbol.ctx);
		$ca_expr = std::make_unique<ca_var_sym>(std::move($var_symbol.vs), r);
	}
	| ca_string	
	{ 
		auto r = provider.get_range($ca_string.ctx);
		collector.add_hl_symbol(token_info(r, hl_scopes::string));
		$ca_expr = std::move($ca_string.ca_expr);
	}
	| data_attribute
	{
		auto r = provider.get_range($data_attribute.ctx);
		if (std::holds_alternative<id_index>($data_attribute.value))
			$ca_expr = std::make_unique<ca_symbol_attribute>(std::get<id_index>($data_attribute.value), $data_attribute.attribute, r);
		else if (std::holds_alternative<vs_ptr>($data_attribute.value))
			$ca_expr = std::make_unique<ca_symbol_attribute>(std::move(std::get<vs_ptr>($data_attribute.value)), $data_attribute.attribute, r);
	}
	| {is_self_def()}? self_def_term
	{
		auto r = provider.get_range($self_def_term.ctx);
		$ca_expr = std::make_unique<ca_constant>($self_def_term.value, r);
	}
	| num
	{
		auto r = provider.get_range($num.ctx);
		$ca_expr = std::make_unique<ca_constant>($num.value, r);
	}
	| ca_dupl_factor id_no_dot subscript_ne
	{ 
		auto r = provider.get_range($ca_dupl_factor.ctx->getStart(), $subscript_ne.ctx->getStop());

		auto func = ca_common_expr_policy::get_function(*$id_no_dot.name);
		auto [param_size, param_kind] = ca_common_expr_policy::get_function_param_info(func, ca_common_expr_policy::get_function_type(func));
		resolve_expression($subscript_ne.value, param_kind);

		$ca_expr = std::make_unique<ca_function>($id_no_dot.name, func, std::move($subscript_ne.value), std::move($ca_dupl_factor.value), r);
	}
	| id_no_dot
	{
		auto r = provider.get_range($id_no_dot.ctx);
		$ca_expr = std::make_unique<ca_symbol>($id_no_dot.name, r);
	};

expr_list returns [ca_expr_ptr ca_expr]
	: lpar SPACE* expr_space_c SPACE* rpar
	{
		auto r = provider.get_range($lpar.ctx->getStart(), $rpar.ctx->getStop());
		$ca_expr = std::make_unique<ca_expr_list>(std::move($expr_space_c.ca_exprs), r);
	};
	
expr_space_c returns [std::vector<ca_expr_ptr> ca_exprs]
 	: expr
	{ 
		$ca_exprs.push_back(std::move($expr.ca_expr)); 
	}
	| tmp=expr_space_c SPACE* expr
	{ 
		$tmp.ca_exprs.push_back(std::move($expr.ca_expr)); 
		$ca_exprs = std::move($tmp.ca_exprs);
	};
 


seq_symbol returns [seq_sym ss]
	: dot id_no_dot		
	{	
		$ss = seq_sym{$id_no_dot.name,provider.get_range( $dot.ctx->getStart(),$id_no_dot.ctx->getStop())};
	};

subscript_ne returns [std::vector<ca_expr_ptr> value]
	: lpar expr_comma_c rpar
	{
		$value = std::move($expr_comma_c.ca_exprs);
	};

subscript returns [std::vector<ca_expr_ptr> value]
	: subscript_ne
	{
		$value = std::move($subscript_ne.value);
		resolve_expression($value, context::SET_t_enum::A_TYPE);
	}
	| ;


expr_comma_c returns [std::vector<ca_expr_ptr> ca_exprs]
	: expr
	{ 
		$ca_exprs.push_back(std::move($expr.ca_expr));
	}
	| tmp=expr_comma_c comma expr
	{ 
		$tmp.ca_exprs.push_back(std::move($expr.ca_expr));
		$ca_exprs = std::move($tmp.ca_exprs);
	};



created_set_body returns [concat_point_ptr point]
	: ORDSYMBOL												{$point = std::make_unique<char_str_conc>($ORDSYMBOL->getText());}
	| IDENTIFIER											{$point = std::make_unique<char_str_conc>($IDENTIFIER->getText());}
	| NUM													{$point = std::make_unique<char_str_conc>($NUM->getText());}
	| var_symbol											{$point = std::make_unique<var_sym_conc>(std::move($var_symbol.vs));}
	| dot													{$point = std::make_unique<dot_conc>();};

created_set_body_c returns [concat_chain concat_list]
	: cl=created_set_body									{$concat_list.push_back(std::move($cl.point));}
	| clc=created_set_body_c cl=created_set_body			{$clc.concat_list.push_back(std::move($cl.point)); $concat_list =std::move($clc.concat_list);};

created_set_symbol returns [vs_ptr vs]
	: AMPERSAND lpar clc=created_set_body_c rpar subscript 	
	{
		$vs = std::make_unique<created_variable_symbol>(std::move($clc.concat_list),std::move($subscript.value),provider.get_range($AMPERSAND,$subscript.ctx->getStop()));
	}
	| ampersand lpar rpar subscript; 	//empty set symbol err TODO

var_symbol returns [vs_ptr vs]
	: AMPERSAND vs_id tmp=subscript								
	{
		auto id = $vs_id.name; 
		auto r = provider.get_range( $AMPERSAND,$tmp.ctx->getStop()); 
		$vs = std::make_unique<basic_variable_symbol>(id, std::move($tmp.value), r);
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



var_def returns [vs_ptr vs]
	: var_def_name var_def_substr
	{
		auto r = provider.get_range($var_def_name.ctx);
		if ($var_def_name.created_name.empty())
		{
			$vs = std::make_unique<basic_variable_symbol>($var_def_name.name, std::move($var_def_substr.value), r);
			collector.add_lsp_symbol($var_def_name.name,r,symbol_type::var);
			collector.add_hl_symbol(token_info(r,hl_scopes::var_symbol));
		}
		else
			$vs = std::make_unique<created_variable_symbol>(std::move($var_def_name.created_name), std::move($var_def_substr.value), r);	
	};

var_def_name returns [id_index name, concat_chain created_name]
	: AMPERSAND? vs_id									{$name = $vs_id.name; }
	| AMPERSAND? lpar clc=created_set_body_c rpar		{$created_name = std::move($clc.concat_list);};

var_def_substr returns [std::vector<ca_expr_ptr> value]
	: lpar num rpar 
	{
		auto r = provider.get_range($num.ctx);
		$value.emplace_back(std::make_unique<ca_constant>($num.value, r));
	}
	|;


ca_dupl_factor returns [ca_expr_ptr value]
	: lpar expr rpar							
	{
		$value =std::move($expr.ca_expr);
		resolve_expression($value, context::SET_t_enum::A_TYPE);
	}										
	|;

substring returns [expressions::ca_string::substring_t value]
	: lpar e1=expr comma e2=expr rpar
	{
		$value.start = std::move($e1.ca_expr);
		$value.count = std::move($e2.ca_expr);
		$value.substring_range = provider.get_range($lpar.ctx->getStart(), $rpar.ctx->getStop());
		resolve_expression($value.start, context::SET_t_enum::A_TYPE);
		resolve_expression($value.count, context::SET_t_enum::A_TYPE);
	}
	| lpar expr comma ASTERISK rpar
	{
		$value.start = std::move($expr.ca_expr);
		$value.substring_range = provider.get_range($lpar.ctx->getStart(), $rpar.ctx->getStop());
		resolve_expression($value.start, context::SET_t_enum::A_TYPE);
	}
	|;

ca_string_b returns [ca_expr_ptr ca_expr]
	: ca_dupl_factor (apostrophe|attr) string_ch_v_c (apostrophe|attr) substring
	{
		auto r = provider.get_range($ca_dupl_factor.ctx->getStart(), $substring.ctx->getStop());
		$ca_expr = std::make_unique<expressions::ca_string>(std::move($string_ch_v_c.chain), std::move($ca_dupl_factor.value), std::move($substring.value), r);
	};

ca_string returns [ca_expr_ptr ca_expr]
	: ca_string_b
	{
		$ca_expr = std::move($ca_string_b.ca_expr);
	}
	| tmp=ca_string dot ca_string_b
	{
		auto r = provider.get_range($tmp.ctx->getStart(), $ca_string_b.ctx->getStop());
		$ca_expr = std::make_unique<ca_basic_binary_operator<ca_conc>>(std::move($tmp.ca_expr), std::move($ca_string_b.ca_expr), r);
	};

string_ch_v returns [concat_point_ptr point]
	: l_sp_ch_v								{$point = std::move($l_sp_ch_v.point);}
	| (APOSTROPHE|ATTR) (APOSTROPHE|ATTR)	{$point = std::make_unique<char_str_conc>("'");};

string_ch_v_c returns [concat_chain chain]
	:
	| cl=string_ch_v_c string_ch_v		{$cl.chain.push_back(std::move($string_ch_v.point)); $chain = std::move($cl.chain);};