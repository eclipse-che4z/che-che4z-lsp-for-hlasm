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

mach_expr_or_address returns [std::optional<expressions::expr_or_address> e]
	:
	disp=mach_expr (lpar base=mach_expr rpar)?
	{
		if ($disp.ctx && $base.ctx && $rpar.ctx) {
			if ($disp.m_e && $base.m_e)
				$e.emplace(std::in_place_type_t<address_nominal>{}, std::move($disp.m_e), std::move($base.m_e), provider.get_range($disp.ctx->getStart(), $rpar.ctx->getStop()));
		}
		else if ($disp.ctx)
		{
			if ($disp.m_e)
				$e.emplace(std::move($disp.m_e));
		}
	}
	;

mach_expr_or_address_comma_c returns [expr_or_address_list exprs]
	:
	f=mach_expr_or_address
	{
		if ($f.e)
			$exprs.emplace_back(std::move($f.e.value()));
	}
	(
		comma n=mach_expr_or_address
		{
			if ($n.e)
				$exprs.emplace_back(std::move($n.e.value()));
		}
	)*
	;

data_def_text returns [std::string value]
	: (t=(IDENTIFIER|NUM|ORDSYMBOL) {$value += $t->getText();})+;

data_def_text_plus_minus returns [std::string value]
	: (plus{$value="+";}|minus{$value="-";}) (t=(IDENTIFIER|NUM|ORDSYMBOL) {$value += $t->getText();})+;

data_def_base [data_definition_parser* p]
	:
	(
		mach_expr_pars
		{
			if ($mach_expr_pars.e) $p->push(std::move($mach_expr_pars.e), provider.get_range($mach_expr_pars.ctx));
		}
	)?
	data_def_text
	{
		$p->push($data_def_text.value, provider.get_range($data_def_text.ctx));
	}
	{
		if (!($p->allowed().expression || $p->allowed().string || $p->allowed().plus_minus || $p->allowed().dot)) goto PERF_HACK_SKIP_EXTRAS;
	}
	(
		{ $p->allowed().expression }? mach_expr_pars {if ($mach_expr_pars.e) $p->push(std::move($mach_expr_pars.e), provider.get_range($mach_expr_pars.ctx)); }
		|
		{ $p->allowed().string }? data_def_text {$p->push($data_def_text.value, provider.get_range($data_def_text.ctx));}
		|
		{ $p->allowed().plus_minus }? data_def_text_plus_minus {$p->push($data_def_text_plus_minus.value, provider.get_range($data_def_text_plus_minus.ctx));}
		|
		{ $p->allowed().dot }? DOT {$p->push(".", provider.get_range($DOT));}
	)*
	{
		PERF_HACK_SKIP_EXTRAS:;
	}
	;

data_def returns [data_definition value] locals [data_definition_parser p]
	:
	{ $p.set_collector(collector); }
	data_def_base[&$p]
	(nominal_value {$p.push(std::move($nominal_value.value));})?
	{
		auto [results, errors] = $p.take_result();
		$value = std::move(results);
		for (auto& d : errors)
			add_diagnostic(std::move(d));
	}
	;

data_def_with_nominal returns [data_definition value] locals [data_definition_parser p]
	:
	{ $p.set_collector(collector); }
	data_def_base[&$p]
	nominal_value {$p.push(std::move($nominal_value.value));}
	{
		auto [results, errors] = $p.take_result();
		$value = std::move(results);
		for (auto& d : errors)
			add_diagnostic(std::move(d));
	}
	;
