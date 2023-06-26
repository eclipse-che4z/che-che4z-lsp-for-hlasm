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

 //rules for label field
parser grammar label_field_rules;

label locals [concat_chain chain, std::string buffer, bool has_variables = false]
	:
	(
		t=(
			DOT|
			ASTERISK|
			MINUS|
			PLUS|
			LT|
			GT|
			COMMA|
			LPAR|
			RPAR|
			SLASH|
			EQUALS|
			IDENTIFIER|
			OR|
			AND|
			EQ|
			LE|
			LTx|
			GTx|
			GE|
			NE|
			VERTICAL|
			NUM|
			ORDSYMBOL
		) {add_label_component($t, $chain, $buffer, $has_variables);}
		|
		(
			var_symbol {add_label_component(std::move($var_symbol.vs), $chain, $buffer, $has_variables);}
			|
			AMPERSAND {add_label_component($AMPERSAND, $chain, $buffer, $has_variables);} AMPERSAND {add_label_component($AMPERSAND, $chain, $buffer, $has_variables);}
		)
		|
		label_string[&$chain, &$buffer, &$has_variables, [](const auto*t){if(!t)return true;auto text = t->getText(); return text == "C" || text == "c";}(_input->LT(-1))]
	)+
	{
		$stop = _input->LT(-1);
		if ($has_variables) {
			concatenation_point::clear_concat_chain($chain);
			for (const auto& c: $chain) {
				if (!std::holds_alternative<char_str_conc>(c.value))
					continue;
				collector.add_hl_symbol(token_info(std::get<char_str_conc>(c.value).conc_range, hl_scopes::label));
			}
			collector.set_label_field(std::move($chain),provider.get_range($ctx));
		}
		else {
			auto r = provider.get_range($ctx);
			auto& label = $buffer;

			static constexpr auto chars_alphanumeric = utils::create_truth_table("\$_#@abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789");
			static constexpr auto chars_alpha = utils::create_truth_table("\$_#@abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ");

			if (label.size() > 1 && label.size() <= 63 && label.front() == '.' && chars_alpha[(unsigned char)label[1]] && utils::find_mismatch(std::string_view(label).substr(2), chars_alphanumeric) == std::string::npos) {
				collector.add_hl_symbol(token_info(r, hl_scopes::seq_symbol));
				label.erase(0, 1);

				collector.set_label_field({parse_identifier(std::move(label),r),r},r);
			}
			else
			{
				collector.add_hl_symbol(token_info(r, hl_scopes::label));
				auto id = add_id(label);
				collector.set_label_field(id,std::move(label),$ctx,r);
			}
		}
	}
	;

label_string [concat_chain* chain, std::string* buffer, bool* has_variables, bool allow_space]
	:
	ap1=(APOSTROPHE | ATTR) {add_label_component($ap1, *$chain, *$buffer, *$has_variables);}
	(
		{!$allow_space && _input->LA(1)==SPACE || is_attribute_consuming(_input->LT(-2))}?
		|
		(
			t=(
				SPACE|
				DOT|
				ASTERISK|
				MINUS|
				PLUS|
				LT|
				GT|
				COMMA|
				LPAR|
				RPAR|
				SLASH|
				EQUALS|
				ORDSYMBOL|
				IDENTIFIER|
				OR|
				AND|
				EQ|
				LE|
				LTx|
				GTx|
				GE|
				NE|
				VERTICAL|
				NUM
			) {add_label_component($t, *$chain, *$buffer, *$has_variables);}
			|
			(
				var_symbol {add_label_component(std::move($var_symbol.vs), *$chain, *$buffer, *$has_variables);}
				|
				AMPERSAND {add_label_component($AMPERSAND, *$chain, *$buffer, *$has_variables);} AMPERSAND {add_label_component($AMPERSAND, *$chain, *$buffer, *$has_variables);}
			)
		)*
		ap2=(APOSTROPHE | ATTR) {add_label_component($ap2, *$chain, *$buffer, *$has_variables);}
		(
			label_string[$chain,$buffer,$has_variables,$allow_space]
			|
			{_input->LA(1)!=APOSTROPHE && _input->LA(1)!=ATTR}?
		)
	)
	;

l_sp_ch returns [std::string value]
	: ASTERISK												{$value = "*";}
	| MINUS													{$value = "-";}
	| PLUS													{$value = "+";}
	| LT													{$value = "<";}
	| GT													{$value = ">";}
	| SLASH													{$value = "/";}
	| EQUALS												{$value = "=";}
	| AMPERSAND AMPERSAND									{$value = "&&";}
	| VERTICAL												{$value = "|";}
	| IDENTIFIER											{$value = $IDENTIFIER->getText();}
	| NUM													{$value = $NUM->getText();}
	| ORDSYMBOL												{$value = $ORDSYMBOL->getText();}
	| DOT													{$value = ".";}
	| COMMA													{$value = ",";}
	| LPAR													{$value = "(";}
	| RPAR													{$value = ")";}
	| SPACE													{$value = $SPACE->getText();}
	;

l_sp_ch_v [concat_chain* chain]
	: ASTERISK														{$chain->emplace_back(char_str_conc("*", provider.get_range($ASTERISK)));}
	| MINUS															{$chain->emplace_back(char_str_conc("-", provider.get_range($MINUS)));}
	| PLUS															{$chain->emplace_back(char_str_conc("+", provider.get_range($PLUS)));}
	| LT															{$chain->emplace_back(char_str_conc("<", provider.get_range($LT)));}
	| GT															{$chain->emplace_back(char_str_conc(">", provider.get_range($GT)));}
	| SLASH															{$chain->emplace_back(char_str_conc("/", provider.get_range($SLASH)));}
	| EQUALS														{$chain->emplace_back(equals_conc(provider.get_range($EQUALS)));}
	| VERTICAL														{$chain->emplace_back(char_str_conc("|", provider.get_range($VERTICAL)));}
	| IDENTIFIER													{$chain->emplace_back(char_str_conc($IDENTIFIER->getText(), provider.get_range($IDENTIFIER)));}
	| NUM															{$chain->emplace_back(char_str_conc($NUM->getText(), provider.get_range($NUM)));}
	| ORDSYMBOL														{$chain->emplace_back(char_str_conc($ORDSYMBOL->getText(), provider.get_range($ORDSYMBOL)));}
	| DOT															{$chain->emplace_back(dot_conc(provider.get_range($DOT)));}
	|
	(
		l=AMPERSAND r=AMPERSAND										{$chain->emplace_back(char_str_conc("&&", provider.get_range($l,$r)));}
		|
		var_symbol													{$chain->emplace_back(var_sym_conc(std::move($var_symbol.vs)));}
	)
	| COMMA															{$chain->emplace_back(char_str_conc(",", provider.get_range($COMMA)));}
	| LPAR															{$chain->emplace_back(char_str_conc("(", provider.get_range($LPAR)));}
	| RPAR															{$chain->emplace_back(char_str_conc(")", provider.get_range($RPAR)));}
	| SPACE															{$chain->emplace_back(char_str_conc($SPACE->getText(), provider.get_range($SPACE)));}
	;
