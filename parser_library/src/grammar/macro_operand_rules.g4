parser grammar macro_operand_rules; 


mac_op returns [operand_ptr op]
	: mac_entry												{$op = std::make_unique<macro_operand>(std::move($mac_entry.chain),provider.get_range($mac_entry.ctx));};

mac_str_ch returns [concat_point_ptr point]
	: common_ch_v									{$point = std::move($common_ch_v.point);}
	| SPACE											{$point = std::make_unique<char_str>(" ");}//here next are for deferred
	| LPAR											{$point = std::make_unique<char_str>("(");}
	| RPAR											{$point = std::make_unique<char_str>(")");}
	| COMMA											{$point = std::make_unique<char_str>(",");};

mac_str_b returns [concat_chain chain]
	:
	| tmp=mac_str_b mac_str_ch						{$tmp.chain.push_back(std::move($mac_str_ch.point)); $chain = std::move($tmp.chain);};

mac_str returns [concat_chain chain]
	: ap1=APOSTROPHE mac_str_b ap2=APOSTROPHE				
	{
		$chain.push_back(std::make_unique<char_str>("'"));
		$chain.insert($chain.end(), std::make_move_iterator($mac_str_b.chain.begin()), std::make_move_iterator($mac_str_b.chain.end()));
		$chain.push_back(std::make_unique<char_str>("'"));
		collector.add_hl_symbol(token_info(provider.get_range($ap1,$ap2),hl_scopes::string)); 
	};

mac_ch returns [concat_chain chain]
	: common_ch_v									{$chain.push_back(std::move($common_ch_v.point));}
	| mac_str										{$chain = std::move($mac_str.chain);}
	| APOSTROPHE									{$chain.push_back(std::make_unique<char_str>("'"));}
	| mac_sublist									{$chain.push_back(std::move($mac_sublist.point));};

mac_ch_c returns [concat_chain chain]
	:
	| tmp=mac_ch_c mac_ch							
	{
		$chain = std::move($tmp.chain);
		$chain.insert($chain.end(), std::make_move_iterator($mac_ch.chain.begin()), std::make_move_iterator($mac_ch.chain.end()));
	};

mac_entry returns [concat_chain chain]
	: mac_ch										{$chain = std::move($mac_ch.chain);}
	| tmp=mac_entry mac_ch							
	{
		$chain = std::move($tmp.chain);
		$chain.insert($chain.end(), std::make_move_iterator($mac_ch.chain.begin()), std::make_move_iterator($mac_ch.chain.end()));
	};

mac_sublist_b returns [concat_chain chain]
	: mac_ch_c										{$chain = std::move($mac_ch_c.chain);}
	| tmp=mac_sublist_b comma mac_ch_c			
	{
		$chain = std::move($tmp.chain);
		$chain.insert($chain.end(), std::make_move_iterator($mac_ch_c.chain.begin()), std::make_move_iterator($mac_ch_c.chain.end()));
	};

mac_sublist returns [concat_point_ptr point]
	: lpar mac_sublist_b rpar						{ $point = std::make_unique<sublist>(std::move($mac_sublist_b.chain)); };