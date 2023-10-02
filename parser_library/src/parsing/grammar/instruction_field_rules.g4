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

 //rules for instruction field
parser grammar instruction_field_rules;

instruction returns [id_index instr] locals [concat_chain chain, std::string instr_text, bool has_vars = false]
	: f=ORDSYMBOL												{$instr_text += $f->getText();$chain.emplace_back(char_str_conc($f->getText(), provider.get_range($f)));}
	(
		{
			collector.add_hl_symbol(token_info(provider.get_range($f),hl_scopes::instruction));
			auto instr_id = parse_identifier($f->getText(),provider.get_range($f));
			collector.set_instruction_field(
				instr_id,
				provider.get_range($f));
		}
		|
		( ASTERISK												{$instr_text += "*";$chain.emplace_back(char_str_conc("*", provider.get_range($ASTERISK)));}
		| MINUS													{$instr_text += "-";$chain.emplace_back(char_str_conc("-", provider.get_range($MINUS)));}
		| PLUS													{$instr_text += "+";$chain.emplace_back(char_str_conc("+", provider.get_range($PLUS)));}
		| LT													{$instr_text += "<";$chain.emplace_back(char_str_conc("<", provider.get_range($LT)));}
		| GT													{$instr_text += ">";$chain.emplace_back(char_str_conc(">", provider.get_range($GT)));}
		| SLASH													{$instr_text += "/";$chain.emplace_back(char_str_conc("/", provider.get_range($SLASH)));}
		| EQUALS												{$instr_text += "=";$chain.emplace_back(equals_conc(provider.get_range($EQUALS)));}
		| VERTICAL												{$instr_text += "|";$chain.emplace_back(char_str_conc("|", provider.get_range($VERTICAL)));}
		| IDENTIFIER											{$instr_text += $IDENTIFIER->getText();$chain.emplace_back(char_str_conc($IDENTIFIER->getText(), provider.get_range($IDENTIFIER)));}
		| NUM													{$instr_text += $NUM->getText();$chain.emplace_back(char_str_conc($NUM->getText(), provider.get_range($NUM)));}
		| ORDSYMBOL												{$instr_text += $ORDSYMBOL->getText();$chain.emplace_back(char_str_conc($ORDSYMBOL->getText(), provider.get_range($ORDSYMBOL)));}
		| DOT													{$instr_text += ".";$chain.emplace_back(dot_conc(provider.get_range($DOT)));}
		| l=AMPERSAND
		(
			r=AMPERSAND											{$instr_text += "&&";$chain.emplace_back(char_str_conc("&&", provider.get_range($l,$r)));}
			|
			var_symbol_base[$l]									{$has_vars = true; $chain.emplace_back(var_sym_conc(std::move($var_symbol_base.vs)));}
		)
		| COMMA													{$instr_text += ",";$chain.emplace_back(char_str_conc(",", provider.get_range($COMMA)));}
		| LPAR													{$instr_text += "(";$chain.emplace_back(char_str_conc("(", provider.get_range($LPAR)));}
		| RPAR													{$instr_text += ")";$chain.emplace_back(char_str_conc(")", provider.get_range($RPAR)));}
		)+
		{
			auto r = provider.get_range($f,_input->LT(-1));
			if ($has_vars){
				for(const auto& point : $chain)
				{
					if(!std::holds_alternative<semantics::char_str_conc>(point.value))
						continue;
					collector.add_hl_symbol(token_info(std::get<semantics::char_str_conc>(point.value).conc_range, hl_scopes::instruction));
				}

				collector.set_instruction_field(std::move($chain), r);
			}
			else {
				collector.add_hl_symbol(token_info(r, hl_scopes::instruction));
				collector.set_instruction_field(parse_identifier(std::move($instr_text),r), provider.get_range($f,_input->LT(-1)));
			}
		}
	)
	| v=AMPERSAND var_symbol_base[$v]							{$chain.emplace_back(var_sym_conc(std::move($var_symbol_base.vs)));}
	(
		( ASTERISK												{$chain.emplace_back(char_str_conc("*", provider.get_range($ASTERISK)));}
		| MINUS													{$chain.emplace_back(char_str_conc("-", provider.get_range($MINUS)));}
		| PLUS													{$chain.emplace_back(char_str_conc("+", provider.get_range($PLUS)));}
		| LT													{$chain.emplace_back(char_str_conc("<", provider.get_range($LT)));}
		| GT													{$chain.emplace_back(char_str_conc(">", provider.get_range($GT)));}
		| SLASH													{$chain.emplace_back(char_str_conc("/", provider.get_range($SLASH)));}
		| EQUALS												{$chain.emplace_back(equals_conc(provider.get_range($EQUALS)));}
		| VERTICAL												{$chain.emplace_back(char_str_conc("|", provider.get_range($VERTICAL)));}
		| IDENTIFIER											{$chain.emplace_back(char_str_conc($IDENTIFIER->getText(), provider.get_range($IDENTIFIER)));}
		| NUM													{$chain.emplace_back(char_str_conc($NUM->getText(), provider.get_range($NUM)));}
		| ORDSYMBOL												{$chain.emplace_back(char_str_conc($ORDSYMBOL->getText(), provider.get_range($ORDSYMBOL)));}
		| DOT													{$chain.emplace_back(dot_conc(provider.get_range($DOT)));}
		| l=AMPERSAND
		(
			r=AMPERSAND											{$chain.emplace_back(char_str_conc("&&", provider.get_range($l,$r)));}
			|
			var_symbol_base[$l]									{$chain.emplace_back(var_sym_conc(std::move($var_symbol_base.vs)));}
		)
		| COMMA													{$instr_text += ",";$chain.emplace_back(char_str_conc(",", provider.get_range($COMMA)));}
		| LPAR													{$instr_text += "(";$chain.emplace_back(char_str_conc("(", provider.get_range($LPAR)));}
		| RPAR													{$instr_text += ")";$chain.emplace_back(char_str_conc(")", provider.get_range($RPAR)));}
		)*
		{
			auto r = provider.get_range($v,_input->LT(-1));
			for(const auto& point : $chain)
			{
				if(!std::holds_alternative<semantics::char_str_conc>(point.value))
					continue;
				collector.add_hl_symbol(token_info(std::get<semantics::char_str_conc>(point.value).conc_range, hl_scopes::instruction));
			}

			collector.set_instruction_field(std::move($chain), r);
		}
	)
	| b=~(ORDSYMBOL|AMPERSAND|SPACE|CONTINUATION|EOF) ~(SPACE)*
	{
		collector.add_hl_symbol(token_info(provider.get_range($b,_input->LT(-1)),hl_scopes::instruction));
		collector.set_instruction_field(add_id(get_context_text($ctx)), provider.get_range($b, _input->LT(-1)));
	};
