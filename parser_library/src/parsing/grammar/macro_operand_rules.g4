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

 //rules for macro operand
parser grammar macro_operand_rules; 


mac_op[int* paren_count] returns [operand_ptr op]
	: mac_preproc[$paren_count]
	{
		$op = std::make_unique<macro_operand_string>($mac_preproc.ctx->getText(),provider.get_range($mac_preproc.ctx));
	};

mac_op_o returns [operand_ptr op] 
	: mac_entry[true]?
	{
		if($mac_entry.ctx)
			$op = std::make_unique<macro_operand_chain>(std::move($mac_entry.chain),provider.get_range($mac_entry.ctx));
		else
			$op = std::make_unique<semantics::empty_operand>(provider.original_range);
	};

macro_ops returns [operand_list list] 
	: mac_op_o  {$list.push_back(std::move($mac_op_o.op));} (comma mac_op_o {$list.push_back(std::move($mac_op_o.op));})* EOF;

mac_preproc[int* paren_count]
	:
	(
		ASTERISK
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
		| AMPERSAND
		(
			ORDSYMBOL (ORDSYMBOL|NUM)*
			|
			LPAR {++*$paren_count;}
			|
			AMPERSAND
		)
		|
		LPAR {++*$paren_count;}
		|
		RPAR {--*$paren_count;}
		|
		APOSTROPHE
		(~(APOSTROPHE|ATTR|CONTINUATION))*
		(APOSTROPHE|ATTR)
		|
		ATTR
		(
			{!is_previous_attribute_consuming(*$paren_count == 0, _input->LT(-2))}?
			(~(APOSTROPHE|ATTR|CONTINUATION))*
			(APOSTROPHE|ATTR)
		)?
	)+
	;

mac_entry [bool top_level = true] returns [concat_chain chain]
	: 
	(
		asterisk		{$chain.push_back(std::make_unique<char_str_conc>("*", provider.get_range($asterisk.ctx)));}
		| minus			{$chain.push_back(std::make_unique<char_str_conc>("-", provider.get_range($minus.ctx)));}
		| plus			{$chain.push_back(std::make_unique<char_str_conc>("+", provider.get_range($plus.ctx)));}
		| LT			{$chain.push_back(std::make_unique<char_str_conc>("<", provider.get_range($LT)));}
		| GT			{$chain.push_back(std::make_unique<char_str_conc>(">", provider.get_range($GT)));}
		| slash			{$chain.push_back(std::make_unique<char_str_conc>("/", provider.get_range($slash.ctx)));}
		| equals		{$chain.push_back(std::make_unique<equals_conc>());}
		| VERTICAL		{$chain.push_back(std::make_unique<char_str_conc>("|", provider.get_range($VERTICAL)));}
		| IDENTIFIER
		{
			auto r = provider.get_range($IDENTIFIER);
			$chain.push_back(std::make_unique<char_str_conc>($IDENTIFIER.text, r));
			collector.add_hl_symbol(token_info(r, hl_scopes::operand));
		}
		| NUM
		{
			auto r = provider.get_range($NUM);
			$chain.push_back(std::make_unique<char_str_conc>($NUM.text, r));
			collector.add_hl_symbol(token_info(r, hl_scopes::operand));
		}
		| ORDSYMBOL
		{
			auto r = provider.get_range($ORDSYMBOL);
			$chain.push_back(std::make_unique<char_str_conc>($ORDSYMBOL.text, r));
			collector.add_hl_symbol(token_info(r, hl_scopes::operand));
		}
		| dot			{$chain.push_back(std::make_unique<dot_conc>());}
		| AMPERSAND
		(
			vs_id tmp=subscript
			{
				auto id = $vs_id.name;
				auto r = provider.get_range( $AMPERSAND,$tmp.ctx->getStop());
				$chain.push_back(std::make_unique<var_sym_conc>(std::make_unique<basic_variable_symbol>(id, std::move($tmp.value), r)));
				collector.add_hl_symbol(token_info(provider.get_range($AMPERSAND, $vs_id.ctx->getStop()),hl_scopes::var_symbol));
			}
			|
			lpar (clc=created_set_body)? rpar subscript
			{
				$chain.push_back(std::make_unique<var_sym_conc>(std::make_unique<created_variable_symbol>($clc.ctx ? std::move($clc.concat_list) : concat_chain{},std::move($subscript.value),provider.get_range($AMPERSAND,$subscript.ctx->getStop()))));
				collector.add_hl_symbol(token_info(provider.get_range($AMPERSAND),hl_scopes::var_symbol));
			}
			|
			AMPERSAND
			{
				$chain.push_back(std::make_unique<char_str_conc>("&&", provider.get_range(_input->LT(-2),_input->LT(-1))));
			}
		)
		|
		lpar
		{
			std::vector<concat_chain> sublist;
			bool pending_empty = false;
		}
		(
			comma
			{
				sublist.emplace_back();
				pending_empty = true;
			}
		)*
		(
			mac_entry[false]
			{
				sublist.push_back(std::move($mac_entry.chain));
				pending_empty = false;
			}			
			(
				comma
				(
					comma
					{
						sublist.emplace_back();
					}
				)*
				{
					pending_empty = true;
				}
				(
					mac_entry[false]
					{
						sublist.push_back(std::move($mac_entry.chain));
						pending_empty = false;
					}
				)?
			)*
		)?
		{
			if (pending_empty)
			{
				sublist.emplace_back();
			}
			$chain.push_back(std::make_unique<sublist_conc>(std::move(sublist)));
		}
		rpar
		|
		ap1=APOSTROPHE
		{
			$chain.push_back(std::make_unique<char_str_conc>("'", provider.get_range($ap1)));
		}
		(
			string_ch_v
			{
				if (auto& p = $string_ch_v.point; p)
					$chain.push_back(std::move(p));
			}
		)*?
		ap2=(APOSTROPHE|ATTR)
		{
			$chain.push_back(std::make_unique<char_str_conc>("'", provider.get_range($ap2)));
			collector.add_hl_symbol(token_info(provider.get_range($ap1,$ap2),hl_scopes::string));
		}
		|
		ap1=ATTR
		{
			$chain.push_back(std::make_unique<char_str_conc>("'", provider.get_range($ap1)));
			bool attribute = true;
		}
		(
			{!is_previous_attribute_consuming($top_level, _input->LT(-2))}?
			(
				string_ch_v
				{
					if (auto& p = $string_ch_v.point; p)
						$chain.push_back(std::move(p));
				}
			)*?
			ap2=(APOSTROPHE|ATTR)
			{
				$chain.push_back(std::make_unique<char_str_conc>("'", provider.get_range($ap2)));
				collector.add_hl_symbol(token_info(provider.get_range($ap1,$ap2),hl_scopes::string));
				attribute = false;
			}
		)?
		{
			if (attribute)
				collector.add_hl_symbol(token_info(provider.get_range($ap1),hl_scopes::operator_symbol));
		}
	)+
	;
