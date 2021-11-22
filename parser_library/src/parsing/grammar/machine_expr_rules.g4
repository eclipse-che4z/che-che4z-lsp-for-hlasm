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

 //rules for machine expression, literal, data attribute, string
parser grammar machine_expr_rules; 

mach_expr returns [mach_expr_ptr m_e]
	: begin=mach_expr_s 
	{
		$m_e = mach_expression::assign_expr(std::move($begin.m_e),provider.get_range($begin.ctx));
	} 
	((plus|minus) next=mach_expr_s
	{
		if ($plus.ctx)
			$m_e = std::make_unique<mach_expr_binary<add>>(std::move($m_e), std::move($next.m_e), provider.get_range( $begin.ctx->getStart(), $next.ctx->getStop()));
		else
			$m_e = std::make_unique<mach_expr_binary<sub>>(std::move($m_e), std::move($next.m_e), provider.get_range( $begin.ctx->getStart(), $next.ctx->getStop()));
		$plus.ctx = nullptr;
	}
	)*;

mach_expr_s returns [mach_expr_ptr m_e]
	: begin=mach_term_c 
	{
		$m_e = std::move($begin.m_e);
	} 
	((slash|asterisk) next=mach_term_c
	{
		if ($slash.ctx)
			$m_e = std::make_unique<mach_expr_binary<div>>(std::move($m_e), std::move($next.m_e), provider.get_range( $begin.ctx->getStart(), $next.ctx->getStop()));
		else
			$m_e = std::make_unique<mach_expr_binary<mul>>(std::move($m_e), std::move($next.m_e), provider.get_range( $begin.ctx->getStart(), $next.ctx->getStop()));
		$slash.ctx = nullptr;
	}
	)*;

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
	| mach_location_counter
	{
		$m_e = std::make_unique<mach_expr_location_counter>( provider.get_range( $mach_location_counter.ctx));
	}
	| {is_data_attr()}? mach_data_attribute
	{
		auto rng = provider.get_range($mach_data_attribute.ctx);
		auto attr = $mach_data_attribute.attribute;
		if(attr == data_attr_kind::UNKNOWN)
			$m_e = std::make_unique<mach_expr_default>(rng);
		else
			$m_e = std::visit([rng, attr, symbol_rng = $mach_data_attribute.symbol_rng](auto& arg) -> mach_expr_ptr {
				if constexpr (std::is_same_v<std::decay_t<decltype(arg)>,std::monostate>) {
					return std::make_unique<mach_expr_default>(rng);
				}
				else {
					return std::make_unique<mach_expr_data_attr>(std::move(arg), attr, rng, symbol_rng);
				}
			}, $mach_data_attribute.data);
	}
	| id
	{
		collector.add_hl_symbol(token_info(provider.get_range( $id.ctx),hl_scopes::ordinary_symbol));
		$m_e = std::make_unique<mach_expr_symbol>($id.name, provider.get_range( $id.ctx));
	}
	| num
	{
		collector.add_hl_symbol(token_info(provider.get_range( $num.ctx),hl_scopes::number));
		$m_e =  std::make_unique<mach_expr_constant>($num.value, provider.get_range( $num.ctx));
	}
	| self_def_term
	{
		$m_e = std::make_unique<mach_expr_constant>($self_def_term.value, provider.get_range( $self_def_term.ctx));
	}
	| literal
	{
		$m_e = std::make_unique<mach_expr_literal>(provider.get_range($literal.ctx), std::move($literal.value), $literal.text);
	};


literal returns [data_definition value]
	: equals {allow_literals()}? {disable_litarals();} data_def
	{
		$value = std::move($data_def.value);
	};
	finally
	{enable_litarals();}

mach_data_attribute returns [data_attr_kind attribute, std::variant<std::monostate, id_index> data, range symbol_rng]
	: ORDSYMBOL (attr|apostrophe_as_attr) mach_data_attribute_value
	{
		collector.add_hl_symbol(token_info(provider.get_range($ORDSYMBOL), hl_scopes::data_attr_type));
		$attribute = get_attribute($ORDSYMBOL->getText());
		$data = std::move($mach_data_attribute_value.data);
		$symbol_rng = provider.get_range($mach_data_attribute_value.ctx);
	};

mach_data_attribute_value returns [std::variant<std::monostate, id_index> data]
	: literal
	{
		// if (auto lit_name = add_literal($literal.text, std::move($literal.value), provider.get_range($literal.ctx)))
		// 	$data = lit_name;
	}
	| mach_location_counter
	| id
	{
		collector.add_hl_symbol(token_info(provider.get_range($id.ctx), hl_scopes::ordinary_symbol));
		if (auto name = $id.name)
			$data = name;
	};

string_ch returns [std::string value]
	: l_sp_ch
	{
		if ($l_sp_ch.value == "&&")
			$value = "&";
		else
			$value = std::move($l_sp_ch.value);

	}
	| (APOSTROPHE|ATTR) (APOSTROPHE|ATTR)	{$value = "'";};

string_ch_c returns [std::string value]
	:
	| tmp=string_ch_c string_ch				{$value = std::move($tmp.value); $value.append($string_ch.value);};

string returns [std::string value]
	: ap1=APOSTROPHE string_ch_c ap2=(APOSTROPHE|ATTR)
	{
		$value.append(std::move($string_ch_c.value));
		collector.add_hl_symbol(token_info(provider.get_range($ap1,$ap2),hl_scopes::string)); 
	};

mach_location_counter
	: ASTERISK
	{
		collector.add_hl_symbol(token_info(provider.get_range( $ASTERISK), hl_scopes::operand));
	};
