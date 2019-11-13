parser grammar deferred_operand_rules;

deferred_entry 
	: mac_preproc_c
	| apostrophe
	| comma
	| IGNORED; 

def_string_body
	: string_ch_v
	| IGNORED
	| CONTINUATION;

def_string returns [concat_chain chain]
	: ap1=(APOSTROPHE|ATTR) def_string_body* ap2=(APOSTROPHE|ATTR)	
	{ 
		collector.add_hl_symbol(token_info(provider.get_range($ap1,$ap2),hl_scopes::string)); 
	};

deferred_op_rem returns [remark_list remarks]
	: deferred_entry* remark_o {if($remark_o.value) $remarks.push_back(*$remark_o.value);} 
		(CONTINUATION deferred_entry* remark_o {if($remark_o.value) $remarks.push_back(*$remark_o.value);})*;