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

 //rules for model operand
parser grammar model_operand_rules; 


op_ch returns [std::string value]
	: common_ch								{$value = std::move($common_ch.value);}
	| lpar									{$value = "("; }
	| rpar									{$value = ")"; }
	| comma									{$value = ","; }
	| ATTR									{$value = "'"; };

op_ch_v [concat_chain* chain]
	: common_ch_v[$chain]
	| lpar									{$chain->emplace_back(char_str_conc("(", provider.get_range($lpar.ctx->getStart()))); }
	| rpar									{$chain->emplace_back(char_str_conc(")", provider.get_range($rpar.ctx->getStart()))); }
	| comma									{$chain->emplace_back(char_str_conc(",", provider.get_range($comma.ctx->getStart()))); }
	| ATTR									{$chain->emplace_back(char_str_conc("'", provider.get_range($ATTR))); };

model_string returns [std::string value]
	: ap1=APOSTROPHE
	(
		l_sp_ch
		{
			if ($l_sp_ch.value == "&&")
				$value.append("&");
			else
				$value.append(std::move($l_sp_ch.value));

		}
		|
		(APOSTROPHE|ATTR) (APOSTROPHE|ATTR)	{$value.append("''");}
	)*?
	ap2=(APOSTROPHE|ATTR)
	{
		collector.add_hl_symbol(token_info(provider.get_range($ap1,$ap2),hl_scopes::string));
	};

before_var_sym_model_string returns [std::string value]
	: ap1=APOSTROPHE model_string_ch_c ap2=(APOSTROPHE|ATTR)
	{
	    $value.append(std::move($model_string_ch_c.value));
		collector.add_hl_symbol(token_info(provider.get_range($ap1,$ap2),hl_scopes::string));
	};

before_var_sym_model_b returns [std::string value]
	: op_ch												{$value = std::move($op_ch.value);}
	|
	before_var_sym_model_string
	{
		$value.reserve($before_var_sym_model_string.value.size()+2);
		$value.push_back('\'');
		$value.append($before_var_sym_model_string.value);
		$value.push_back('\'');
	};

before_var_sym_model returns [std::string value]
	: 
	| tmp=before_var_sym_model before_var_sym_model_b	{$tmp.value.append(std::move($before_var_sym_model_b.value)); $value = std::move($tmp.value);};

var_sym_model [concat_chain* chain]
	: var_symbol										{$chain->emplace_back(var_sym_conc(std::move($var_symbol.vs)));}
	| string_v_actual[$chain];

after_var_sym_model_b [concat_chain* chain]
	: op_ch_v[$chain]
	| model_string_v[$chain]
	;

after_var_sym_model [concat_chain* chain]
	:
	(
		after_var_sym_model_b[$chain]
	)*
	;
	

model_op returns [std::optional<concat_chain> chain_opt = concat_chain()]
	:
	before_var_sym_model
	{
			$chain_opt->emplace_back(char_str_conc(std::move($before_var_sym_model.value), provider.get_range($before_var_sym_model.ctx)));
	}
	var_sym_model[&*$chain_opt] after_var_sym_model[&*$chain_opt]
	{
		if($var_sym_model.ctx->exception)
			$chain_opt = std::nullopt;
		else
		{
			concatenation_point::clear_concat_chain($chain_opt.value());
			resolve_concat_chain($chain_opt.value());
		}
	}
	;

model_string_ch returns [std::string value]
	: l_sp_ch								{$value = std::move($l_sp_ch.value);}
	| (APOSTROPHE|ATTR) (APOSTROPHE|ATTR)	{$value = "''";};

model_string_ch_v [concat_chain* chain]
	: l_sp_ch_v[$chain]
	| l=(APOSTROPHE|ATTR) r=(APOSTROPHE|ATTR)	{$chain->emplace_back(char_str_conc("''", provider.get_range($l, $r)));};

model_string_ch_v_c [concat_chain* chain]
	:
	(
		model_string_ch_v[$chain]
	)*;

model_string_ch_c returns [std::string value]
	:
	| tmp=model_string_ch_c model_string_ch				{$value = std::move($tmp.value); $value.append($model_string_ch.value);};

string_v_actual [concat_chain* chain]
	: ap1=(APOSTROPHE|ATTR)	model_string_ch_c var_symbol
	{ 
		$chain->emplace_back(char_str_conc("'", provider.get_range($ap1)));
		$chain->emplace_back(char_str_conc(std::move($model_string_ch_c.value), provider.get_range($model_string_ch_c.ctx)));
		$chain->emplace_back(var_sym_conc(std::move($var_symbol.vs)));
	}
	model_string_ch_v_c[$chain] ap2=(APOSTROPHE|ATTR)
	{
		$chain->emplace_back(char_str_conc("'", provider.get_range($ap2)));
		collector.add_hl_symbol(token_info(provider.get_range($ap1,$ap2),hl_scopes::string)); 
	};

model_string_v [concat_chain* chain]
	: ap1=(APOSTROPHE|ATTR)
	{ 
		$chain->emplace_back(char_str_conc("'", provider.get_range($ap1)));
	}
	model_string_ch_v_c[$chain] ap2=(APOSTROPHE|ATTR)
	{
		$chain->emplace_back(char_str_conc("'", provider.get_range($ap2)));
		collector.add_hl_symbol(token_info(provider.get_range($ap1,$ap2),hl_scopes::string)); 
	};
