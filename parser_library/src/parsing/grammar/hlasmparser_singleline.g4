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
parser grammar hlasmparser_singleline;
                                                 
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
	#include "lexing/token.h"
	#include "lexing/token_stream.h"
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
	#include "utils/string_operations.h"
	#include "utils/truth_table.h"

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

lab_instr returns [std::optional<std::string> op_text, range op_range, size_t op_logical_column = 0]
	: PROCESS (SPACE (~EOF)*)? EOF
	{
		collector.set_label_field(provider.get_range($PROCESS));
		collector.set_instruction_field(
			parse_identifier($PROCESS->getText(),provider.get_range($PROCESS)),
			provider.get_range( $PROCESS));
		collector.add_hl_symbol(token_info(provider.get_range($PROCESS),hl_scopes::instruction));

		auto op_index = $PROCESS->getTokenIndex()+1;
		$op_text = _input->getText(misc::Interval(op_index,_input->size()-1));
		$op_range = provider.get_range(_input->get(op_index),_input->get(_input->size()-1));
		$op_logical_column = static_cast<hlasm_plugin::parser_library::lexing::token*>(_input->get(op_index))->get_logical_column();
	}
	| label SPACE instruction (SPACE (~EOF)*)? EOF
	{
		if (!$instruction.ctx->exception)
		{
			auto op_index = $instruction.stop->getTokenIndex()+1;
			$op_text = _input->getText(misc::Interval(op_index,_input->size()-1));
			$op_range = provider.get_range(_input->get(op_index),_input->get(_input->size()-1));
			$op_logical_column = static_cast<hlasm_plugin::parser_library::lexing::token*>(_input->get(op_index))->get_logical_column();
		}
	}
	| SPACE
	(
		instruction (SPACE (~EOF)*)? EOF
		{
			collector.set_label_field(provider.get_empty_range( _localctx->getStart()));
			if (!$instruction.ctx->exception)
			{
				auto op_index = $instruction.stop->getTokenIndex()+1;
				$op_text = _input->getText(misc::Interval(op_index,_input->size()-1));
				$op_range = provider.get_range(_input->get(op_index),_input->get(_input->size()-1));
				$op_logical_column = static_cast<hlasm_plugin::parser_library::lexing::token*>(_input->get(op_index))->get_logical_column();
			}
		}
		|
		EOF
		{
			collector.set_label_field(provider.get_range( _localctx));
			collector.set_instruction_field(provider.get_range( _localctx));
			collector.set_operand_remark_field(provider.get_range( _localctx));
		}
	)
	| EOF
	{
		collector.set_label_field(provider.get_range( _localctx));
		collector.set_instruction_field(provider.get_range( _localctx));
		collector.set_operand_remark_field(provider.get_range( _localctx));
	};
	catch[const FailedPredicateException&]
	{
		collector.set_label_field(provider.get_range( _localctx));
		collector.set_instruction_field(provider.get_range( _localctx));
		collector.set_operand_remark_field(provider.get_range( _localctx));
	}
	catch[RecognitionException &e]
	{
		_errHandler->reportError(this, e);
		_localctx->exception = std::current_exception();
		_errHandler->recover(this, _localctx->exception);
	}

num_ch
	: NUM+;

num returns [self_def_t value]
	: num_ch									{$value = parse_self_def_term("D",get_context_text($num_ch.ctx),provider.get_range($num_ch.ctx));};

signed_num_ch
	: MINUS? NUM+;

id returns [id_index name, id_index using_qualifier]
	: f=id_no_dot {$name = $f.name;} (dot s=id_no_dot {$name = $s.name; $using_qualifier = $f.name;})?;

id_no_dot returns [id_index name] locals [std::string buffer]
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


deferred_op_rem returns [remark_list remarks, std::vector<vs_ptr> var_list]
	:
	(
		deferred_entry
		{
			for (auto&v : $deferred_entry.vs)
				$var_list.push_back(std::move(v));
		}
	)*
	remark_o {if($remark_o.value) $remarks.push_back(*$remark_o.value);}
	;

//////////////////////////////////////// ca

op_rem_body_ca_branch locals [bool pending_empty_op = true, std::vector<range> remarks, std::vector<operand_ptr> operands, antlr4::Token* first_token = nullptr]
	:
	EOF
	{
		collector.set_operand_remark_field(provider.get_range(_localctx));
	}
	|
	SPACE+
	(
		{
			$first_token = _input->LT(1);
		}
		(
			comma
			{
				if ($pending_empty_op)
					$operands.push_back(std::make_unique<semantics::empty_operand>(provider.get_empty_range($comma.start)));
				$pending_empty_op = true;
			}
			|
			{
				if (!$pending_empty_op)
					throw NoViableAltException(this);
			}
			ca_op=ca_op_branch
			{
				$pending_empty_op = false;
			}
			{
                if ($ca_op.op)
                    $operands.push_back(std::move($ca_op.op));
                else
                    $operands.push_back(std::make_unique<semantics::empty_operand>(provider.get_empty_range($ca_op.start)));
			}
		)+
		{
			if ($pending_empty_op)
				$operands.push_back(std::make_unique<semantics::empty_operand>(provider.get_empty_range(_input->LT(-1))));
		}
		(
			SPACE
			remark
			{
				$remarks.push_back(provider.get_range($remark.ctx));
			}
		)?
		|
		{
			collector.set_operand_remark_field(provider.get_range($ctx->getStart(),_input->LT(-1)));
		}
	);
	finally
	{
		if ($first_token)
			collector.set_operand_remark_field(std::move($operands), std::move($remarks), provider.get_range($first_token, _input->LT(-1)));
	}
op_rem_body_ca_expr locals [bool pending_empty_op = true, std::vector<range> remarks, std::vector<operand_ptr> operands, antlr4::Token* first_token = nullptr]
	:
	EOF
	{
		collector.set_operand_remark_field(provider.get_range(_localctx));
	}
	|
	SPACE+
	(
		{
			$first_token = _input->LT(1);
		}
		(
			comma
			{
				if ($pending_empty_op)
					$operands.push_back(std::make_unique<semantics::empty_operand>(provider.get_empty_range($comma.start)));
				$pending_empty_op = true;
			}
			|
			{
				if (!$pending_empty_op)
					throw NoViableAltException(this);
			}
			ca_op=ca_op_expr
			{
				$pending_empty_op = false;
			}
			{
                if ($ca_op.op)
                    $operands.push_back(std::move($ca_op.op));
                else
                    $operands.push_back(std::make_unique<semantics::empty_operand>(provider.get_empty_range($ca_op.start)));
			}
		)+
		{
			if ($pending_empty_op)
				$operands.push_back(std::make_unique<semantics::empty_operand>(provider.get_empty_range(_input->LT(-1))));
		}
		(
			SPACE
			remark
			{
				$remarks.push_back(provider.get_range($remark.ctx));
			}
		)?
		|
		{
			collector.set_operand_remark_field(provider.get_range($ctx->getStart(),_input->LT(-1)));
		}
	);
	finally
	{
		if ($first_token)
			collector.set_operand_remark_field(std::move($operands), std::move($remarks), provider.get_range($first_token, _input->LT(-1)));
	}

op_rem_body_ca_var_def locals [bool pending_empty_op = true, std::vector<range> remarks, std::vector<operand_ptr> operands, antlr4::Token* first_token = nullptr]
	:
	EOF
	{
		collector.set_operand_remark_field(provider.get_range(_localctx));
	}
	|
	SPACE+
	(
		{
			$first_token = _input->LT(1);
		}
		(
			comma
			{
				if ($pending_empty_op)
					$operands.push_back(std::make_unique<semantics::empty_operand>(provider.get_empty_range($comma.start)));
				$pending_empty_op = true;
			}
			|
			{
				if (!$pending_empty_op)
					throw NoViableAltException(this);
			}
			ca_op=ca_op_var_def
			{
				$pending_empty_op = false;
			}
			{
                if ($ca_op.op)
                    $operands.push_back(std::move($ca_op.op));
                else
                    $operands.push_back(std::make_unique<semantics::empty_operand>(provider.get_empty_range($ca_op.start)));
			}
		)+
		{
			if ($pending_empty_op)
				$operands.push_back(std::make_unique<semantics::empty_operand>(provider.get_empty_range(_input->LT(-1))));
		}
		(
			SPACE
			remark
			{
				$remarks.push_back(provider.get_range($remark.ctx));
			}
		)?
		|
		{
			collector.set_operand_remark_field(provider.get_range($ctx->getStart(),_input->LT(-1)));
		}
	);
	finally
	{
		if ($first_token)
			collector.set_operand_remark_field(std::move($operands), std::move($remarks), provider.get_range($first_token, _input->LT(-1)));
	}

//////////////////////////////////////// mac

op_rem_body_mac returns [op_rem line, range line_range, size_t line_logical_column = 0]
	:
	SPACE* EOF {$line_range = provider.get_range($ctx->getStart(), _input->LT(-1));}
	|
	SPACE+ op_rem_body_alt_mac
	{
		$line = std::move($op_rem_body_alt_mac.line);
		$line_range = provider.get_range($op_rem_body_alt_mac.ctx);
		$line_logical_column = static_cast<hlasm_plugin::parser_library::lexing::token*>($op_rem_body_alt_mac.start)->get_logical_column();
	} EOF;

op_rem_body_alt_mac returns [op_rem line]
	:
	(
		mac_op? COMMA
		{
			if ($mac_op.ctx && $mac_op.op)
				$line.operands.push_back(std::move($mac_op.op));
			$line.operands.push_back(std::make_unique<semantics::empty_operand>(provider.get_range($COMMA)));
		}
	)*
	(
		last_mac_op=mac_op? last_remark=remark_o
		{
			if ($last_mac_op.ctx && $last_mac_op.op)
				$line.operands.push_back(std::move($last_mac_op.op));
			if ($last_remark.value)
				$line.remarks.push_back(std::move(*$last_remark.value));
		}
	);
