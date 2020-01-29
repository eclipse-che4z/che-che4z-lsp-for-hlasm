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

parser grammar instruction_field_rules;

instruction returns [id_index instr]
	: l_string_v			/*model*/							
	{
		collector.set_instruction_field(std::move($l_string_v.chain),provider.get_range( $l_string_v.ctx));
		process_instruction();
	}
	| macro_name												
	{
		collector.set_instruction_field(
			parse_identifier(std::move($macro_name.value),provider.get_range($macro_name.ctx)),
			provider.get_range( $macro_name.ctx));
		process_instruction();
	}
	| ORDSYMBOL													
	{
		auto instr_id = parse_identifier($ORDSYMBOL->getText(),provider.get_range($ORDSYMBOL));
		collector.add_lsp_symbol(instr_id,provider.get_range( $ORDSYMBOL),symbol_type::instruction);
		collector.set_instruction_field(
			instr_id,
			provider.get_range($ORDSYMBOL));
		process_instruction();
	}
	| bad_instr
	{
		collector.set_instruction_field(
			ctx->ids().add($bad_instr.ctx->getText()),
			provider.get_range($bad_instr.ctx));
		process_instruction();
	};

macro_name_b returns [std::string value]
	: l_ch													{$value = std::move($l_ch.value);}
	| tmp=macro_name_b l_ch									{$value = std::move($tmp.value); $value.append(std::move($l_ch.value));};

macro_name returns [std::string value]
	: ORDSYMBOL macro_name_b								{$value = std::move($ORDSYMBOL->getText()); $value.append(std::move($macro_name_b.value));};

bad_instr
	: IDENTIFIER ~(SPACE|EOLLN)*;