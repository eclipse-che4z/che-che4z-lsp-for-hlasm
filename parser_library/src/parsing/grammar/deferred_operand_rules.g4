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

deferred_entry returns [std::vector<vs_ptr> vs]
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
	| ap1=ATTR
	(
		{!is_previous_attribute_consuming(true, _input->LT(-2))}?
		{disable_ca_string();}
		(
			(APOSTROPHE|ATTR) (APOSTROPHE|ATTR)
			|
			(
				{std::string name;}
				AMPERSAND
				ORDSYMBOL {name += $ORDSYMBOL->getText();}
				(ORDSYMBOL {name += $ORDSYMBOL->getText();}|NUM {name += $NUM->getText();})*
				{
					auto r = provider.get_range($AMPERSAND,_input->LT(-1));
					$vs.push_back(std::make_unique<basic_variable_symbol>(add_id(std::move(name)), std::vector<ca_expr_ptr>(), r));
					collector.add_hl_symbol(token_info(r,hl_scopes::var_symbol));
				}
			)
			|
			l_sp_ch_v
		)*
		ap2=(APOSTROPHE|ATTR)
		{
			collector.add_hl_symbol(token_info(provider.get_range($ap1,$ap2),hl_scopes::string));
		}
		{enable_ca_string();}
		|
		{collector.add_hl_symbol(token_info(provider.get_range($ap1),hl_scopes::operator_symbol)); }
	)
	|
	ap1=APOSTROPHE
	{disable_ca_string();}
	(
		(APOSTROPHE|ATTR) (APOSTROPHE|ATTR)
		|
		(
			{std::string name;}
			AMPERSAND
			ORDSYMBOL {name += $ORDSYMBOL->getText();}
			(ORDSYMBOL {name += $ORDSYMBOL->getText();}|NUM {name += $NUM->getText();})*
			{
				auto r = provider.get_range($AMPERSAND,_input->LT(-1));
				$vs.push_back(std::make_unique<basic_variable_symbol>(add_id(std::move(name)), std::vector<ca_expr_ptr>(), r));
				collector.add_hl_symbol(token_info(r,hl_scopes::var_symbol));
			}
		)
		|
		l_sp_ch_v
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
			ORDSYMBOL
			{
				name += $ORDSYMBOL->getText();
			}
			|
			NUM
			{
				name += $NUM->getText();
			}
		)*
		{
			auto r = provider.get_range($AMPERSAND,_input->LT(-1));
			$vs.push_back(std::make_unique<basic_variable_symbol>(add_id(std::move(name)), std::vector<ca_expr_ptr>(), r));
			collector.add_hl_symbol(token_info(r,hl_scopes::var_symbol));
		}
		|
		LPAR
		|
		AMPERSAND
	)?
	;
	finally
	{enable_ca_string();}

deferred_op_rem returns [remark_list remarks, std::vector<vs_ptr> var_list]
	:
	(
		deferred_entry
		{
			for (auto&v : $deferred_entry.vs)
				$var_list.push_back(std::move(v));
		}
	)*
	{enable_continuation();}
	remark_o {if($remark_o.value) $remarks.push_back(*$remark_o.value);}
	(
		CONTINUATION
		{disable_continuation();}
		(
			deferred_entry
			{
				for (auto&v : $deferred_entry.vs)
					$var_list.push_back(std::move(v));
			}
		)*
		{enable_continuation();}
		remark_o {if($remark_o.value) $remarks.push_back(*$remark_o.value);}
	)*
	;
	finally
	{disable_continuation();}