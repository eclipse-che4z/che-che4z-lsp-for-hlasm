parser grammar hlasmparser;

@header{
	#include <vector>
	#include <string>
	#include "../../include/shared/lexer.h"
	#include "../context/expression.h"
	#include "../context/arithmetic_expression.h"
	#include "../context/logic_expression.h"
	#include "../context/character_expression.h"
	#include "../context/keyword_expression.h"

	#include "../parser_tools.h"
	#include "../context/hlasm_context.h"

	/* disables unreferenced parameter (_localctx) warning */
	#ifdef _MSC_VER
		#pragma warning(push)
		#pragma warning(disable: 4100)
	#endif
}

@footer {
	#ifdef _MSC_VER
		#pragma warning(pop)
	#endif
}

options {
    tokenVocab = lex;
}

@members {
	#define last_text() _input->LT(-1)->getText()
	#define text(token) token->getText()

	using expr_ptr = std::unique_ptr<hlasm_plugin::parser_library::context::expression>;
	using expression = hlasm_plugin::parser_library::context::expression;
	using arith_expr = hlasm_plugin::parser_library::context::arithmetic_expression;
	using char_expr = hlasm_plugin::parser_library::context::character_expression;
	using keyword_expr = hlasm_plugin::parser_library::context::keyword_expression;
	hlasm_plugin::parser_library::lexer * lexer;
	hlasm_plugin::parser_library::context::hlasm_context ctx;
	bool check_cont()
	{
		return !lexer->continuation_before_token(_input->index());
	}
}

program : ictl? process_instruction* program_block  EOF 
			;

ictl: SPACE ORDSYMBOL { $ORDSYMBOL.text == "ICTL" }? SPACE ictl_begin EOLLN{ lexer->set_ictl(); };

ictl_begin: IDENTIFIER ictl_end? 
				{ 
					size_t idx = 0;
					auto val = std::stoi($IDENTIFIER.text, &idx);
					if(idx > 0 || !lexer->set_begin(val))
						throw RecognitionException("invalid ICTL parameter value", this, _input, _localctx, $IDENTIFIER); 
				}
			;

ictl_end: COMMA IDENTIFIER ictl_continue 
				{ 
					size_t idx = 0;
					auto val = std::stoi($IDENTIFIER.text, &idx);
					if(idx > 0 || !lexer->set_end(val))
						throw RecognitionException("invalid ICTL parameter value", this, _input, _localctx, $IDENTIFIER); 
				}
			;

ictl_continue:  { lexer->set_continuation_enabled(false); }

                | COMMA IDENTIFIER 
					{ 
						size_t idx = 0;
						auto val = std::stoi($IDENTIFIER.text, &idx);
						if(idx > 0 || !lexer->set_continue(val))
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

identifier: VERTICAL;

program_block: instr_c;

instr_c: instruction_statement
	   | instr_c EOLLN instruction_statement;

instruction_statement: label SPACE instruction operand_and_remark 
			       	 | SPACE
					 | ;


//label rules************************************************************

label: char_string	//MAC ordsymbol no ca sequence symbol
	 | char_string_ap_c char_string_o //MAC
	 | l_str l_str_no_space_c_o //possible model stmt rule with no space
	 | l_str_poss_space_c l_str l_str_no_space_c_o //possible model stmt rule with possible space
	 | l_str_poss_space_c //possible model stmt rule with possible space
	 | ;

//label rules************************************************************



l_ch: ASTERISK|MINUS|PLUS|LT|GT|COMMA|LPAR|RPAR|SLASH|EQUALS|AMPERSAND AMPERSAND|VERTICAL|IDENTIFIER|ORDSYMBOL|DOT
	| var_symbol;

l_str: l_ch
	| l_str l_ch;

l_str_o: l_str
	| ;

l_str_no_space_c: APOSTROPHE l_str_o APOSTROPHE l_str_o
	| l_str_no_space_c APOSTROPHE l_str_o APOSTROPHE l_str_o;

l_str_no_space_c_o: l_str_no_space_c
	| ;


l_sp_ch: ASTERISK|MINUS|PLUS|LT|GT|COMMA|LPAR|RPAR|SLASH|EQUALS|AMPERSAND AMPERSAND|VERTICAL|IDENTIFIER|ORDSYMBOL|DOT|SPACE
	| var_symbol;

l_sp_str: l_sp_ch
	| l_sp_str l_sp_ch;

l_sp_str_o: l_sp_str
	| ;

l_str_poss_space_c: APOSTROPHE l_sp_str_o APOSTROPHE
			| l_str_poss_space_c APOSTROPHE l_sp_str_o APOSTROPHE;





ch: ASTERISK|MINUS|PLUS|LT|GT|COMMA|LPAR|RPAR|SLASH|EQUALS|AMPERSAND AMPERSAND|VERTICAL|IDENTIFIER|ORDSYMBOL|DOT;

ch_s:ASTERISK|MINUS|PLUS|LT|GT|COMMA|LPAR|RPAR|SLASH|EQUALS|AMPERSAND AMPERSAND|VERTICAL|IDENTIFIER|ORDSYMBOL|DOT|SPACE;

ch_a: ASTERISK|MINUS|PLUS|LT|GT|COMMA|LPAR|RPAR|SLASH|EQUALS|AMPERSAND AMPERSAND|VERTICAL|IDENTIFIER|ORDSYMBOL|DOT|APOSTROPHE;

//char strings without var symbols***************************************
char_string_b: ASTERISK|MINUS|PLUS|LT|GT|COMMA|LPAR|RPAR|SLASH|EQUALS|AMPERSAND AMPERSAND|VERTICAL|IDENTIFIER|ORDSYMBOL|APOSTROPHE|DOT;

char_string_b_a: ASTERISK|MINUS|PLUS|LT|GT|COMMA|LPAR|RPAR|SLASH|EQUALS|AMPERSAND AMPERSAND|VERTICAL|IDENTIFIER|ORDSYMBOL|DOT;

char_string_p: char_string_b				//does not begin with apo
		     | char_string_p char_string_b;

char_string_p_o: char_string_p
			   | ;

char_string: char_string_b_a char_string_p_o;

char_string_o: char_string
			 | ;

char_string_ap_b: DOT|ASTERISK|MINUS|PLUS|LT|GT|COMMA|LPAR|RPAR|SLASH|EQUALS|AMPERSAND AMPERSAND|IDENTIFIER|VERTICAL|SPACE|ORDSYMBOL;

char_string_ap_p: char_string_ap_b
			    | char_string_ap_p char_string_ap_b;

char_string_ap_p_o: char_string_ap_p
				  | ;

char_string_ap: APOSTROPHE char_string_ap_p_o APOSTROPHE;

char_string_ap_c: char_string_ap
			    |char_string_ap_c char_string_ap  ;
//char strings without var symbols***************************************
			


//instruction rules******************************************************
instruction: ORDSYMBOL 
		   | ORDSYMBOL macro_name
		   | instr_model; //instruction model stmt
//instruction rules******************************************************


instr_ch: ASTERISK|MINUS|PLUS|LT|GT|COMMA|LPAR|RPAR|SLASH|EQUALS|AMPERSAND AMPERSAND|VERTICAL|IDENTIFIER|ORDSYMBOL|DOT|APOSTROPHE
	| var_symbol;

instr_model: instr_ch
	| instr_model instr_ch;

macro_name: ASTERISK|MINUS|PLUS|LT|GT|COMMA|LPAR|RPAR|SLASH|EQUALS|AMPERSAND|VERTICAL|IDENTIFIER|DOT|APOSTROPHE|ORDSYMBOL
	|macro_name ASTERISK|MINUS|PLUS|LT|GT|COMMA|LPAR|RPAR|SLASH|EQUALS|AMPERSAND|VERTICAL|IDENTIFIER|DOT|APOSTROPHE|ORDSYMBOL;


remark: DOT|ASTERISK|MINUS|PLUS|LT|GT|COMMA|LPAR|RPAR|SLASH|EQUALS|AMPERSAND|APOSTROPHE|IDENTIFIER|VERTICAL|ORDSYMBOL|SPACE;

remark_cont: { check_cont() }? remark;

remark_c: remark
	    |remark_c remark  ;

remark_c_o: remark_c
		  | ;


operand_and_remark: space_c op_rem_body
	| ; 

op_rem_body: op_list COMMA operand_not_empty SPACE remark_c_o 
		   | operand SPACE remark_c_o
		   | op_list space_o
		   | op_list COMMA SPACE (remark_cont+)? op_rem_body;

op_list: operand
	| op_list COMMA operand;

operand: operand_not_empty
	| ;

operand_not_empty: expr_p
	| expr_p LPAR expr RPAR
	| expr_p LPAR COMMA expr_p RPAR
	| expr_p LPAR expr_p COMMA expr_p RPAR
	| LPAR expr RPAR seq_symbol
	| mac_entry
	| id EQUALS mac_entry
	| id EQUALS mac_entry_b
	| data_def
	| seq_symbol;

expr_test returns [std::string test_str]
	:  EOF 
	|  expr EOLLN e=expr_test 
		{ 
			$test_str.append($expr.e->get_str_val());
			$test_str.append("\n");
			$test_str.append($e.test_str);
		}
	;

expr returns [expr_ptr e]
	: SPACE x=expr_p_space_c { $e = expression::evaluate(std::move($x.exprs)); }
	| x=expr_p_space_c { $e = expression::evaluate(std::move($x.exprs)); }
	;

expr_p returns [expr_ptr e]
	: expr_s { $e = std::move($expr_s.e); }
	| PLUS expr_p { $e = +*$expr_p.e; }
	| MINUS expr_p { $e = -*$expr_p.e; }
	;

expr_s returns [expr_ptr e]
	: a=expr_s PLUS b=term_c { $e = *$a.e + *$b.e; }
	| a=expr_s MINUS b=term_c  { $e = *$a.e - *$b.e; }
	| term_c { $e = std::move($term_c.e); }
	;

term_c returns [expr_ptr e]
	: term  { $e = std::move($term.e); }
	| a=term_c SLASH b=term { $e = *$a.e / *$b.e; }
	| a=term_c ASTERISK b=term { $e = *$a.e * *$b.e; }
	;

term returns [expr_ptr e]
	: LPAR expr RPAR { $e = std::move($expr.e); }
	| var_symbol { $e = expression::resolve_ord_symbol($var_symbol.id_name); }
	| str_term { $e = std::move($str_term.e); }
	| ASTERISK { $e = std::make_unique<keyword_expr>("*"); }
	| type_attribute { $e = std::move($type_attribute.e); }
	| id { $e = expression::resolve_ord_symbol($id.id_name); /*TODO:DBCS*/ }
	| o=ORDSYMBOL val=string { $e = expression::self_defining_term(text($o), $val.s, false);  /*TODO:DBCS*/}
	| EQUALS data_def  { $e = std::make_unique<arith_expr>(0); /* TODO */ }
	| id LPAR expr RPAR { $e = $expr.e->unary_operation($id.id_name); }//built in //TODO check for input
	| id LPAR a1=expr COMMA a2=expr RPAR { $e = $a1.e->binary_operation($id.id_name, $a2.e); }
	; //built in 

str_term returns [std::unique_ptr<char_expr> e]
	: df=dupl_factor ce=string subs=substring 
		{ 
			auto ex = std::make_unique<char_expr>(std::move($ce.s));
			$e = ex->substring($df.df, $subs.start, $subs.end);
		}
	| a=str_term DOT b=str_term { $e = $a.e->append($b.e); }
	;

id_begin returns [std::string id_name]
	: IDENTIFIER  { $id_name.append(last_text()); }
	| ORDSYMBOL { $id_name.append(last_text()); }
	;

id_ch returns [std::string id_name]
	: IDENTIFIER  { $id_name.append(last_text()); }
	| ORDSYMBOL  { $id_name.append(last_text()); }
	| DOT { $id_name.append(last_text()); }
	;

id_ch_c returns [std::string id_name]
	: id_ch { $id_name = std::move($id_ch.id_name); }
    | a=id_ch_c b=id_ch { $id_name = $a.id_name + $b.id_name; }
	;

id_ch_c_o returns [std::string id_name]
	: id_ch_c { $id_name = std::move($id_ch_c.id_name); }
    | 
	;

id returns [std::string id_name]
	: id_begin id_ch_c_o { $id_name = $id_begin.id_name + $id_ch_c_o.id_name; }
	;

seq_symbol: DOT id;

var_symbol returns [std::string id_name]
: AMPERSAND id LPAR expr_p_comma_c RPAR DOT? { $id_name = std::move($id.id_name); } //with sublist
	| AMPERSAND id DOT? { $id_name = std::move($id.id_name); }
	;

expr_p_comma_c: expr_p
	| expr_p_comma_c COMMA expr_p;

expr_p_space_c returns [std::deque<expr_ptr> exprs]
	: expr_p { $exprs.push_back(std::move($expr_p.e)); }
	| exs=expr_p_space_c SPACE expr_p 
		{ 
			$exs.exprs.push_back(std::move($expr_p.e));   
			$exprs = std::move($exs.exprs);
		}
	;

dupl_factor returns [int32_t df]
	: LPAR expr RPAR { $df = $expr.e->get_numeric_value(); }			//absolute
	| { $df = 1; }
	;

substring returns [expr_ptr start, expr_ptr end]
	: LPAR s=expr_p RPAR { $start = std::move($s.e); }
	| LPAR s=expr_p COMMA e=expr_p RPAR { $start = std::move($s.e); $end = std::move($e.e); }
	|
	;

type_attribute returns [expr_ptr e]
	: ORDSYMBOL APOSTROPHE EQUALS data_def { $e = std::make_unique<arith_expr>(0); /* TODO */ }
	| ORDSYMBOL APOSTROPHE string { $e = std::make_unique<arith_expr>(0); /* TODO */ }
	| ORDSYMBOL APOSTROPHE var_symbol { $e = std::make_unique<arith_expr>(0); /* TODO */ }
	| ORDSYMBOL APOSTROPHE id { $e = std::make_unique<arith_expr>(0); /* TODO */ }
	;
modifer: id
	   | ;

prog_type_and_modifier: LPAR  ORDSYMBOL string RPAR modifer		// self def term - dec/char/hex/bin ORDSYMBOL + modifier
		| LPAR id RPAR modifer									// symbol + modifier
		| ;

nominal_value: string 
	| LPAR expr_p_comma_c RPAR
	|;

data_def: df=dupl_factor type=id prog_type_and_modifier nominal_value;





space_c: SPACE
	|space_c SPACE;

space_o: SPACE
	   | ;



























mac_entry_b: var_symbol
		 | mac_entry
		 | mac_inner_c_o;

mac_ch_start: ASTERISK|MINUS|PLUS|LT|GT|SLASH|EQUALS|AMPERSAND AMPERSAND|VERTICAL|IDENTIFIER|ORDSYMBOL|DOT;

mac_ch_in:ASTERISK|MINUS|PLUS|LT|GT|SLASH|EQUALS|AMPERSAND AMPERSAND|VERTICAL|IDENTIFIER|ORDSYMBOL|DOT|COMMA;

mac_ch_start_c: mac_ch_start
 | mac_ch_start_c mac_ch_start;

mac_ch_in_c: mac_ch_in
 | mac_ch_in_c mac_ch_in;

mac_inner: mac_ch_start_c
 | string
 | LPAR  mac_in_c_o RPAR;

mac_in: mac_ch_in_c
	| string
	| LPAR  mac_in_c_o RPAR;

mac_in_c: mac_in
	|mac_in_c mac_in;

mac_in_c_o: mac_in_c
	| ;

mac_inner_c: mac_inner
 | mac_inner_c mac_inner;

mac_inner_c_o:mac_inner_c
	| ;

mac_ch: ASTERISK|MINUS|PLUS|LT|GT|COMMA|LPAR|RPAR|SLASH|EQUALS|AMPERSAND AMPERSAND|VERTICAL|IDENTIFIER|ORDSYMBOL|DOT|APOSTROPHE;


mac_entry_c: mac_entry_b
		   | mac_entry_c COMMA mac_entry_b;

mac_entry: LPAR mac_entry_c RPAR;

mac_arg: mac_entry
	   | id EQUALS mac_entry_b;


//not adn not and

string_ch: APOSTROPHE APOSTROPHE | DOT|ASTERISK|MINUS|PLUS|LT|GT|COMMA|LPAR|RPAR|SLASH|EQUALS|AMPERSAND AMPERSAND|IDENTIFIER|VERTICAL|ORDSYMBOL|SPACE;

string_str returns [std::string s]
	: string_ch { $s.append(last_text()); }
	| a=string_str string_ch { $a.s.append(last_text()); $s = std::move($a.s); }
	; 

string_b returns [std::string s]
	: a=string_str { $s = std::move($a.s); }
	| var_symbol
	;

string returns [std::string s]
	: APOSTROPHE a=string_b APOSTROPHE { $s = std::move($a.s); }
	| APOSTROPHE APOSTROPHE
	;
