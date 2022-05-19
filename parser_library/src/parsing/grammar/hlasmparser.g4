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

 //starting statement rules
 //rules for identifier, number, remark
parser grammar hlasmparser;
                                                 
import
label_field_rules,
operand_field_rules,
instruction_field_rules,
lookahead_rules,
machine_operand_rules,
assembler_operand_rules,
ca_operand_rules,
macro_operand_rules,
model_operand_rules,
machine_expr_rules,
data_def_rules,
ca_expr_rules,
deferred_operand_rules;

@header
{
	#include "parsing/parser_impl.h"
	#include "expressions/conditional_assembly/ca_operator_unary.h"
	#include "expressions/conditional_assembly/ca_operator_binary.h"
	#include "expressions/conditional_assembly/terms/ca_constant.h"
	#include "expressions/conditional_assembly/terms/ca_expr_list.h"
	#include "expressions/conditional_assembly/terms/ca_function.h"
	#include "expressions/conditional_assembly/terms/ca_string.h"
	#include "expressions/conditional_assembly/terms/ca_symbol.h"
	#include "expressions/conditional_assembly/terms/ca_symbol_attribute.h"
	#include "expressions/conditional_assembly/terms/ca_var_sym.h"
	#include "expressions/mach_expr_term.h"
	#include "expressions/mach_operator.h"
	#include "expressions/data_definition.h"
	#include "semantics/operand_impls.h"

	namespace hlasm_plugin::parser_library::parsing
	{
		using namespace hlasm_plugin::parser_library;
		using namespace hlasm_plugin::parser_library::semantics;
		using namespace hlasm_plugin::parser_library::context;
		using namespace hlasm_plugin::parser_library::checking;
		using namespace hlasm_plugin::parser_library::expressions;
		using namespace hlasm_plugin::parser_library::processing;
	}

	/* disables unreferenced parameter (_localctx) warning */
	#ifdef _MSC_VER
		#pragma warning(push)
		#pragma warning(disable: 4100)
	#endif
}

@members {
	using parser_impl::initialize;
}

@footer 
{
	#ifdef _MSC_VER
		#pragma warning(pop)
	#endif
}

options {
    tokenVocab = lex;
	superClass = parser_impl;
}

/*
program :  ictl? prcs*  program_block  EOF;

ictl: { _input->LT(2)->getText() == "ICTL" }? SPACE ORDSYMBOL SPACE ictl_begin EOF{ analyzer.get_lexer()->set_ictl(); };

ictl_begin: IDENTIFIER ictl_end? 
				{ 
					size_t idx = 0;
					auto val = std::stoi($IDENTIFIER.text, &idx);
					if(idx > 0 || !analyzer.get_lexer()->set_begin(val))
						throw RecognitionException("invalid ICTL parameter value", this, _input, _localctx, $IDENTIFIER); 
					hl_info.lines.push_back(token_info(provider.get_range( $IDENTIFIER),hl_scopes::operand));
				}
			;

ictl_end: COMMA IDENTIFIER ictl_continue 
				{ 
					size_t idx = 0;
					auto val = std::stoi($IDENTIFIER.text, &idx);
					if(idx > 0 || !analyzer.get_lexer()->set_end(val))
						throw RecognitionException("invalid ICTL parameter value", this, _input, _localctx, $IDENTIFIER); 
					hl_info.lines.push_back(token_info(provider.get_range( $COMMA),hl_scopes::operator_symbol));
					hl_info.lines.push_back(token_info(provider.get_range( $IDENTIFIER),hl_scopes::operand));
				}
			;

ictl_continue:  { analyzer.get_lexer()->set_continuation_enabled(false); }

                | COMMA IDENTIFIER 
					{ 
						size_t idx = 0;
						auto val = std::stoi($IDENTIFIER.text, &idx);
						if(idx > 0 || !analyzer.get_lexer()->set_continue(val))
							throw RecognitionException("invalid ICTL parameter value", this, _input, _ctx, $IDENTIFIER);
							
						hl_info.lines.push_back(token_info(provider.get_range( $COMMA),hl_scopes::operator_symbol));
						hl_info.lines.push_back(token_info(provider.get_range( $IDENTIFIER),hl_scopes::operand));
					}
				;


prcs: PROCESS SPACE (assembler_options
									| (ORDSYMBOL { $ORDSYMBOL.text == "OVERRIDE" }? LPAR assembler_options RPAR)) EOF;


assembler_options: assembler_option (COMMA assembler_option)*;

assembler_option: id (LPAR (list | codepage | machine | id)? RPAR)?;

list: list_item (COMMA list_item)*;

list_item: id (LPAR (id | string)+? RPAR)?;

machine: id (MINUS id)? (COMMA id)?;

codepage: id VERTICAL id string;

*/


program : EOF;


first_part 
	: label SPACE instruction 
	{
		_localctx->exception = std::move($instruction.ctx->exception);
	}
	| PROCESS 
	{
		collector.set_label_field(provider.get_range($PROCESS));
		collector.set_instruction_field(
			parse_identifier($PROCESS->getText(),provider.get_range($PROCESS)),
			provider.get_range( $PROCESS));
		collector.add_hl_symbol(token_info(provider.get_range($PROCESS),hl_scopes::instruction));
	};

operand_field_rest
	: (~EOF)*;

lab_instr returns [std::optional<std::string> op_text, range op_range]
	: first_part operand_field_rest EOF
	{
		if (!$first_part.ctx->exception)
		{
			auto op_index = $first_part.stop->getTokenIndex()+1;
			$op_text = _input->getText(misc::Interval(op_index,_input->size()-1));
			$op_range = provider.get_range(_input->get(op_index),_input->get(_input->size()-1));
		}
	}
	| SPACE? EOF
	{
		collector.set_label_field(provider.get_range( _localctx));
		collector.set_instruction_field(provider.get_range( _localctx));
		collector.set_operand_remark_field(provider.get_range( _localctx));
	};

num_ch
	: NUM+;

num returns [self_def_t value]
	: num_ch									{$value = parse_self_def_term("D",$num_ch.ctx->getText(),provider.get_range($num_ch.ctx));};

signed_num_ch
	: MINUS? NUM+;

signed_num returns [self_def_t value]
	: signed_num_ch									{$value = parse_self_def_term("D",$signed_num_ch.ctx->getText(),provider.get_range($signed_num_ch.ctx));};

self_def_term returns [self_def_t value]
	: ORDSYMBOL string							
	{
		collector.add_hl_symbol(token_info(provider.get_range( $ORDSYMBOL),hl_scopes::self_def_type));
		auto opt = $ORDSYMBOL->getText();
		$value = parse_self_def_term(opt, $string.value, provider.get_range($ORDSYMBOL,$string.ctx->getStop()));
	};

id returns [id_index name = nullptr, id_index using_qualifier = nullptr]
	: f=id_no_dot {$name = $f.name;} (dot s=id_no_dot {$name = $s.name; $using_qualifier = $f.name;})?;

id_no_dot returns [id_index name = id_storage::empty_id] locals [std::string buffer]
	: ORDSYMBOL { $buffer = $ORDSYMBOL->getText(); } (l=(IDENTIFIER|NUM|ORDSYMBOL) {$buffer.append($l->getText());})*
	{
		$name = parse_identifier(std::move($buffer),provider.get_range($ORDSYMBOL,$l?$l:$ORDSYMBOL));
	}
	;

remark
	: (DOT|ASTERISK|MINUS|PLUS|LT|GT|COMMA|LPAR|RPAR|SLASH|EQUALS|AMPERSAND|APOSTROPHE|IDENTIFIER|NUM|VERTICAL|ORDSYMBOL|SPACE|ATTR)*;

remark_non_empty
	: (DOT|ASTERISK|MINUS|PLUS|LT|GT|COMMA|LPAR|RPAR|SLASH|EQUALS|AMPERSAND|APOSTROPHE|IDENTIFIER|NUM|VERTICAL|ORDSYMBOL|SPACE|ATTR)+;

remark_o returns [std::optional<range> value]
	: SPACE remark							{$value = provider.get_range( $remark.ctx);}
	| ;

remark_eol returns [std::optional<range> value]
	:
	(
		SPACE
		{
			auto s = _input->LT(1);
		}
		l=(DOT|ASTERISK|MINUS|PLUS|LT|GT|COMMA|LPAR|RPAR|SLASH|EQUALS|AMPERSAND|APOSTROPHE|IDENTIFIER|NUM|VERTICAL|ORDSYMBOL|SPACE|ATTR)*
		{$value = provider.get_range(s, $l);}
	)?
	(
		CONTINUATION
		|
		EOF
	)
	;




	//***** highlighting rules
comma 
	: COMMA {collector.add_hl_symbol(token_info(provider.get_range( $COMMA),hl_scopes::operator_symbol)); };
dot 
	: DOT {collector.add_hl_symbol(token_info(provider.get_range( $DOT),hl_scopes::operator_symbol)); };
apostrophe
	: APOSTROPHE;
attr
	: ATTR {collector.add_hl_symbol(token_info(provider.get_range( $ATTR),hl_scopes::operator_symbol)); };
lpar
	: LPAR { collector.add_hl_symbol(token_info(provider.get_range( $LPAR),hl_scopes::operator_symbol)); };
rpar 
	: RPAR {collector.add_hl_symbol(token_info(provider.get_range( $RPAR),hl_scopes::operator_symbol)); };
ampersand 
	: AMPERSAND { collector.add_hl_symbol(token_info(provider.get_range( $AMPERSAND),hl_scopes::operator_symbol)); };
equals 
	: EQUALS { collector.add_hl_symbol(token_info(provider.get_range( $EQUALS),hl_scopes::operator_symbol)); };
asterisk 
	: ASTERISK {collector.add_hl_symbol(token_info(provider.get_range( $ASTERISK),hl_scopes::operator_symbol)); };
slash 
	: SLASH { collector.add_hl_symbol(token_info(provider.get_range( $SLASH),hl_scopes::operator_symbol)); };
minus 
	: MINUS {collector.add_hl_symbol(token_info(provider.get_range( $MINUS),hl_scopes::operator_symbol)); };
plus 
	: PLUS {collector.add_hl_symbol(token_info(provider.get_range( $PLUS),hl_scopes::operator_symbol)); };
