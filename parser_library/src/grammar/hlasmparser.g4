parser grammar hlasmparser;

@header{
	#include <string>
	#include <iostream>
	#include "../../include/shared/lexer.h"
}

options {
    tokenVocab = lex;
}

@members {
	hlasm_plugin::parser_library::HlasmLexer * lexer;
	bool has_operands = false;
	void identify(const std::string& token)
	{
		has_operands = std::all_of(token.cbegin(), token.cend(), [](char c){ return isupper(c)||isdigit(c); } );
	}
	bool check_cont()
	{
		return !lexer->continuationBeforeToken(_input->index());
	}
	void substitute()
	{
	
	}
	bool is(std::string expected)
	{
		return expected == getCurrentToken()->getText();
	}
}

program : ictl? process_instruction* programBlock EOF 
			;

ictl: SPACE ORDSYMBOL { $ORDSYMBOL.text == "ICTL" }? SPACE ictl_begin EOLLN{ lexer->setICTL(); };

ictl_begin: IDENTIFIER ictl_end? 
				{ 
					size_t idx = 0;
					auto val = std::stoi($IDENTIFIER.text, &idx);
					if(idx > 0 || !lexer->setBegin(val))
						throw RecognitionException("invalid ICTL parameter value", this, _input, _localctx, $IDENTIFIER); 
				}
			;

ictl_end: COMMA IDENTIFIER ictl_continue 
				{ 
					size_t idx = 0;
					auto val = std::stoi($IDENTIFIER.text, &idx);
					if(idx > 0 || !lexer->setEnd(val))
						throw RecognitionException("invalid ICTL parameter value", this, _input, _localctx, $IDENTIFIER); 
				}
			;

ictl_continue:  { lexer->setContinuationEnabled(false); }

                | COMMA IDENTIFIER 
					{ 
						size_t idx = 0;
						auto val = std::stoi($IDENTIFIER.text, &idx);
						if(idx > 0 || !lexer->setContinue(val))
							throw RecognitionException("invalid ICTL parameter value", this, _input, _ctx, $IDENTIFIER); 
					}
				;

process_instruction: PROCESS SPACE (assembler_options
									| (ORDSYMBOL { $ORDSYMBOL.text == "OVERRIDE" }? LPAR assembler_options RPAR)) EOLLN;


assembler_options: assembler_option (COMMA assembler_option)*;

assembler_option: identifier (LPAR (list | codepage | machine | identifier)? RPAR)?;

list: list_item (COMMA list_item)*;

list_item: identifier (LPAR (identifier | string)+? RPAR)?;

machine: identifier (MINUS identifier)? (COMMA identifier)?;

codepage: identifier VERTICAL identifier string;


programBlock : ((instructionStatement|macro|SPACE) EOLLN+)+
			;

macro : macroStart programBlock macroEnd
		;

macroStart: { is("MACRO") }? IDENTIFIER EOLLN
				;

macroEnd: { is("MEND") }? IDENTIFIER
		;

instructionStatement : (label)? SPACE instruction SPACE+ instrEnd
					| (label)? SPACE instruction SPACE*
					;
label : seqSymbol
		| (identifier|~(PROCESS|SPACE|EOLLN))+
		;

symbol : seqSymbol
		| setSymbol
		;

seqSymbol: DOT ORDSYMBOL;

varSymbol : AMPERSAND ORDSYMBOL (LPAR arithExpr RPAR)? (DOT)?
			;

setSymbol : AMPERSAND LPAR (identifier|setSymbol)+ RPAR
			;

instruction : (varSymbol|~(SPACE|EOLLN))+ { identify(getRuleContext()->getStart()->getText()); }
				;

instrEnd : { has_operands }? endWithOperands
			| { !has_operands }? remark+
			;

endWithOperands : operands
			| (operand? COMMA)+ SPACE remark? (endWithOperands|continuedRemark)?
			| operands SPACE (remark+)?
			;

continuedRemark: SPACE remark+?
				;

operands : operand? (COMMA operand?)+ 
			| operand
			;

remark
  :  ( { check_cont() }? ~(EOLLN) )+
  ;

operand : expr
		| string sublist
		| arithExpr LPAR arithExpr (COMMA arithExpr)* RPAR
		| arithExpr LPAR (arithExpr)? COMMA arithExpr RPAR
		| logicExpr seqSymbol
		| dataDef
		| sublist
		| identifier EQUALS (operand)?
		| builtInFunction
		;

builtInFunction : LPAR identifier SPACE arithExpr RPAR
				;

arithExpr : term
			| arithExpr (MINUS|PLUS) term
			| (MINUS|PLUS) term

			;
term: factor
	| term (ASTERISK|SLASH) factor
	;

expr : arithExpr
		| logicExpr
		;

logicExpr : LPAR logicExpr RPAR
			| LPAR arithExpr SPACE logicSymbol SPACE arithExpr RPAR
			;

logicSymbol : OR | AND | EQ | LE | LTx | GE | GTx | NE 
			;

factor : LPAR arithExpr RPAR
		| symbol
		| location
		| typeAttribute
		| literal
		| selfDefTerm
		| string
		| identifier
		;

literal : EQUALS identifier string
		;

location : ASTERISK
		;

typeAttribute : identifier APOSTROPHE identifier
				;

selfDefTerm : identifier string
			;

identifier : (IDENTIFIER|ORDSYMBOL|varSymbol)+
			;

dataDef: identifier LPAR arithExpr RPAR
		;

sublist : arithExpr
		| LPAR ((sublist)? COMMA)+ (sublist)? RPAR
		;

string: APOSTROPHE (APOSTROPHE APOSTROPHE | varSymbol | ~APOSTROPHE)* APOSTROPHE
		;
