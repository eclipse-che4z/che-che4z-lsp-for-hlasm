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

 //rules for instruction field
parser grammar instruction_field_rules;

instruction returns [id_index instr]
	: l_string_v			/*model*/
	{
		for(const auto& point : $l_string_v.chain)
		{
			if(point->type != concat_type::STR)
				continue;
			collector.add_hl_symbol(token_info(point->access_str()->conc_range,hl_scopes::instruction));
		}

		collector.set_instruction_field(std::move($l_string_v.chain),provider.get_range($l_string_v.ctx));
	}
	| macro_name
	{
		collector.add_hl_symbol(token_info(provider.get_range($macro_name.ctx),hl_scopes::instruction));
		collector.set_instruction_field(
			parse_identifier(std::move($macro_name.value),provider.get_range($macro_name.ctx)),
			provider.get_range( $macro_name.ctx));
	}
	| ORDSYMBOL
	{
		collector.add_hl_symbol(token_info(provider.get_range($ORDSYMBOL),hl_scopes::instruction));
		auto instr_id = parse_identifier($ORDSYMBOL->getText(),provider.get_range($ORDSYMBOL));
		collector.set_instruction_field(
			instr_id,
			provider.get_range($ORDSYMBOL));
	}
	| bad_instr
	{
		collector.add_hl_symbol(token_info(provider.get_range($bad_instr.ctx),hl_scopes::instruction));
		collector.set_instruction_field(
			hlasm_ctx->ids().add($bad_instr.ctx->getText()),
			provider.get_range($bad_instr.ctx));
	};

macro_name_b returns [std::string value]
	: l_ch													{$value = std::move($l_ch.value);}
	| tmp=macro_name_b l_ch									{$value = std::move($tmp.value); $value.append(std::move($l_ch.value));};

macro_name returns [std::string value]
	: ORDSYMBOL macro_name_b								{$value = $ORDSYMBOL->getText(); $value.append(std::move($macro_name_b.value));};

bad_instr
	: IDENTIFIER ~(SPACE)*;