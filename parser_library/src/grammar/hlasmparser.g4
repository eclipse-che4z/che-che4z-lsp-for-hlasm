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
	#include "../parser_impl.h"
	#include "../expressions/expression.h"
	#include "../expressions/arithmetic_expression.h"
	#include "../expressions/logic_expression.h"
	#include "../expressions/character_expression.h"
	#include "../expressions/keyword_expression.h"
	#include "../expressions/mach_expr_term.h"
	#include "../expressions/mach_operator.h"
	#include "../expressions/data_definition.h"

	namespace hlasm_plugin::parser_library::generated
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


program : program_line*;

program_line
	: instruction_statement EOLLN
	| EOF	{finished_flag=true;};




instruction_statement
	: { processor->kind == processing_kind::LOOKAHEAD }? lookahead_instruction_statement										
	{
		collector.clear_hl_lsp_symbols();
		process_statement();
	}
	| { processor->kind != processing_kind::LOOKAHEAD }? ordinary_instruction_statement										
	{
		process_statement();
		//sem_info_.process_deferred_syms();
	}
	| SPACE*					
	{
		collector.set_label_field(provider.get_range( _localctx));
		collector.set_instruction_field(provider.get_range( _localctx));
		collector.set_operand_remark_field(provider.get_range( _localctx));
		process_instruction();
		process_statement();
	};


ordinary_instruction_statement
	: label SPACE instruction operands_and_remarks
	{
		collector.add_hl_symbol(token_info(provider.get_range($instruction.ctx),hl_scopes::instruction));
		collector.add_operands_hl_symbols();
		collector.add_remarks_hl_symbols();
	}
	| PROCESS 
	{
		collector.set_label_field(provider.get_range($PROCESS));
		collector.set_instruction_field(
			parse_identifier($PROCESS->getText(),provider.get_range($PROCESS)),
			provider.get_range( $PROCESS));
		process_instruction();
	}
	operands_and_remarks
	{
		collector.add_hl_symbol(token_info(provider.get_range($PROCESS),hl_scopes::instruction));
		collector.add_operands_hl_symbols();
		collector.add_remarks_hl_symbols();
	};


num_ch
	: NUM+;

num returns [self_def_t value]
	: num_ch									{$value = parse_self_def_term("",$num_ch.ctx->getText(),provider.get_range($num_ch.ctx));};

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
	| q=id_no_dot dot_ n=id_no_dot				{$name = $n.name; $using_qualifier = $q.name; };

id_no_dot returns [id_index name = id_storage::empty_id]
	: ORDSYMBOL id_ch_c
	{
		std::string tmp($ORDSYMBOL->getText());
		tmp.append(std::move($id_ch_c.value));
		$name = parse_identifier(std::move(tmp),provider.get_range($ORDSYMBOL,$id_ch_c.ctx->getStop()));
	}
	| ORDSYMBOL									{$name = parse_identifier($ORDSYMBOL->getText(),provider.get_range($ORDSYMBOL));};




opt_dot returns [std::string value]
	: dot_ id_ch_c								{$value = std::move($id_ch_c.value); }
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
dot_ 
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
equals_ 
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
	: expr_p
	| tmp=expr_statement EOLLN expr_p
	;

expr_test
	:  expr_statement EOLLN EOF	;
