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

parser grammar ca_operand_rules; 


ca_op returns [operand_ptr op]
	: lpar expr rpar seq_symbol
	{
		collector.add_hl_symbol(token_info(provider.get_range($seq_symbol.ctx),hl_scopes::seq_symbol));
		collector.add_lsp_symbol({*$seq_symbol.ss.name,provider.get_range($seq_symbol.ctx),symbol_type::seq});
		$op = std::make_unique<branch_ca_operand>(std::move($seq_symbol.ss),$expr.ctx,provider.get_range($lpar.ctx->getStart(),$seq_symbol.ctx->getStop()));
	}
	| seq_symbol
	{
		collector.add_hl_symbol(token_info(provider.get_range($seq_symbol.ctx),hl_scopes::seq_symbol));
		collector.add_lsp_symbol({*$seq_symbol.ss.name,provider.get_range($seq_symbol.ctx),symbol_type::seq});
		$op = std::make_unique<seq_ca_operand>(std::move($seq_symbol.ss),provider.get_range($seq_symbol.ctx));
	}
	| expr_p
	{
		if($expr_p.vs_link && is_var_def()) 
		{
			
			auto tmp = std::make_unique<var_ca_operand>(std::move(*$expr_p.vs_link),provider.get_range($expr_p.ctx));
			$op = std::move(tmp);
		}
		else 
		{
			$op = std::make_unique<expr_ca_operand>($expr_p.ctx,provider.get_range($expr_p.ctx));
		}
	};