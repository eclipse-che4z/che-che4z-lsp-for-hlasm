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

 //rules for model operand
parser grammar model_operand_rules;

before_var_sym_model_string returns [std::string value]
    : ap1=APOSTROPHE model_string_ch_c ap2=(APOSTROPHE|ATTR)
    {
        $value.append(std::move($model_string_ch_c.value));
        collector.add_hl_symbol(token_info(provider.get_range($ap1,$ap2),hl_scopes::string));
    };

before_var_sym_model_b returns [std::string value]
    :
    ( ASTERISK                              {$value = "*";}
    | MINUS                                 {$value = "-";}
    | PLUS                                  {$value = "+";}
    | LT                                    {$value = "<";}
    | GT                                    {$value = ">";}
    | SLASH                                 {$value = "/";}
    | EQUALS                                {$value = "=";}
    | AMPERSAND AMPERSAND                   {$value = "&&";}
    | VERTICAL                              {$value = "|";}
    | IDENTIFIER                            {$value = $IDENTIFIER->getText();}
    | NUM                                   {$value = $NUM->getText();}
    | ORDSYMBOL                             {$value = $ORDSYMBOL->getText();}
    | DOT                                   {$value = ".";}
    | lpar                                  {$value = "("; }
    | rpar                                  {$value = ")"; }
    | comma                                 {$value = ","; }
    )
    | ATTR                                  {$value = "'"; }
    |
    before_var_sym_model_string
    {
        $value.reserve($before_var_sym_model_string.value.size()+2);
        $value.push_back('\'');
        $value.append($before_var_sym_model_string.value);
        $value.push_back('\'');
    };

before_var_sym_model returns [std::string value]
    :
    | tmp=before_var_sym_model before_var_sym_model_b   {$tmp.value.append(std::move($before_var_sym_model_b.value)); $value = std::move($tmp.value);};

var_sym_model [concat_chain* chain]
    : AMPERSAND var_symbol_base[$AMPERSAND]             {$chain->emplace_back(var_sym_conc(std::move($var_symbol_base.vs)));}
    | string_v_actual[$chain];

after_var_sym_model [concat_chain* chain]
    :
    ( ASTERISK                              {$chain->emplace_back(char_str_conc("*", provider.get_range($ASTERISK)));}
    | MINUS                                 {$chain->emplace_back(char_str_conc("-", provider.get_range($MINUS)));}
    | PLUS                                  {$chain->emplace_back(char_str_conc("+", provider.get_range($PLUS)));}
    | LT                                    {$chain->emplace_back(char_str_conc("<", provider.get_range($LT)));}
    | GT                                    {$chain->emplace_back(char_str_conc(">", provider.get_range($GT)));}
    | SLASH                                 {$chain->emplace_back(char_str_conc("/", provider.get_range($SLASH)));}
    | EQUALS                                {$chain->emplace_back(equals_conc(provider.get_range($EQUALS)));}
    | VERTICAL                              {$chain->emplace_back(char_str_conc("|", provider.get_range($VERTICAL)));}
    | IDENTIFIER                            {$chain->emplace_back(char_str_conc($IDENTIFIER->getText(), provider.get_range($IDENTIFIER)));}
    | NUM                                   {$chain->emplace_back(char_str_conc($NUM->getText(), provider.get_range($NUM)));}
    | ORDSYMBOL                             {$chain->emplace_back(char_str_conc($ORDSYMBOL->getText(), provider.get_range($ORDSYMBOL)));}
    | DOT                                   {$chain->emplace_back(dot_conc(provider.get_range($DOT)));}
    | l=AMPERSAND
    (
        r=AMPERSAND                         {$chain->emplace_back(char_str_conc("&&", provider.get_range($l,$r)));}
        |
        var_symbol_base[$l]                 {$chain->emplace_back(var_sym_conc(std::move($var_symbol_base.vs)));}
    )
    | lpar                                  {$chain->emplace_back(char_str_conc("(", provider.get_range($lpar.ctx->getStart()))); }
    | rpar                                  {$chain->emplace_back(char_str_conc(")", provider.get_range($rpar.ctx->getStart()))); }
    | comma                                 {$chain->emplace_back(char_str_conc(",", provider.get_range($comma.ctx->getStart()))); }
    |
    (
        ATTR                                {$chain->emplace_back(char_str_conc("'", provider.get_range($ATTR))); }
        |
        model_string_v[$chain]
    )
    )*
    ;


model_op returns [std::optional<concat_chain> chain_opt = concat_chain()]
    :
    before_var_sym_model
    {
            $chain_opt->emplace_back(char_str_conc(std::move($before_var_sym_model.value), provider.get_range($before_var_sym_model.ctx)));
    }
    var_sym_model[&$chain_opt.value()] after_var_sym_model[&$chain_opt.value()]
    {
        if($var_sym_model.ctx->exception)
            $chain_opt = std::nullopt;
        else
        {
            concatenation_point::clear_concat_chain($chain_opt.value());
            resolve_concat_chain($chain_opt.value());
        }
    }
    ;
    finally
    {
        // rest of the code assumes non-empty chain_opt
        if (_localctx->chain_opt && _localctx->chain_opt->empty())
            _localctx->chain_opt.reset();
    }

model_string_ch returns [std::string value]
    :
    ( ASTERISK                              {$value = "*";}
    | MINUS                                 {$value = "-";}
    | PLUS                                  {$value = "+";}
    | LT                                    {$value = "<";}
    | GT                                    {$value = ">";}
    | SLASH                                 {$value = "/";}
    | EQUALS                                {$value = "=";}
    | AMPERSAND AMPERSAND                   {$value = "&&";}
    | VERTICAL                              {$value = "|";}
    | IDENTIFIER                            {$value = $IDENTIFIER->getText();}
    | NUM                                   {$value = $NUM->getText();}
    | ORDSYMBOL                             {$value = $ORDSYMBOL->getText();}
    | DOT                                   {$value = ".";}
    | COMMA                                 {$value = ",";}
    | LPAR                                  {$value = "(";}
    | RPAR                                  {$value = ")";}
    | SPACE                                 {$value = $SPACE->getText();}
    )
    | (APOSTROPHE|ATTR) (APOSTROPHE|ATTR)   {$value = "''";}
    ;

model_string_ch_v [concat_chain* chain]
    :
    ( ASTERISK                              {$chain->emplace_back(char_str_conc("*", provider.get_range($ASTERISK)));}
    | MINUS                                 {$chain->emplace_back(char_str_conc("-", provider.get_range($MINUS)));}
    | PLUS                                  {$chain->emplace_back(char_str_conc("+", provider.get_range($PLUS)));}
    | LT                                    {$chain->emplace_back(char_str_conc("<", provider.get_range($LT)));}
    | GT                                    {$chain->emplace_back(char_str_conc(">", provider.get_range($GT)));}
    | SLASH                                 {$chain->emplace_back(char_str_conc("/", provider.get_range($SLASH)));}
    | EQUALS                                {$chain->emplace_back(equals_conc(provider.get_range($EQUALS)));}
    | VERTICAL                              {$chain->emplace_back(char_str_conc("|", provider.get_range($VERTICAL)));}
    | IDENTIFIER                            {$chain->emplace_back(char_str_conc($IDENTIFIER->getText(), provider.get_range($IDENTIFIER)));}
    | NUM                                   {$chain->emplace_back(char_str_conc($NUM->getText(), provider.get_range($NUM)));}
    | ORDSYMBOL                             {$chain->emplace_back(char_str_conc($ORDSYMBOL->getText(), provider.get_range($ORDSYMBOL)));}
    | DOT                                   {$chain->emplace_back(dot_conc(provider.get_range($DOT)));}
    | l=AMPERSAND
    (
        r=AMPERSAND                         {$chain->emplace_back(char_str_conc("&&", provider.get_range($l,$r)));}
        |
        var_symbol_base[$l]                 {$chain->emplace_back(var_sym_conc(std::move($var_symbol_base.vs)));}
    )
    | COMMA                                 {$chain->emplace_back(char_str_conc(",", provider.get_range($COMMA)));}
    | LPAR                                  {$chain->emplace_back(char_str_conc("(", provider.get_range($LPAR)));}
    | RPAR                                  {$chain->emplace_back(char_str_conc(")", provider.get_range($RPAR)));}
    | SPACE                                 {$chain->emplace_back(char_str_conc($SPACE->getText(), provider.get_range($SPACE)));}
    )
    | l=(APOSTROPHE|ATTR) r=(APOSTROPHE|ATTR)   {$chain->emplace_back(char_str_conc("''", provider.get_range($l, $r)));}
    ;

model_string_ch_v_c [concat_chain* chain]
    :
    (
        model_string_ch_v[$chain]
    )*;

model_string_ch_c returns [std::string value]
    :
    | tmp=model_string_ch_c model_string_ch {$value = std::move($tmp.value); $value.append($model_string_ch.value);};

string_v_actual [concat_chain* chain]
    : ap1=(APOSTROPHE|ATTR)    model_string_ch_c AMPERSAND var_symbol_base[$AMPERSAND]
    {
        $chain->emplace_back(char_str_conc("'", provider.get_range($ap1)));
        $chain->emplace_back(char_str_conc(std::move($model_string_ch_c.value), provider.get_range($model_string_ch_c.ctx)));
        $chain->emplace_back(var_sym_conc(std::move($var_symbol_base.vs)));
    }
    model_string_ch_v_c[$chain] ap2=(APOSTROPHE|ATTR)
    {
        $chain->emplace_back(char_str_conc("'", provider.get_range($ap2)));
        collector.add_hl_symbol(token_info(provider.get_range($ap1,$ap2),hl_scopes::string));
    };

model_string_v [concat_chain* chain]
    : ap1=(APOSTROPHE|ATTR)
    {
        $chain->emplace_back(char_str_conc("'", provider.get_range($ap1)));
    }
    model_string_ch_v_c[$chain] ap2=(APOSTROPHE|ATTR)
    {
        $chain->emplace_back(char_str_conc("'", provider.get_range($ap2)));
        collector.add_hl_symbol(token_info(provider.get_range($ap1,$ap2),hl_scopes::string));
    };
