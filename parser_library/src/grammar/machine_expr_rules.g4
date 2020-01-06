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

parser grammar machine_expr_rules; 

mach_expr returns [mach_expr_ptr m_e]
	: l=mach_expr plus r=mach_expr_s
	{
		$m_e = std::make_unique<mach_expr_binary<add>>(std::move($l.m_e), std::move($r.m_e), provider.get_range( $l.ctx->getStart(), $r.ctx->getStop()));
	}
	| l=mach_expr minus r=mach_expr_s
	{
		$m_e = std::make_unique<mach_expr_binary<sub>>(std::move($l.m_e), std::move($r.m_e), provider.get_range( $l.ctx->getStart(), $r.ctx->getStop()));
	}
	| mach_expr_s
	{
		$m_e = mach_expression::assign_expr(std::move($mach_expr_s.m_e),provider.get_range($mach_expr_s.ctx));
	};

mach_expr_s returns [mach_expr_ptr m_e]
	: mach_term_c
	{
		$m_e = std::move($mach_term_c.m_e);
	}
	| l=mach_expr_s slash r=mach_term_c 
	{
		$m_e = std::make_unique<mach_expr_binary<div>>(std::move($l.m_e), std::move($r.m_e), provider.get_range( $l.ctx->getStart(), $r.ctx->getStop()));
	}
	| l=mach_expr_s asterisk r=mach_term_c
	{
		$m_e = std::make_unique<mach_expr_binary<mul>>(std::move($l.m_e), std::move($r.m_e), provider.get_range( $l.ctx->getStart(), $r.ctx->getStop()));
	};

mach_term_c returns [mach_expr_ptr m_e]
	: mach_term
	{
		$m_e = std::move($mach_term.m_e);
	}
	| plus mach_term_c
	{
		$m_e = std::make_unique<mach_expr_unary<add>>(std::move($mach_term_c.m_e), provider.get_range( $plus.ctx->getStart(), $mach_term_c.ctx->getStop()));
	}
	| minus mach_term_c
	{
		$m_e = std::make_unique<mach_expr_unary<sub>>(std::move($mach_term_c.m_e), provider.get_range( $minus.ctx->getStart(), $mach_term_c.ctx->getStop()));
	};

mach_term returns [mach_expr_ptr m_e]
	: lpar mach_expr rpar
	{
		$m_e = std::make_unique<mach_expr_unary<par>>(std::move($mach_expr.m_e), provider.get_range( $lpar.ctx->getStart(), $rpar.ctx->getStop()));
	}
	| asterisk								
	{ 
		$m_e = std::make_unique<mach_expr_location_counter>( provider.get_range( $asterisk.ctx));
	}
	| {is_data_attr()}? mach_data_attribute		
	{
		auto rng = provider.get_range( $mach_data_attribute.ctx);
		auto attr = get_attribute(std::move($mach_data_attribute.attribute),rng);
		if(attr == data_attr_kind::UNKNOWN || $mach_data_attribute.data == nullptr)
			$m_e = std::make_unique<mach_expr_default>(rng);
		else
			$m_e = std::make_unique<mach_expr_data_attr>($mach_data_attribute.data,attr,rng);
	}
	| id
	{
		$m_e = std::make_unique<mach_expr_symbol>($id.name, provider.get_range( $id.ctx));
		collector.add_lsp_symbol({*$id.name,provider.get_range($id.ctx),symbol_type::ord});
	}
	| num
	{
		$m_e =  std::make_unique<mach_expr_constant>($num.value, provider.get_range( $num.ctx));
	}
	| self_def_term
	{
		$m_e = std::make_unique<mach_expr_constant>($self_def_term.value, provider.get_range( $self_def_term.ctx));
	}
	| literal
	{
		$m_e = std::make_unique<mach_expr_constant>(0, provider.get_range($literal.ctx));
	};


literal
	: equals_ data_def;

mach_data_attribute returns [std::string attribute, id_index data = nullptr]
	: ORDSYMBOL ATTR literal	
	| ORDSYMBOL ATTR id				{$attribute = $ORDSYMBOL->getText(); $data = $id.name;};

data_attribute
	: ORDSYMBOL ATTR literal
	| ORDSYMBOL ATTR var_symbol
	| ORDSYMBOL ATTR id;


string_ch returns [std::string value]
	: l_sp_ch								{$value = std::move($l_sp_ch.value);}
	| (APOSTROPHE|ATTR) (APOSTROPHE|ATTR)	{$value = "'";};

string_ch_v returns [concat_point_ptr point]
	: l_sp_ch_v								{$point=std::move($l_sp_ch_v.point);}
	| (APOSTROPHE|ATTR) (APOSTROPHE|ATTR)	{$point = std::make_unique<char_str>("'");};

string_ch_c returns [std::string value]
	:
	| tmp=string_ch_c string_ch				{$value = std::move($tmp.value); $value.append($string_ch.value);};

string_ch_v_c returns [concat_chain chain]
	:
	| tmp=string_ch_v_c string_ch_v				{$tmp.chain.push_back(std::move($string_ch_v.point)); $chain = std::move($tmp.chain);};



string returns [std::string value]
	: ap1=APOSTROPHE string_ch_c ap2=(APOSTROPHE|ATTR)	
	{ 
		$value.append(std::move($string_ch_c.value));
		collector.add_hl_symbol(token_info(provider.get_range($ap1,$ap2),hl_scopes::string)); 
	};

string_v returns [concat_chain chain]
	: ap1=APOSTROPHE string_ch_v_c ap2=(APOSTROPHE|ATTR)	
	{ 
		$chain.push_back(std::make_unique<char_str>("'"));
		$chain.insert($chain.end(), 
			std::make_move_iterator($string_ch_v_c.chain.begin()), 
			std::make_move_iterator($string_ch_v_c.chain.end())
		);
		$chain.push_back(std::make_unique<char_str>("'"));
		collector.add_hl_symbol(token_info(provider.get_range($ap1,$ap2),hl_scopes::string)); 
	};
