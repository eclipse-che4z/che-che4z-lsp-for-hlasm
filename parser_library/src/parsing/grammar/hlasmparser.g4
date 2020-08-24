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

@members 
{
	#define last_text() _input->LT(-1)->getText()
	#define text(token) token->getText()
}

/*
program :  ictl? prcs*  program_block  EOF;

ictl: { _input->LT(2)->getText() == "ICTL" }? SPACE ORDSYMBOL SPACE ictl_begin EOLLN{ analyzer.get_lexer()->set_ictl(); };

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
									| (ORDSYMBOL { $ORDSYMBOL.text == "OVERRIDE" }? LPAR assembler_options RPAR)) EOLLN;


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
		collector.add_hl_symbol(token_info(provider.get_range($instruction.ctx),hl_scopes::instruction));
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
	: ~EOLLN*;

lab_instr returns [std::optional<std::string> op_text, range op_range]
	: first_part {enable_hidden();} operand_field_rest {disable_hidden();} 
	{
		ctx->set_source_indices(statement_start().file_offset, statement_end().file_offset, statement_end().file_line);
		if (!$first_part.ctx->exception)
		{
			$op_text = $operand_field_rest.ctx->getText();
			$op_range = provider.get_range($operand_field_rest.ctx);
			process_instruction();
		}
	} EOLLN
	| SPACE? 
	{
		collector.set_label_field(provider.get_range( _localctx));
		collector.set_instruction_field(provider.get_range( _localctx));
		collector.set_operand_remark_field(provider.get_range( _localctx));
		ctx->set_source_indices(statement_start().file_offset, statement_end().file_offset, statement_end().file_line);
		process_instruction();
		process_statement();
	} EOLLN
	| EOF	{finished_flag=true;};

num_ch
	: NUM+;

num returns [self_def_t value]
	: num_ch									{$value = parse_self_def_term("D",$num_ch.ctx->getText(),provider.get_range($num_ch.ctx));};

self_def_term returns [self_def_t value]
	: ORDSYMBOL string							
	{
		auto opt = $ORDSYMBOL->getText(); 
		$value = parse_self_def_term(opt, $string.value, provider.get_range($ORDSYMBOL,$string.ctx->getStop()));
	};


id_ch returns [std::string value]
	: IDENTIFIER								{$value = $IDENTIFIER->getText();} 
	| NUM										{$value = $NUM->getText();} 
	| ORDSYMBOL									{$value = $ORDSYMBOL->getText();};

id_ch_c returns [std::string value]
	: id_ch										{$value = std::move($id_ch.value);}
	| tmp=id_ch_c id_ch							{$tmp.value.append(std::move($id_ch.value));  $value = std::move($tmp.value);};


id returns [id_index name, id_index using_qualifier]
	: id_no_dot									{$name = $id_no_dot.name;}
	| q=id_no_dot dot n=id_no_dot				{$name = $n.name; $using_qualifier = $q.name; };

id_no_dot returns [id_index name = id_storage::empty_id]
	: ORDSYMBOL id_ch_c
	{
		std::string tmp($ORDSYMBOL->getText());
		tmp.append(std::move($id_ch_c.value));
		$name = parse_identifier(std::move(tmp),provider.get_range($ORDSYMBOL,$id_ch_c.ctx->getStop()));
	}
	| ORDSYMBOL									{$name = parse_identifier($ORDSYMBOL->getText(),provider.get_range($ORDSYMBOL));};




opt_dot returns [std::string value]
	: dot id_ch_c								{$value = std::move($id_ch_c.value); }
	| ;

id_comma_c: id
	| id comma id_comma_c;



remark_ch: DOT|ASTERISK|MINUS|PLUS|LT|GT|COMMA|LPAR|RPAR|SLASH|EQUALS|AMPERSAND|APOSTROPHE|IDENTIFIER|NUM|VERTICAL|ORDSYMBOL|SPACE|ATTR;

remark
	: remark_ch*;

remark_o returns [std::optional<range> value]
	: SPACE remark							{$value = provider.get_range( $remark.ctx);}
	| ;





	//***** highlighting rules
comma 
	: COMMA {collector.add_hl_symbol(token_info(provider.get_range( $COMMA),hl_scopes::operator_symbol)); };
dot 
	: DOT {collector.add_hl_symbol(token_info(provider.get_range( $DOT),hl_scopes::operator_symbol)); };
apostrophe 
	: APOSTROPHE {collector.add_hl_symbol(token_info(provider.get_range( $APOSTROPHE),hl_scopes::operator_symbol)); };
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




expr_statement
	: expr
	| tmp=expr_statement EOLLN expr
	;

expr_test
	:  expr_statement EOLLN EOF	;
