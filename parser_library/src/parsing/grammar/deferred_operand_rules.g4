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
	: mac_preproc_c					{$vs = std::move($mac_preproc_c.vs);}
	| apostrophe
	| comma
	| AMPERSAND
	| IGNORED; 

def_string_body
	: string_ch_v
	| IGNORED
	| CONTINUATION;

def_string returns [concat_chain chain]
	: ap1=APOSTROPHE def_string_body* ap2=(APOSTROPHE|ATTR)	
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
	)*;