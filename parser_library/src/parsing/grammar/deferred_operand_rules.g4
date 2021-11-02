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

 //rules for deferred operand
parser grammar deferred_operand_rules;

deferred_entry returns [vs_ptr vs]
	: asterisk
	| minus
	| plus
	| LT
	| GT
	| slash
	| equals
	| VERTICAL
	| IDENTIFIER											{collector.add_hl_symbol(token_info(provider.get_range($IDENTIFIER), hl_scopes::operand));}
	| NUM													{collector.add_hl_symbol(token_info(provider.get_range($NUM), hl_scopes::operand));}
	| ORDSYMBOL												{collector.add_hl_symbol(token_info(provider.get_range($ORDSYMBOL), hl_scopes::operand));}
	| dot
	| lpar
	| rpar
	| attr
	|
	ap1=APOSTROPHE
	{disable_ca_string();}
	(
		(APOSTROPHE|ATTR) (APOSTROPHE|ATTR)
		|
		l_sp_ch_v+
		|
		(CONTINUATION IGNORED*)
	)*
	{enable_ca_string();}
	ap2=(APOSTROPHE|ATTR)
	{
		collector.add_hl_symbol(token_info(provider.get_range($ap1,$ap2),hl_scopes::string));
	}
	| comma
	| AMPERSAND
	(
		ORDSYMBOL
		{
			auto name = $ORDSYMBOL->getText();
		}
		(
			CONTINUATION IGNORED* ORDSYMBOL
			{
				name += $ORDSYMBOL->getText();
			}
		)*
		{
			auto r = provider.get_range($AMPERSAND,$ORDSYMBOL);
			$vs = std::make_unique<basic_variable_symbol>(hlasm_ctx->ids().add(std::move(name)), std::vector<ca_expr_ptr>(), r);
			collector.add_hl_symbol(token_info(r,hl_scopes::var_symbol));
		}
		|
		LPAR
		|
		AMPERSAND
	)?
	| IGNORED; 
	finally
	{enable_ca_string();}

def_string_body
	: string_ch_v
	| IGNORED
	| CONTINUATION;

def_string returns [concat_chain chain]
	: ap1=APOSTROPHE def_string_body*? ap2=(APOSTROPHE|ATTR)
	{ 
		collector.add_hl_symbol(token_info(provider.get_range($ap1,$ap2),hl_scopes::string)); 
	};
	finally
	{concatenation_point::clear_concat_chain($chain);}

deferred_op_rem returns [remark_list remarks, std::vector<vs_ptr> var_list]
	: (deferred_entry {if ($deferred_entry.vs) $var_list.push_back(std::move($deferred_entry.vs));})* 
	remark_o {if($remark_o.value) $remarks.push_back(*$remark_o.value);} 
	(CONTINUATION 
	(deferred_entry {if ($deferred_entry.vs) $var_list.push_back(std::move($deferred_entry.vs));})* 
	remark_o {if($remark_o.value) $remarks.push_back(*$remark_o.value);} 
	)* IGNORED*;