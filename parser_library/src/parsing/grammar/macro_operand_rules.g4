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


mac_op returns [operand_ptr op]
    : mac_preproc
    {
        $op = std::make_unique<macro_operand_string>(get_context_text($mac_preproc.ctx),provider.get_range($mac_preproc.ctx));
    };

mac_op_o returns [operand_ptr op]
    : mac_entry?
    {
        if($mac_entry.ctx)
            $op = std::make_unique<macro_operand_chain>(std::move($mac_entry.chain),provider.get_range($mac_entry.ctx));
        else
            $op = std::make_unique<semantics::empty_operand>(provider.original_range);
    };

macro_ops returns [operand_list list]
    : mac_op_o  {$list.push_back(std::move($mac_op_o.op));} (comma mac_op_o {$list.push_back(std::move($mac_op_o.op));})* EOF;

mac_preproc
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
            LPAR
            |
            AMPERSAND
        )
        |
        LPAR
        |
        RPAR
        |
        APOSTROPHE
        (~(APOSTROPHE|ATTR|CONTINUATION))*
        (APOSTROPHE|ATTR)
        |
        ATTR
        (
            AMPERSAND
            (~(APOSTROPHE|ATTR|CONTINUATION|SPACE))*
            (APOSTROPHE|ATTR)
            |
            {!is_attribute_consuming(_input->LT(-2)) || (!can_attribute_consume(_input->LT(1)) && _input->LA(1) != AMPERSAND)}?
            (~(APOSTROPHE|ATTR|CONTINUATION))*
            (APOSTROPHE|ATTR)
            |
        )
    )+
    ;

mac_entry_nested_strings [concat_chain *chain]
    :
    (
        string_ch_v[$chain]
    )*
    ;

mac_entry_basic_tokens [concat_chain* chain]
    :
    (
        token=(ASTERISK|MINUS|PLUS|LT|GT|SLASH|VERTICAL)
        {
            $chain->emplace_back(char_str_conc($token.text, provider.get_range($token)));
        }
        | equals        {$chain->emplace_back(equals_conc(provider.get_range($equals.ctx)));}
        | dot            {$chain->emplace_back(dot_conc(provider.get_range($dot.ctx)));}
        | token=(IDENTIFIER|NUM|ORDSYMBOL)
        {
            auto r = provider.get_range($token);
            $chain->emplace_back(char_str_conc($token.text, r));
            collector.add_hl_symbol(token_info(r, hl_scopes::operand));
        }
        | AMPERSAND
        (
            vs_id tmp=subscript
            {
                auto id = $vs_id.name;
                auto r = provider.get_range( $AMPERSAND,$tmp.ctx->getStop());
                $chain->emplace_back(var_sym_conc(std::make_unique<basic_variable_symbol>(id, std::move($tmp.value), r)));
                collector.add_hl_symbol(token_info(provider.get_range($AMPERSAND, $vs_id.ctx->getStop()),hl_scopes::var_symbol));
            }
            |
            lpar (clc=created_set_body)? rpar subscript
            {
                $chain->emplace_back(var_sym_conc(std::make_unique<created_variable_symbol>($clc.ctx ? std::move($clc.concat_list) : concat_chain{},std::move($subscript.value),provider.get_range($AMPERSAND,$subscript.ctx->getStop()))));
                collector.add_hl_symbol(token_info(provider.get_range($AMPERSAND),hl_scopes::var_symbol));
            }
            |
            AMPERSAND
            {
                $chain->emplace_back(char_str_conc("&&", provider.get_range(_input->LT(-2),_input->LT(-1))));
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
            mac_entry
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
                    mac_entry
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
            $chain->emplace_back(sublist_conc(std::move(sublist)));
        }
        rpar
    )
    ;

mac_entry returns [concat_chain chain]
    :
    (
        mac_entry_basic_tokens[&$chain]
        |
        ap1=APOSTROPHE
        {
            $chain.emplace_back(char_str_conc("'", provider.get_range($ap1)));
        }
        (
            string_ch_v[&$chain]
        )*?
        ap2=(APOSTROPHE|ATTR)
        {
            $chain.emplace_back(char_str_conc("'", provider.get_range($ap2)));
            collector.add_hl_symbol(token_info(provider.get_range($ap1,$ap2),hl_scopes::string));
        }
        |
        at=ATTR
        {
            $chain.emplace_back(char_str_conc("'", provider.get_range($at)));
        }
        (
            {is_attribute_consuming(_input->LT(-2))}?
            {
                collector.add_hl_symbol(token_info(provider.get_range($at),hl_scopes::operator_symbol));
            }
            (
                (
                    equals
                    {
                        $chain.emplace_back(equals_conc(provider.get_range($equals.ctx)));
                    }
                )?
                ORDSYMBOL
                {
                    auto r = provider.get_range($ORDSYMBOL);
                    $chain.emplace_back(char_str_conc($ORDSYMBOL.text, r));
                    collector.add_hl_symbol(token_info(r, hl_scopes::operand));
                }
                |
                AMPERSAND
                (
                    vs_id tmp=subscript
                    {
                        auto id = $vs_id.name;
                        auto r = provider.get_range( $AMPERSAND,$tmp.ctx->getStop());
                        $chain.emplace_back(var_sym_conc(std::make_unique<basic_variable_symbol>(id, std::move($tmp.value), r)));
                        collector.add_hl_symbol(token_info(provider.get_range($AMPERSAND, $vs_id.ctx->getStop()),hl_scopes::var_symbol));
                    }
                    (
                        dot
                        mac_entry_basic_tokens[&$chain]+
                    )*
                    (
                        ap=(APOSTROPHE|ATTR)
                        {
                            $chain.emplace_back(char_str_conc("'", provider.get_range($ap)));
                            collector.add_hl_symbol(token_info(provider.get_range($at,$ap),hl_scopes::string));
                        }
                    )?
                    |
                    AMPERSAND
                    {
                        $chain.emplace_back(char_str_conc("&&", provider.get_range(_input->LT(-2),_input->LT(-1))));
                    }
                    (
                        ap=(APOSTROPHE|ATTR)
                        {
                            $chain.emplace_back(char_str_conc("'", provider.get_range($ap)));
                            collector.add_hl_symbol(token_info(provider.get_range($at,$ap),hl_scopes::string));
                        }
                    )?
                )
                |
                token=(ASTERISK|MINUS|PLUS|LT|GT|SLASH|VERTICAL|IDENTIFIER|NUM|DOT|LPAR|RPAR)
                {
                    auto r = provider.get_range($token);
                    $chain.emplace_back(char_str_conc($token.text, r));
                }
                mac_entry_nested_strings[&$chain]
                ap=(APOSTROPHE|ATTR)
                {
                    $chain.emplace_back(char_str_conc("'", provider.get_range($ap)));
                    collector.add_hl_symbol(token_info(provider.get_range($at,$ap),hl_scopes::string));
                }
            )
            |
            {!is_attribute_consuming(_input->LT(-2))}?
            mac_entry_nested_strings[&$chain]
            ap=(APOSTROPHE|ATTR)
            {
                $chain.emplace_back(char_str_conc("'", provider.get_range($ap)));
                collector.add_hl_symbol(token_info(provider.get_range($at,$ap),hl_scopes::string));
            }
        )
    )+
    ;
