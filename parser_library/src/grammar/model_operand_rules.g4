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
	| lpar									{$point = std::make_unique<char_str>("("); }
	| rpar									{$point = std::make_unique<char_str>(")"); }
	| comma									{$point = std::make_unique<char_str>(","); }
	| ATTR									{$point = std::make_unique<char_str>("'"); };

op_ch_v_c returns [concat_chain chain]
	:
	| tmp=op_ch_v_c op_ch_v						{$tmp.chain.push_back(std::move($op_ch_v.point)); $chain = std::move($tmp.chain);};


before_var_sym_model_b returns [std::string value]
	: op_ch												{$value = std::move($op_ch.value);}
	| string											{$value = std::move($string.value);};

before_var_sym_model returns [std::string value]
	: 
	| tmp=before_var_sym_model before_var_sym_model_b	{$tmp.value.append(std::move($before_var_sym_model_b.value)); $value = std::move($tmp.value);};

var_sym_model returns [concat_chain chain]
	: var_symbol										{$chain.push_back(std::move($var_symbol.vs));}
	| string_v_actual									{$chain = std::move($string_v_actual.chain);};

after_var_sym_model_b returns [concat_chain chain]
	: op_ch_v											{$chain.push_back(std::move($op_ch_v.point));}
	| string_v											{$chain = std::move($string_v.chain);};

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
			chain.push_back(std::make_unique<char_str>(std::move($before_var_sym_model.value)));
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

string_v_actual returns [concat_chain chain]
	: ap1=APOSTROPHE string_ch_c var_symbol string_ch_v_c ap2=(APOSTROPHE|ATTR)	
	{ 
		$chain.push_back(std::make_unique<char_str>("'"));
		$chain.push_back(std::make_unique<char_str>(std::move($string_ch_c.value)));
		$chain.push_back(std::move($var_symbol.vs));
		$chain.insert($chain.end(), 
			std::make_move_iterator($string_ch_v_c.chain.begin()), 
			std::make_move_iterator($string_ch_v_c.chain.end())
		);
		$chain.push_back(std::make_unique<char_str>("'"));
		collector.add_hl_symbol(token_info(provider.get_range($ap1,$ap2),hl_scopes::string)); 
	};
