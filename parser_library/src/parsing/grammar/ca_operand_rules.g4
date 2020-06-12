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


ca_op returns [operand_ptr op]
	: expr_list seq_symbol
	{
		collector.add_hl_symbol(token_info(provider.get_range($seq_symbol.ctx),hl_scopes::seq_symbol));
		collector.add_lsp_symbol($seq_symbol.ss.name,provider.get_range($seq_symbol.ctx),symbol_type::seq);

		auto r = provider.get_range($expr_list.ctx->getStart(),$seq_symbol.ctx->getStop());
		$op = std::make_unique<branch_ca_operand>(std::move($seq_symbol.ss), std::move($expr_list.ca_expr), r);
	}
	| seq_symbol
	{
		collector.add_hl_symbol(token_info(provider.get_range($seq_symbol.ctx),hl_scopes::seq_symbol));
		collector.add_lsp_symbol($seq_symbol.ss.name,provider.get_range($seq_symbol.ctx),symbol_type::seq);
		$op = std::make_unique<seq_ca_operand>(std::move($seq_symbol.ss),provider.get_range($seq_symbol.ctx));
	}
	| {!is_var_def()}? expr
	{
		$op = std::make_unique<expr_ca_operand>(std::move($expr.ca_expr), provider.get_range($expr.ctx));
	}
	| { is_var_def()}? var_def
	{
		$op = std::make_unique<var_ca_operand>(std::move($var_def.vs), provider.get_range($var_def.ctx));
	};