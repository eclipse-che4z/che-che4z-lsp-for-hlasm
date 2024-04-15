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

 //rules for CA operands
parser grammar ca_operand_rules;

ca_op_branch returns [operand_ptr op]
    : expr_list seq_symbol
    {
        collector.add_hl_symbol(token_info(provider.get_range($seq_symbol.ctx),hl_scopes::seq_symbol));

        resolve_expression($expr_list.ca_expr);
        auto r = provider.get_range($expr_list.ctx->getStart(),$seq_symbol.ctx->getStop());
        if (!$seq_symbol.ss.name.empty())
            $op = std::make_unique<branch_ca_operand>(std::move($seq_symbol.ss), std::move($expr_list.ca_expr), r);
    }
    | seq_symbol
    {
        collector.add_hl_symbol(token_info(provider.get_range($seq_symbol.ctx),hl_scopes::seq_symbol));
        if (!$seq_symbol.ss.name.empty())
            $op = std::make_unique<seq_ca_operand>(std::move($seq_symbol.ss),provider.get_range($seq_symbol.ctx));
    };
    finally
    {if (!$op) $op = std::make_unique<semantics::empty_operand>(provider.get_range(_localctx));}


ca_op_expr returns [operand_ptr op]
    : expr_general
    {
        resolve_expression($expr_general.ca_expr);
        $op = std::make_unique<expr_ca_operand>(std::move($expr_general.ca_expr), provider.get_range($expr_general.ctx));
    };
    finally
    {if (!$op) $op = std::make_unique<semantics::empty_operand>(provider.get_range(_localctx));}

ca_op_var_def returns [operand_ptr op]
    : var_def
    {
        $op = std::make_unique<var_ca_operand>(std::move($var_def.vs), provider.get_range($var_def.ctx));
    };
    finally
    {if (!$op) $op = std::make_unique<semantics::empty_operand>(provider.get_range(_localctx));}
