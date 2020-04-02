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

#include "ordinary_processor.h"
#include "../statement.h"

#include "../../checking/instruction_checker.h"
#include "../../ebcdic_encoding.h"

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::processing;

ordinary_processor::ordinary_processor(
	context::hlasm_context& hlasm_ctx,
	attribute_provider& attr_provider,
	branching_provider& branch_provider,
	parse_lib_provider& lib_provider,
	processing_state_listener& state_listener,
	statement_fields_parser& parser,
	processing_tracer * tracer)
	:statement_processor(processing_kind::ORDINARY, hlasm_ctx),
	eval_ctx{ hlasm_ctx, attr_provider,lib_provider },
	ca_proc_(hlasm_ctx, attr_provider, branch_provider, lib_provider, state_listener),
	mac_proc_(hlasm_ctx, attr_provider, branch_provider, lib_provider),
	asm_proc_(hlasm_ctx, attr_provider, branch_provider, lib_provider, parser),
	mach_proc_(hlasm_ctx, attr_provider, branch_provider, lib_provider, parser),
	finished_flag_(false),
	tracer_(tracer){}

processing_status ordinary_processor::get_processing_status(const semantics::instruction_si& instruction) const
{
	context::id_index id;
	if (instruction.type == semantics::instruction_si_type::CONC)
		id = resolve_instruction(std::get<semantics::concat_chain>(instruction.value), instruction.field_range);
	else
		id = std::get<context::id_index>(instruction.value);

	if (hlasm_ctx.macros().find(id) != hlasm_ctx.macros().end())
	{
		return std::make_pair(
			processing_format(processing_kind::ORDINARY, processing_form::MAC),
			op_code(id, context::instruction_type::MAC)
		);
	}

	auto status = get_instruction_processing_status(id, hlasm_ctx);

	if (!status)
	{
		auto found = eval_ctx.lib_provider.parse_library(*id, hlasm_ctx, library_data{ processing_kind::MACRO,id });
		processing_form f;
		context::instruction_type t;
		if (found)
		{
			f = processing_form::MAC;
			t = (hlasm_ctx.macros().find(id) != hlasm_ctx.macros().end()) ?
				context::instruction_type::MAC : context::instruction_type::UNDEF;
		}
		else
		{
			f = processing_form::UNKNOWN;
			t = context::instruction_type::UNDEF;
		}
		return std::make_pair(processing_format(processing_kind::ORDINARY, f), op_code(id, t));
	}
	else
		return *status;
}

void ordinary_processor::process_statement(context::shared_stmt_ptr statement)
{
	process_statement_base(statement);
}

void ordinary_processor::process_statement(context::unique_stmt_ptr statement)
{
	process_statement_base(std::move(statement));
}

void ordinary_processor::end_processing()
{
	hlasm_ctx.ord_ctx.finish_module_layout();

	check_postponed_statements(hlasm_ctx.ord_ctx.symbol_dependencies.collect_postponed());
	collect_ordinary_symbol_definitions();

	hlasm_ctx.pop_statement_processing();
	finished_flag_ = true;
}

bool ordinary_processor::terminal_condition(const statement_provider_kind prov_kind) const
{
	return prov_kind == statement_provider_kind::OPEN;
}

bool ordinary_processor::finished()
{
	return finished_flag_;
}

std::optional<processing_status> ordinary_processor::get_instruction_processing_status(context::id_index instruction, context::hlasm_context& hlasm_ctx)
{
	auto iter = hlasm_ctx.instruction_map().find(instruction);

	if (iter == hlasm_ctx.instruction_map().end())
	{
		if (instruction == context::id_storage::empty_id)
			return std::make_pair(
				processing_format(processing_kind::ORDINARY, processing_form::CA, operand_occurence::ABSENT),
				op_code(instruction, context::instruction_type::CA));
		else
			return std::nullopt;
	}

	auto id = iter->first;
	auto arr = iter->second;
	processing_form f = processing_form::UNKNOWN;
	operand_occurence o = operand_occurence::PRESENT;
	context::instruction_type t = context::instruction_type::UNDEF;
	switch (arr)
	{
	case context::instruction::instruction_array::ASM:
		if (instruction == hlasm_ctx.ids().add("DC") || instruction == hlasm_ctx.ids().add("DS"))
			f = processing_form::DAT;
		else
			f = processing_form::ASM;
		o = context::instruction::assembler_instructions.find(*id)->second.max_operands == 0 ? operand_occurence::ABSENT : operand_occurence::PRESENT;
		t = context::instruction_type::ASM;
		break;
	case context::instruction::instruction_array::MACH:
		f = processing_form::MACH;
		o = context::instruction::machine_instructions.find(*id)->second->operands.empty() ? operand_occurence::ABSENT : operand_occurence::PRESENT;
		t = context::instruction_type::MACH;
		break;
	case context::instruction::instruction_array::CA:
		f = processing_form::CA;
		o = std::find_if(context::instruction::ca_instructions.begin(), context::instruction::ca_instructions.end(),
			[&](auto& instr) {return instr.name == *id; })->operandless ? operand_occurence::ABSENT : operand_occurence::PRESENT;
		t = context::instruction_type::CA;
		break;
	case context::instruction::instruction_array::MNEM:
		f = processing_form::MACH;
		o = (context::instruction::machine_instructions.at(context::instruction::mnemonic_codes.at(*id).instruction)->operands.size()
			+ context::instruction::machine_instructions.at(context::instruction::mnemonic_codes.at(*id).instruction)->no_optional
			- context::instruction::mnemonic_codes.at(*id).replaced.size() == 0) //counting  number of operands in mnemonic
			? operand_occurence::ABSENT : operand_occurence::PRESENT;
		t = context::instruction_type::MACH;
		break;
	default:
		assert(false); //opcode should already be found
		break;
	}

	return std::make_pair(processing_format(processing_kind::ORDINARY, f, o), op_code(instruction, t));
}

void ordinary_processor::collect_diags() const
{
	collect_diags_from_child(ca_proc_);
	collect_diags_from_child(asm_proc_);
	collect_diags_from_child(mac_proc_);
	collect_diags_from_child(mach_proc_);
}

void ordinary_processor::check_postponed_statements(std::vector<context::post_stmt_ptr> stmts)
{
	checking::assembler_checker asm_checker;
	checking::machine_checker mach_checker;

	for (auto& stmt : stmts)
	{
		if (!stmt) continue;

		assert(stmt->opcode_ref().type == context::instruction_type::ASM || stmt->opcode_ref().type == context::instruction_type::MACH);

		if (stmt->opcode_ref().type == context::instruction_type::ASM)
			low_language_processor::check(*stmt, hlasm_ctx, asm_checker, *this);
		else
			low_language_processor::check(*stmt, hlasm_ctx, mach_checker, *this);
	}
}

bool ordinary_processor::check_fatals(range line_range)
{
	if (hlasm_ctx.scope_stack().size() > NEST_LIMIT)
	{
		while (hlasm_ctx.is_in_macro())
			hlasm_ctx.leave_macro();

		add_diagnostic(diagnostic_op::error_E055(line_range));
		return true;
	}

	if (hlasm_ctx.get_branch_counter() < 0)
	{
		add_diagnostic(diagnostic_op::error_E056(line_range));
		if (hlasm_ctx.is_in_macro())
			hlasm_ctx.leave_macro();
		else
			finished_flag_ = true;

		return true;
	}

	if (hlasm_ctx.scope_stack().back().branch_counter_change > ACTR_LIMIT)
	{
		add_diagnostic(diagnostic_op::error_E063(line_range));
		finished_flag_ = true;
		return true;
	}

	return false;
}

context::id_index ordinary_processor::resolve_instruction(const semantics::concat_chain& chain, range instruction_range) const
{
	context_manager ctx_mngr(hlasm_ctx);
	auto tmp = ctx_mngr.concatenate_str(chain, eval_ctx);
	collect_diags_from_child(ctx_mngr);

	//trimright
	while (tmp.size() && tmp.back() == ' ')
		tmp.pop_back();

	//trimleft
	size_t i;
	for (i = 0; i < tmp.size() && tmp[i] == ' '; i++);
	tmp.erase(0U, i);

	if (tmp.find(' ') != std::string::npos)
	{
		add_diagnostic(diagnostic_op::error_E067(instruction_range));
		return context::id_storage::empty_id;
	}

	return hlasm_ctx.ids().add(std::move(tmp));
}

void ordinary_processor::collect_ordinary_symbol_definitions()
{
	// for all collected ordinary symbol definitions
	for (const auto& symbol : hlasm_ctx.lsp_ctx->deferred_ord_defs)
	{
		// get the symbol id
		auto id = hlasm_ctx.ids().find(*symbol.name);
		// find symbol in ord context
		auto ord_symbol = hlasm_ctx.ord_ctx.get_symbol(id);
		// if not found, skip it
		if (!ord_symbol)
			continue;

		// add new definition
		auto file = hlasm_ctx.ids().add(ord_symbol->symbol_location.file, true);
		auto occurences = &hlasm_ctx.lsp_ctx->ord_symbols[context::ord_definition(
			symbol.name,
			file,
			symbol.definition_range,
			ord_symbol->value(),
			ord_symbol->attributes())];
		occurences->push_back({ symbol.definition_range,file });

		// adds all its occurences
		for (auto& occurence : hlasm_ctx.lsp_ctx->deferred_ord_occs)
		{
			if (occurence.first.name == symbol.name)
			{
				occurences->push_back({ occurence.first.definition_range,
					occurence.first.file_name });
				occurence.second = true;
			}

		}
	}

	std::vector<std::pair<context::ord_definition, bool>> temp_occs;
	// if there are still some symbols in occurences, check if they are defined in context
	for (const auto& occurence : hlasm_ctx.lsp_ctx->deferred_ord_occs)
	{
		if (occurence.second)
			continue;
		// get the symbol id
		auto id = hlasm_ctx.ids().find(*occurence.first.name);
		// find symbol in ord context
		auto ord_symbol = hlasm_ctx.ord_ctx.get_symbol(id);
		// if not found, skip it
		if (!ord_symbol)
		{
			temp_occs.push_back(occurence);
			continue;
		}

		// add
		hlasm_ctx.lsp_ctx->ord_symbols[context::ord_definition(
			occurence.first.name,
			hlasm_ctx.ids().add(ord_symbol->symbol_location.file, true),
			{ ord_symbol->symbol_location.pos,ord_symbol->symbol_location.pos },
			ord_symbol->value(),
			ord_symbol->attributes())].push_back(
				{ occurence.first.definition_range,
					occurence.first.file_name });
	}
	hlasm_ctx.lsp_ctx->deferred_ord_occs = std::move(temp_occs);
}