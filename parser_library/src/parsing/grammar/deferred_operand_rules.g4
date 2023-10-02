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
	:
	( asterisk
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
	| comma
	)
	| ap1=ATTR
	(
		{!is_attribute_consuming(_input->LT(-2))}?
		{disable_ca_string();}
		(
			(APOSTROPHE|ATTR) (APOSTROPHE|ATTR)
			|
			( AMPERSAND
			(
				AMPERSAND
				|
				var_symbol_base[$AMPERSAND]							{$vs.push_back(std::move($var_symbol_base.vs));}
			)
			| ASTERISK
			| MINUS
			| PLUS
			| LT
			| GT
			| SLASH
			| EQUALS
			| VERTICAL
			| IDENTIFIER
			| NUM
			| ORDSYMBOL
			| DOT
			| COMMA
			| LPAR
			| RPAR
			| SPACE
			)
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
		( AMPERSAND
		(
			AMPERSAND
			|
			var_symbol_base[$AMPERSAND]							{$vs.push_back(std::move($var_symbol_base.vs));}
		)
		| ASTERISK
		| MINUS
		| PLUS
		| LT
		| GT
		| SLASH
		| EQUALS
		| VERTICAL
		| IDENTIFIER
		| NUM
		| ORDSYMBOL
		| DOT
		| COMMA
		| LPAR
		| RPAR
		| SPACE
		)
	)*
	{enable_ca_string();}
	ap2=(APOSTROPHE|ATTR)
	{
		collector.add_hl_symbol(token_info(provider.get_range($ap1,$ap2),hl_scopes::string));
	}
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