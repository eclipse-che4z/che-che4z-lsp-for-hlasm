parser grammar machine_expr_rules; 

mach_expr returns [mach_expr_ptr m_e]
	: l=mach_expr plus r=mach_expr_s
	{
		$m_e = std::make_unique<mach_expr_binary<add>>(std::move($l.m_e), std::move($r.m_e), provider.get_range( $l.ctx->getStart(), $r.ctx->getStop()));
	}
	| l=mach_expr minus r=mach_expr_s
	{
		$m_e = std::make_unique<mach_expr_binary<sub>>(std::move($l.m_e), std::move($r.m_e), provider.get_range( $l.ctx->getStart(), $r.ctx->getStop()));
	}
	| mach_expr_s
	{
		$m_e = std::move($mach_expr_s.m_e);
	};

mach_expr_s returns [mach_expr_ptr m_e]
	: mach_term_c
	{
		$m_e = std::move($mach_term_c.m_e);
	}
	| l=mach_expr_s slash r=mach_term_c 
	{
		$m_e = std::make_unique<mach_expr_binary<div>>(std::move($l.m_e), std::move($r.m_e), provider.get_range( $l.ctx->getStart(), $r.ctx->getStop()));
	}
	| l=mach_expr_s asterisk r=mach_term_c
	{
		$m_e = std::make_unique<mach_expr_binary<mul>>(std::move($l.m_e), std::move($r.m_e), provider.get_range( $l.ctx->getStart(), $r.ctx->getStop()));
	};

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
	| asterisk								
	{ 
		$m_e = std::make_unique<mach_expr_location_counter>( provider.get_range( $asterisk.ctx));
	}
	| {!is_self_def()}? data_attribute		
	{
		$m_e = std::make_unique<mach_expr_constant>(0, provider.get_range( $data_attribute.ctx));
	}
	| id
	{
		$m_e = std::make_unique<mach_expr_symbol>($id.name, provider.get_range( $id.ctx));
	}
	| num
	{
		$m_e =  std::make_unique<mach_expr_constant>($num.value, provider.get_range( $num.ctx));
	}
	| { is_self_def()}? self_def_term
	{
		$m_e = std::make_unique<mach_expr_constant>($self_def_term.value, provider.get_range( $self_def_term.ctx));
	}
	| equals_ data_def
	{
		$m_e = std::make_unique<mach_expr_constant>(0, provider.get_range( $equals_.ctx->getStart(), $data_def.ctx->getStop()));
	};


data_attribute // returns [expr_ptr e]
	: ORDSYMBOL apostrophe equals_ data_def				
	| ORDSYMBOL apostrophe string				
	| ORDSYMBOL apostrophe var_symbol		
	| ORDSYMBOL apostrophe id;


string_ch returns [std::string value]
	: l_sp_ch							{$value = std::move($l_sp_ch.value);}
	| APOSTROPHE APOSTROPHE				{$value = "'";};

string_ch_c returns [std::string value]
	:
	| tmp=string_ch_c string_ch				{$value = std::move($tmp.value); $value.append($string_ch.value);};

string_ch_v returns [concat_point_ptr point]
	: l_sp_ch_v							{$point = std::move($l_sp_ch_v.point);}
	| APOSTROPHE APOSTROPHE				{$point = std::make_unique<char_str>("'");};

string_ch_v_c returns [concat_chain chain]
	:
	| cl=string_ch_v_c string_ch_v		{$cl.chain.push_back(std::move($string_ch_v.point)); $chain = std::move($cl.chain);};



string returns [std::string value]
	: ap1=APOSTROPHE string_ch_c ap2=APOSTROPHE	
	{ 
		$value.append("'");
		$value.append(std::move($string_ch_c.value));
		$value.append("'"); 
		collector.add_hl_symbol(token_info(provider.get_range($ap1,$ap2),hl_scopes::string)); 
	};
