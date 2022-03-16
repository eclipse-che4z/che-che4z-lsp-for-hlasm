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

op_ch_c returns [std::string value]
	:
	| tmp=op_ch_c op_ch							{$value = std::move($tmp.value); $value.append($op_ch.value);};

op_ch_v returns [concat_point_ptr point]
	: common_ch_v							{$point = std::move($common_ch_v.point);}
	| lpar									{$point = std::make_unique<char_str_conc>("(", provider.get_range($lpar.ctx->getStart())); }
	| rpar									{$point = std::make_unique<char_str_conc>(")", provider.get_range($rpar.ctx->getStart())); }
	| comma									{$point = std::make_unique<char_str_conc>(",", provider.get_range($comma.ctx->getStart())); }
	| ATTR									{$point = std::make_unique<char_str_conc>("'", provider.get_range($ATTR)); };

op_ch_v_c returns [concat_chain chain]
	:
	| tmp=op_ch_v_c op_ch_v						{$tmp.chain.push_back(std::move($op_ch_v.point)); $chain = std::move($tmp.chain);};

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
	: ap1=APOSTROPHE 
	(
		l_string
		{
			$value.append(std::move($l_string.value));
		}
		|
		(APOSTROPHE|ATTR) (APOSTROPHE|ATTR)	{$value.append("''");}
	)*?
	ap2=(APOSTROPHE|ATTR)
	{
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

var_sym_model returns [concat_chain chain]
	: var_symbol										{$chain.push_back(std::make_unique<var_sym_conc>(std::move($var_symbol.vs)));}
	| string_v_actual									{$chain = std::move($string_v_actual.chain);};

after_var_sym_model_b returns [concat_chain chain]
	: op_ch_v											{$chain.push_back(std::move($op_ch_v.point));}
	| model_string_v									{$chain = std::move($model_string_v.chain);};

after_var_sym_model returns [concat_chain chain]
	:
	| tmp=after_var_sym_model after_var_sym_model_b					
	{
		$tmp.chain.insert($tmp.chain.end(), std::make_move_iterator($after_var_sym_model_b.chain.begin()), std::make_move_iterator($after_var_sym_model_b.chain.end()));
		$chain = std::move($tmp.chain);
	};
	

model_op returns [std::optional<concat_chain> chain_opt]
	: before_var_sym_model var_sym_model after_var_sym_model
	{
		if($var_sym_model.ctx->exception)
			$chain_opt = std::nullopt;
		else
		{
			concat_chain chain;
			chain.push_back(std::make_unique<char_str_conc>(std::move($before_var_sym_model.value), provider.get_range($before_var_sym_model.ctx)));
			chain.insert(chain.end(), 
				std::make_move_iterator($var_sym_model.chain.begin()), 
				std::make_move_iterator($var_sym_model.chain.end())
			);
			chain.insert(chain.end(), 
				std::make_move_iterator($after_var_sym_model.chain.begin()), 
				std::make_move_iterator($after_var_sym_model.chain.end())
			);
			$chain_opt = std::move(chain);
		}
	};
	finally
	{if ($chain_opt) concatenation_point::clear_concat_chain(*$chain_opt);}

model_string_ch returns [std::string value]
	: l_sp_ch								{$value = std::move($l_sp_ch.value);}
	| (APOSTROPHE|ATTR) (APOSTROPHE|ATTR)	{$value = "''";};

model_string_ch_v returns [concat_point_ptr point]
	: l_sp_ch_v								{$point = std::move($l_sp_ch_v.point);}
	| l=(APOSTROPHE|ATTR) r=(APOSTROPHE|ATTR)	{$point = std::make_unique<char_str_conc>("''", provider.get_range($l, $r));};

model_string_ch_v_c returns [concat_chain chain]
	:
	| cl=model_string_ch_v_c model_string_ch_v		{$cl.chain.push_back(std::move($model_string_ch_v.point)); $chain = std::move($cl.chain);};

model_string_ch_c returns [std::string value]
	:
	| tmp=model_string_ch_c model_string_ch				{$value = std::move($tmp.value); $value.append($model_string_ch.value);};

string_v_actual returns [concat_chain chain]
	: ap1=(APOSTROPHE|ATTR)	 model_string_ch_c var_symbol model_string_ch_v_c ap2=(APOSTROPHE|ATTR)	
	{ 
		$chain.push_back(std::make_unique<char_str_conc>("'", provider.get_range($ap1)));
		$chain.push_back(std::make_unique<char_str_conc>(std::move($model_string_ch_c.value), provider.get_range($model_string_ch_c.ctx)));
		$chain.push_back(std::make_unique<var_sym_conc>(std::move($var_symbol.vs)));
		$chain.insert($chain.end(), 
			std::make_move_iterator($model_string_ch_v_c.chain.begin()), 
			std::make_move_iterator($model_string_ch_v_c.chain.end())
		);
		$chain.push_back(std::make_unique<char_str_conc>("'", provider.get_range($ap2)));
		collector.add_hl_symbol(token_info(provider.get_range($ap1,$ap2),hl_scopes::string)); 
	};

model_string_v returns [concat_chain chain]
	: ap1=(APOSTROPHE|ATTR) model_string_ch_v_c ap2=(APOSTROPHE|ATTR)
	{ 
		$chain.push_back(std::make_unique<char_str_conc>("'", provider.get_range($ap1)));
		$chain.insert($chain.end(),
			std::make_move_iterator($model_string_ch_v_c.chain.begin()),
			std::make_move_iterator($model_string_ch_v_c.chain.end())
		);
		$chain.push_back(std::make_unique<char_str_conc>("'", provider.get_range($ap2)));
		collector.add_hl_symbol(token_info(provider.get_range($ap1,$ap2),hl_scopes::string)); 
	};