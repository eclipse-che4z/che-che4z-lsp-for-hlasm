#include "ordinary_processor.h"
#include "../statement.h"

#include "../../checking/instruction_checker.h"

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::processing;

ordinary_processor::ordinary_processor(
	context::hlasm_context& hlasm_ctx,
	attribute_provider& attr_provider,
	branching_provider& branch_provider,
	parse_lib_provider& lib_provider,
	processing_state_listener& state_listener,
	statement_fields_parser& parser)
	:statement_processor(processing_kind::ORDINARY, hlasm_ctx),
	eval_ctx{ hlasm_ctx, attr_provider,lib_provider },
	ca_proc_(hlasm_ctx, attr_provider, branch_provider, lib_provider, state_listener),
	mac_proc_(hlasm_ctx, attr_provider, branch_provider, lib_provider),
	asm_proc_(hlasm_ctx, attr_provider, branch_provider, lib_provider, parser),
	mach_proc_(hlasm_ctx, attr_provider, branch_provider, lib_provider, parser),
	finished_flag_(false) {}

processing_status ordinary_processor::get_processing_status(const semantics::instruction_si& instruction) const
{
	context::id_index id;
	if (instruction.type == semantics::instruction_si_type::CONC)
		id = context_manager(hlasm_ctx).concatenate(std::get<semantics::concat_chain>(instruction.value),eval_ctx);
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
	auto valid_layout = check_layout();

	if (valid_layout)
	{
		hlasm_ctx.ord_ctx.finish_module_layout();
		hlasm_ctx.ord_ctx.symbol_dependencies.add_defined();
	}

	check_postponed_statements(hlasm_ctx.ord_ctx.symbol_dependencies.collect_postponed());

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

bool ordinary_processor::check_layout()
{
	for (auto& sect : hlasm_ctx.ord_ctx.sections())
	{
		for (size_t i = 0; i < sect->location_counters().size(); i++)
		{
			if (i == 0)
				continue;

			if (sect->location_counters()[i - 1]->has_undefined_layout())
				return false;
		}
	}
	return true;
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
