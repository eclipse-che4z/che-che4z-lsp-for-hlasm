parser grammar deferred_operand_rules;

deferred_entry 
	: asterisk
	| minus
	| plus
	| LT
	| GT
	| slash
	| equals_
	| AMPERSAND AMPERSAND
	| VERTICAL
	| IDENTIFIER											{collector.add_hl_symbol(token_info(provider.get_range($IDENTIFIER), hl_scopes::operand));}
	| ORDSYMBOL												{collector.add_hl_symbol(token_info(provider.get_range($ORDSYMBOL), hl_scopes::operand));}
	| dot_									
	| var_symbol
	| lpar
	| rpar
	| comma									
	| attr							
	| def_string
	| apostrophe
	| IGNORED; 

def_string_body
	: string_ch_v
	| IGNORED
	| CONTINUATION;

def_string returns [concat_chain chain]
	: ap1=APOSTROPHE def_string_body+ ap2=(APOSTROPHE|ATTR)	
	{ 
		collector.add_hl_symbol(token_info(provider.get_range($ap1,$ap2),hl_scopes::string)); 
	};

deferred_op_rem returns [remark_list remarks]
	: tmp=deferred_op_rem CONTINUATION deferred_entry* remark_o		{$remarks = std::move($tmp.remarks); if($remark_o.value) $remarks.push_back(*$remark_o.value);}
	| deferred_entry* remark_o										{if($remark_o.value) $remarks.push_back(*$remark_o.value);};