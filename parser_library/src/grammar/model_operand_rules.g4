parser grammar model_operand_rules; 


op_ch returns [std::string value]
	: common_ch								{$value = std::move($common_ch.value);}
	| lpar									{$value = "("; }
	| rpar									{$value = ")"; }
	| comma									{$value = ","; }
	| SPACE									{$value = " "; }
	| APOSTROPHE							{$value = "'"; }
	| DAPOSTROPHE							{$value = "\""; };

op_ch_c returns [std::string value]
	:
	| tmp=op_ch_c op_ch							{$value = std::move($tmp.value); $value.append($op_ch.value);};

op_ch_v returns [concat_point_ptr point]
	: common_ch_v							{$point = std::move($common_ch_v.point);}
	| lpar									{$point = std::make_unique<char_str>("("); }
	| rpar									{$point = std::make_unique<char_str>(")"); }
	| comma									{$point = std::make_unique<char_str>(","); }
	| SPACE									{$point = std::make_unique<char_str>(" "); }
	| APOSTROPHE							{$point = std::make_unique<char_str>("'"); }
	| DAPOSTROPHE							{$point = std::make_unique<char_str>("\""); };

op_ch_v_c returns [concat_chain chain]
	:
	| tmp=op_ch_v_c op_ch_v						{$tmp.chain.push_back(std::move($op_ch_v.point)); $chain = std::move($tmp.chain);};

model_op returns [concat_chain chain]
	: op_ch_c var_symbol cl=op_ch_v_c
	{
		$chain.push_back(std::make_unique<char_str>(std::move($op_ch_c.value)));
		$chain.push_back(std::move($var_symbol.vs));
		$chain.insert($chain.end(), 
			std::make_move_iterator($op_ch_v_c.chain.begin()), 
			std::make_move_iterator($op_ch_v_c.chain.end())
		);
	};

