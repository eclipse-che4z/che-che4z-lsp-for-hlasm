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

 //rules for CA expressions, variable symbol, CA string
parser grammar ca_expr_rules; 

expr_general returns [ca_expr_ptr ca_expr]
	:
	{
		std::vector<ca_expr_ptr> ca_exprs;
	}
	(
		{NOT(_input->LT(1))}?
		ORDSYMBOL SPACE*
		{
			auto not_r = provider.get_range($ORDSYMBOL);
			collector.add_hl_symbol(token_info(not_r, hl_scopes::operand));
			ca_exprs.push_back(std::make_unique<ca_symbol>(parse_identifier($ORDSYMBOL->getText(), not_r), not_r)); 
		}
	)*
	expr
	(
		{
			if (!ca_exprs.empty()) {
				ca_exprs.push_back(std::move($expr.ca_expr)); 
				$ca_expr = std::make_unique<ca_expr_list>(std::move(ca_exprs), range(ca_exprs.front()->expr_range.start, ca_exprs.back()->expr_range.end), false);

			}
			else
				$ca_expr = std::move($expr.ca_expr);
		}
	);
	finally
	{if (!$ca_expr) $ca_expr = std::make_unique<ca_constant>(0, provider.get_range(_localctx));}

expr returns [ca_expr_ptr ca_expr]
	: begin=expr_s
	{
		$ca_expr = std::move($begin.ca_expr);
	} 
	(
		(
			(plus|minus) next=expr_s
			{
				auto r = provider.get_range($begin.ctx->getStart(), $next.ctx->getStop());
				if ($plus.ctx)
					$ca_expr = std::make_unique<ca_basic_binary_operator<ca_add>>(std::move($ca_expr), std::move($next.ca_expr), r);
				else
					$ca_expr = std::make_unique<ca_basic_binary_operator<ca_sub>>(std::move($ca_expr), std::move($next.ca_expr), r);
				$plus.ctx = nullptr;
			}
		)+
		|
		(
			dot term
			{
				auto r = provider.get_range($begin.ctx->getStart(), $term.ctx->getStop());
				$ca_expr = std::make_unique<ca_basic_binary_operator<ca_conc>>(std::move($ca_expr), std::move($term.ca_expr), r);
			}
		)+
		|
	);
	finally
	{if (!$ca_expr) $ca_expr = std::make_unique<ca_constant>(0, provider.get_range(_localctx));}

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
	finally
	{if (!$ca_expr) $ca_expr = std::make_unique<ca_constant>(0, provider.get_range(_localctx));}

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
	finally
	{if (!$ca_expr) $ca_expr = std::make_unique<ca_constant>(0, provider.get_range(_localctx));}

term returns [ca_expr_ptr ca_expr]
	: var_symbol
	{
		auto r = provider.get_range($var_symbol.ctx);
		$ca_expr = std::make_unique<ca_var_sym>(std::move($var_symbol.vs), r);
	}
	| {allow_ca_string()}? ca_string
	{ 
		auto r = provider.get_range($ca_string.ctx);
		collector.add_hl_symbol(token_info(r, hl_scopes::string));
		$ca_expr = std::move($ca_string.ca_expr);
	}
	| data_attribute
	{
		auto r = provider.get_range($data_attribute.ctx);
		auto val_range = $data_attribute.value_range;
		auto attr = $data_attribute.attribute;
		$ca_expr = std::visit([&r, &val_range, &attr](auto& v){
			return std::make_unique<ca_symbol_attribute>(std::move(v), attr, r, val_range);
		}, $data_attribute.value);
	}
	| {is_self_def()}? self_def_term
	{
		auto r = provider.get_range($self_def_term.ctx);
		$ca_expr = std::make_unique<ca_constant>($self_def_term.value, r);
	}
	| signed_num
	{
		collector.add_hl_symbol(token_info(provider.get_range( $signed_num.ctx),hl_scopes::number));
		auto r = provider.get_range($signed_num.ctx);
		$ca_expr = std::make_unique<ca_constant>($signed_num.value, r);
	}
	| ca_dupl_factor id_no_dot subscript_ne
	{
		collector.add_hl_symbol(token_info(provider.get_range( $id_no_dot.ctx),hl_scopes::operand));
		
		auto r = provider.get_range($ca_dupl_factor.ctx->getStart(), $subscript_ne.ctx->getStop());
		auto func = ca_common_expr_policy::get_function($id_no_dot.name.to_string_view());
		$ca_expr = std::make_unique<ca_function>($id_no_dot.name, func, std::move($subscript_ne.value), std::move($ca_dupl_factor.value), r);
	}
	| id_no_dot
	{
		collector.add_hl_symbol(token_info(provider.get_range( $id_no_dot.ctx),hl_scopes::operand));
		auto r = provider.get_range($id_no_dot.ctx);
		$ca_expr = std::make_unique<ca_symbol>($id_no_dot.name, r);
	}
	|
	expr_list
	{
		$ca_expr = std::move($expr_list.ca_expr);
	};
	finally
	{if (!$ca_expr) $ca_expr = std::make_unique<ca_constant>(0, provider.get_range(_localctx));}

signed_num returns [self_def_t value]
	: signed_num_ch									{$value = parse_self_def_term("D",get_context_text($signed_num_ch.ctx),provider.get_range($signed_num_ch.ctx));};

self_def_term returns [self_def_t value]
	: ORDSYMBOL string
	{
		collector.add_hl_symbol(token_info(provider.get_range( $ORDSYMBOL),hl_scopes::self_def_type));
		auto opt = $ORDSYMBOL->getText();
		$value = parse_self_def_term(opt, $string.value, provider.get_range($ORDSYMBOL,$string.ctx->getStop()));
	};

expr_list returns [ca_expr_ptr ca_expr]
	: lpar SPACE* expr_space_c SPACE* rpar
	{
		auto r = provider.get_range($lpar.ctx->getStart(), $rpar.ctx->getStop());
		$ca_expr = std::make_unique<ca_expr_list>(std::move($expr_space_c.ca_exprs), r, true);
	};
	finally
	{if (!$ca_expr) $ca_expr = std::make_unique<ca_constant>(0, provider.get_range(_localctx));}
	
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

seq_symbol returns [seq_sym ss = seq_sym{}]
	: DOT id_no_dot
	{
		$ss = seq_sym{$id_no_dot.name,provider.get_range( $DOT, $id_no_dot.ctx->getStop())};
	};

subscript_ne returns [std::vector<ca_expr_ptr> value]
	: lpar SPACE? expr SPACE? rpar
	{
		$value.push_back(std::move($expr.ca_expr));
	}
	| lpar expr comma expr_comma_c rpar
	{
		$value.push_back(std::move($expr.ca_expr));
		$value.insert($value.end(), std::make_move_iterator($expr_comma_c.ca_exprs.begin()),std::make_move_iterator($expr_comma_c.ca_exprs.end()));
	};

subscript returns [std::vector<ca_expr_ptr> value]
	: lpar expr_comma_c rpar
	{
		$value = std::move($expr_comma_c.ca_exprs);
	}
	|
	{_input->LA(1) != LPAR }?
	;


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

created_set_body returns [concat_chain concat_list]
	:
	(
		ORDSYMBOL
		{
			collector.add_hl_symbol(token_info(provider.get_range( $ORDSYMBOL),hl_scopes::var_symbol));
			$concat_list.emplace_back(char_str_conc($ORDSYMBOL->getText(), provider.get_range($ORDSYMBOL)));
		}
		|
		IDENTIFIER
		{
			collector.add_hl_symbol(token_info(provider.get_range( $IDENTIFIER),hl_scopes::var_symbol));
			$concat_list.emplace_back(char_str_conc($IDENTIFIER->getText(), provider.get_range($IDENTIFIER)));
		}
		|
		NUM
		{
			collector.add_hl_symbol(token_info(provider.get_range( $NUM),hl_scopes::var_symbol));
			$concat_list.emplace_back(char_str_conc($NUM->getText(), provider.get_range($NUM)));
		}
		|
		var_symbol
		{
			$concat_list.emplace_back(var_sym_conc(std::move($var_symbol.vs)));
		}
		|
		dot
		{
			$concat_list.emplace_back(dot_conc(provider.get_range($dot.ctx)));
		}
	)+
	;
	finally
	{concatenation_point::clear_concat_chain($concat_list);}

var_symbol returns [vs_ptr vs]
	:
	AMPERSAND
	(
		vs_id tmp=subscript
		{
			auto id = $vs_id.name;
			auto r = provider.get_range( $AMPERSAND,$tmp.ctx->getStop());
			$vs = std::make_unique<basic_variable_symbol>(id, std::move($tmp.value), r);
			collector.add_hl_symbol(token_info(provider.get_range( $AMPERSAND, $vs_id.ctx->getStop()),hl_scopes::var_symbol));
		}
		|
		lpar (clc=created_set_body)? rpar subscript
		{
			collector.add_hl_symbol(token_info(provider.get_range( $AMPERSAND),hl_scopes::var_symbol));
			$vs = std::make_unique<created_variable_symbol>($clc.ctx ? std::move($clc.concat_list) : concat_chain{},std::move($subscript.value),provider.get_range($AMPERSAND,$subscript.ctx->getStop()));
		}
	);
	finally
	{if (!$vs) $vs = std::make_unique<basic_variable_symbol>(parse_identifier("", {}), std::vector<expressions::ca_expr_ptr>{}, provider.get_range($ctx));}

data_attribute returns [context::data_attr_kind attribute, std::variant<context::id_index, semantics::vs_ptr, semantics::literal_si> value, range value_range]
	: ORDSYMBOL attr data_attribute_value
	{
		collector.add_hl_symbol(token_info(provider.get_range($ORDSYMBOL), hl_scopes::data_attr_type));
		$attribute = get_attribute($ORDSYMBOL->getText());
		$value = std::move($data_attribute_value.value);
		$value_range = provider.get_range( $data_attribute_value.ctx);
	};

data_attribute_value returns [std::variant<context::id_index, semantics::vs_ptr, semantics::literal_si> value]
	: literal
	{
		if (auto& lv = $literal.value; lv)
			$value = std::move(lv);
	}
	| var_symbol DOT? // in reality, this seems to be much more complicated (arbitrary many dots are consumed for *some* attributes)
	{
		$value = std::move($var_symbol.vs);
	}
	| id
	{
		collector.add_hl_symbol(token_info(provider.get_range( $id.ctx), hl_scopes::ordinary_symbol));
		$value = $id.name;
	};

vs_id returns [id_index name]
	: ORDSYMBOL
	{
		std::string text = $ORDSYMBOL->getText();
		auto first = $ORDSYMBOL;
		auto last = first;
	}
	(
		NUM
		{
			text += $NUM->getText();
			last = $NUM;
		}
		|
		ORDSYMBOL
		{
			text += $ORDSYMBOL->getText();
			last = $ORDSYMBOL;
		}
	)*
	{
		$name = parse_identifier(std::move(text), provider.get_range(first, last));
	};

var_def returns [vs_ptr vs]
	: var_def_name var_def_substr
	{
		auto r = provider.get_range($var_def_name.ctx);
		if ($var_def_name.created_name.empty())
		{
			$vs = std::make_unique<basic_variable_symbol>($var_def_name.name, std::move($var_def_substr.value), r);
			collector.add_hl_symbol(token_info(r,hl_scopes::var_symbol));
		}
		else
			$vs = std::make_unique<created_variable_symbol>(std::move($var_def_name.created_name), std::move($var_def_substr.value), r);	
	};

var_def_name returns [id_index name, concat_chain created_name]
	: AMPERSAND?
	(
		vs_id									{$name = $vs_id.name;}
		|
		lpar clc=created_set_body rpar			{$created_name = std::move($clc.concat_list);}
	);

var_def_substr returns [std::vector<ca_expr_ptr> value]
	: lpar num rpar 
	{
		auto r = provider.get_range($num.ctx);
		$value.emplace_back(std::make_unique<ca_constant>($num.value, r));
	}
	|;


ca_dupl_factor returns [ca_expr_ptr value]
	: lpar expr_general rpar
	{
		$value =std::move($expr_general.ca_expr);
	}
	|;

substring returns [expressions::ca_string::substring_t value]
	: lpar e1=expr_general comma (e2=expr_general|ASTERISK) rpar
	{
		$value.start = std::move($e1.ca_expr);
		$value.substring_range = provider.get_range($lpar.ctx->getStart(), $rpar.ctx->getStop());
		if ($e2.ctx)
		{
			$value.count = std::move($e2.ca_expr);
		}
	}
	|;

ca_string returns [ca_expr_ptr ca_expr]
	:
	(
		ca_dupl_factor (apostrophe|attr) string_ch_v_c (APOSTROPHE|ATTR) substring
		{
			auto r = provider.get_range($ca_dupl_factor.ctx->getStart(), $substring.ctx->getStop());
			auto next = std::make_unique<expressions::ca_string>(std::move($string_ch_v_c.chain), std::move($ca_dupl_factor.value), std::move($substring.value), r);
			auto& ca_expr = $ca_expr;
			if (!ca_expr)
				ca_expr = std::move(next);
			else
				ca_expr = std::make_unique<ca_basic_binary_operator<ca_conc>>(std::move(ca_expr), std::move(next), provider.get_range($ctx->getStart(), $substring.ctx->getStop()));
		}
	)+;
	finally
	{if (!$ca_expr) $ca_expr = std::make_unique<ca_constant>(0, provider.get_range(_localctx));}

string_ch_v [concat_chain* chain]
	: l_sp_ch_v[$chain]
	| l=(APOSTROPHE|ATTR) r=(APOSTROPHE|ATTR)	{$chain->emplace_back(char_str_conc("'", provider.get_range($l, $r)));};

string_ch_v_c returns [concat_chain chain]
	:
	(
		string_ch_v[&$chain]
	)*
	;
	finally
	{concatenation_point::clear_concat_chain($chain);}
