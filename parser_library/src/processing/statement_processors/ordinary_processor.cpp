#include "ordinary_processor.h"
#include "../statement.h"

#include "../../checking/instruction_checker.h"

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::processing;

ordinary_processor::ordinary_processor(
	context::hlasm_context& hlasm_ctx, 
	parse_lib_provider& lib_provider,
	branching_provider& branch_provider,
	processing_state_listener& state_listener,
	statement_field_reparser& parser)
	:statement_processor(processing_kind::ORDINARY, hlasm_ctx),
	lib_provider_(lib_provider),
	ca_proc_(hlasm_ctx,branch_provider,state_listener),
	mac_proc_(hlasm_ctx),
	asm_proc_(hlasm_ctx,branch_provider, lib_provider,parser),
	mach_proc_(hlasm_ctx,branch_provider,parser),
	finished_flag_(false) {}

processing_status ordinary_processor::get_processing_status(const semantics::instruction_si& instruction) const
{
	context::id_index id;
	if (instruction.type == semantics::instruction_si_type::CONC)
		id = context_manager(hlasm_ctx).concatenate(std::get<semantics::concat_chain>(instruction.value));
	else
		id =  std::get<context::id_index>(instruction.value);

	if (hlasm_ctx.macros().find(id) != hlasm_ctx.macros().end())
	{
		return std::make_pair(
			processing_format(processing_kind::ORDINARY, processing_form::MAC), 
			op_code(id,context::instruction_type::MAC)
		);
	}

	auto status = get_instruction_processing_status(id, hlasm_ctx);
	
	if (!status)
	{
		auto found = lib_provider_.parse_library(*id, hlasm_ctx, library_data{ context::file_processing_type::MACRO,id });
		processing_form f;
		context::instruction_type t;
		if (found)
		{
			f = processing_form::MAC;
			t = context::instruction_type::MAC;
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
	if (hlasm_ctx.current_file_proc_type() != context::file_processing_type::OPENCODE)
	{
		hlasm_ctx.pop_processing_file();
		finished_flag_ = true;
		return;
	}

	auto [loctr_offsets,valid_layout] = check_layout();

	if (valid_layout)
	{
		hlasm_ctx.ord_ctx.finish_module_layout();
		hlasm_ctx.ord_ctx.symbol_dependencies.add_defined(loctr_offsets);
	}

	check_postponed_statements(hlasm_ctx.ord_ctx.symbol_dependencies.collect_all());

	hlasm_ctx.pop_processing_file();
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
				op_code(instruction,context::instruction_type::CA));
		else
			return std::nullopt;
	}

	auto id = iter->first;
	auto arr = iter->second;
	processing_form f= processing_form::UNKNOWN;
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
		o = context::instruction::mnemonic_codes.find(*id)->second.replaced.empty() ? operand_occurence::ABSENT : operand_occurence::PRESENT;
		t = context::instruction_type::MACH;
		break;
	default:
		assert(false); //opcode should already be found
		break;
	}

	return std::make_pair(processing_format(processing_kind::ORDINARY, f, o), op_code(instruction,t));
}

void ordinary_processor::collect_diags() const
{
	collect_diags_from_child(ca_proc_);
	collect_diags_from_child(asm_proc_);
	collect_diags_from_child(mac_proc_);
	collect_diags_from_child(mach_proc_);
}

std::pair<std::vector<context::id_index>, bool> ordinary_processor::check_layout()
{
	std::vector<context::id_index> ret;
	for (auto& sect : hlasm_ctx.ord_ctx.sections())
	{
		for (size_t i = 0; i < sect->location_counters().size(); i++)
		{
			if (i == 0)
				continue;

			if (sect->location_counters()[i - 1]->has_undefined_layout())
				return { {},false };

			if (i != 0)
				ret.push_back(sect->location_counters()[i]->spaces()[0]->name);
		}
	}
	return { ret,true };
}

void ordinary_processor::check_postponed_statements(std::vector<context::post_stmt_ptr> stmts)
{
	checking::assembler_checker asm_checker;
	checking::machine_checker mach_checker;

	for (auto& stmt : stmts)
	{
		assert(stmt->opcode_ref().type == context::instruction_type::ASM || stmt->opcode_ref().type == context::instruction_type::MACH);

		if (stmt->opcode_ref().type == context::instruction_type::ASM)
			low_language_processor::check(*stmt,hlasm_ctx, asm_checker,*this);
		else
			low_language_processor::check(*stmt, hlasm_ctx, mach_checker,*this);
	}
}
