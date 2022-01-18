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

 //rules for data definition operand
parser grammar data_def_rules;

dat_op returns [operand_ptr op]
	: data_def {$op = std::make_unique<data_def_operand>(std::move($data_def.value),provider.get_range($data_def.ctx));};

mach_expr_pars returns [mach_expr_ptr e]
	: lpar mach_expr rpar
	{
		$e = std::move($mach_expr.m_e);
	};

data_def_string returns [std::string value]
	: ap1=(APOSTROPHE|ATTR) string_ch_c ap2=(APOSTROPHE|ATTR)
	{
		$value.append(std::move($string_ch_c.value));
		collector.add_hl_symbol(token_info(provider.get_range($ap1,$ap2),hl_scopes::string));
	};

nominal_value returns [nominal_value_ptr value]
	: data_def_string
	{
		$value = std::make_unique<nominal_value_string>($data_def_string.value, provider.get_range($data_def_string.ctx->getStart(),$data_def_string.ctx->getStop()));
	}
	| lpar exprs=mach_expr_or_address_comma_c rpar
	{
		$value = std::make_unique<nominal_value_exprs>(std::move($exprs.exprs));
	};

data_def_address returns [address_nominal addr]
	: disp=mach_expr lpar base=mach_expr rpar
	{
		$addr.base = std::move($base.m_e);
		$addr.displacement = std::move($disp.m_e);
	};

mach_expr_or_address_comma_c returns [expr_or_address_list exprs]
	: e=mach_expr
	{
		$exprs.push_back(std::move($e.m_e));
	}
	| a=data_def_address
	{
		$exprs.push_back(std::move($a.addr));
	}
	| p_list=mach_expr_or_address_comma_c comma e=mach_expr
	{
		$exprs = std::move($p_list.exprs);
		$exprs.push_back(std::move($e.m_e));
	}
	| p_list=mach_expr_or_address_comma_c comma data_def_address
	{
		$exprs = std::move($p_list.exprs);
		$exprs.push_back(std::move($data_def_address.addr));
	}
	;
	
data_def_text [bool allow_sign]
	: ({$allow_sign}? (plus|minus))? (IDENTIFIER|NUM|ORDSYMBOL)+;

data_def returns [data_definition value] locals [data_definition_parser p]
	:
	{ $p.set_collector(collector); }
	(
		{ $p.allowed().expression }? mach_expr_pars	{if ($mach_expr_pars.e) $p.push(std::move($mach_expr_pars.e), provider.get_range($mach_expr_pars.ctx)); }
		|
		{ $p.allowed().string || $p.allowed().plus_minus }? data_def_text[$p.allowed().plus_minus] {$p.push($data_def_text.text, provider.get_range($data_def_text.ctx));}
		|
		{ $p.allowed().dot }? DOT {$p.push(".", provider.get_range($DOT));}
	)+
	(nominal_value {$p.push(std::move($nominal_value.value));})?
	{
		$value = $p.take_result();
	};
