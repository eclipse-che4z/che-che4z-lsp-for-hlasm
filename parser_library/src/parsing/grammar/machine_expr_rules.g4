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
				else if constexpr (std::is_same_v<std::decay_t<decltype(arg)>,int>) {
					return std::make_unique<mach_expr_constant>(arg, symbol_rng);
				}
				else if constexpr (std::is_same_v<std::decay_t<decltype(arg)>,std::unique_ptr<mach_expr_literal>>) {
					return std::make_unique<mach_expr_data_attr_literal>(std::move(arg), attr, rng, symbol_rng);
				}
				else {
					return std::make_unique<mach_expr_data_attr>(arg.first, arg.second, attr, rng, symbol_rng);
				}
			}, $mach_data_attribute.data);
	}
	| self_def_term
	{
		$m_e = std::make_unique<mach_expr_constant>($self_def_term.value, provider.get_range( $self_def_term.ctx));
	}
	| id
	{
		collector.add_hl_symbol(token_info(provider.get_range( $id.ctx),hl_scopes::ordinary_symbol));
		$m_e = std::make_unique<mach_expr_symbol>($id.name, $id.using_qualifier, provider.get_range( $id.ctx));
	}
	| num
	{
		collector.add_hl_symbol(token_info(provider.get_range( $num.ctx),hl_scopes::number));
		$m_e =  std::make_unique<mach_expr_constant>($num.value, provider.get_range( $num.ctx));
	}
	| literal
	{
		auto rng = provider.get_range($literal.ctx);
		if (auto& lv = $literal.value; lv)
			$m_e = std::make_unique<mach_expr_literal>(rng, std::move(lv));
		else
			$m_e = std::make_unique<mach_expr_default>(rng);
	};


literal returns [literal_si value]
	: literal_internal
	{
		if (auto& v = $literal_internal.value; v.has_value())
			$value = collector.add_literal($literal_internal.text, std::move(v.value()), provider.get_range($literal_internal.ctx));
	};

literal_internal returns [std::optional<data_definition> value]
	: equals
	{
		bool lit_allowed = allow_literals();
		auto lit_restore = disable_literals();
	}
	data_def_with_nominal
	{
		if (lit_allowed)
			$value.emplace(std::move($data_def_with_nominal.value));
		else
			add_diagnostic(diagnostic_severity::error, "S0013", "Invalid literal usage", provider.get_range($equals.ctx));
	};

mach_data_attribute returns [data_attr_kind attribute, std::variant<std::monostate, std::pair<id_index,id_index>, std::unique_ptr<mach_expr_literal>, int> data, range symbol_rng]
	: ORDSYMBOL (attr|apostrophe_as_attr) {auto lit_restore = enable_literals();}
	{
		collector.add_hl_symbol(token_info(provider.get_range($ORDSYMBOL), hl_scopes::data_attr_type));
		$attribute = get_attribute($ORDSYMBOL->getText());
	}
	(
		{ loctr_len_allowed($ORDSYMBOL.text) }? mach_location_counter
		{
			$data = (int)get_loctr_len();
		}
		|
		mach_data_attribute_value
		{
			std::visit([&data = $data](auto&x){data=std::move(x);}, $mach_data_attribute_value.data);
			$symbol_rng = provider.get_range($mach_data_attribute_value.ctx);
		}
	);

mach_data_attribute_value returns [std::variant<std::monostate, std::pair<id_index,id_index>, std::unique_ptr<mach_expr_literal>> data]
	: literal
	{
		auto rng = provider.get_range($literal.ctx);
		if (auto& lv = $literal.value; lv)
			$data = std::make_unique<mach_expr_literal>(rng, std::move(lv));
	}
	| id
	{
		collector.add_hl_symbol(token_info(provider.get_range($id.ctx), hl_scopes::ordinary_symbol));
		if (auto name = $id.name)
			$data = std::make_pair(name, $id.using_qualifier);
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
