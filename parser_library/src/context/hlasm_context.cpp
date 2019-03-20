#include "hlasm_context.h"
#include "instruction.h"
#include "../diagnosable_impl.h"

namespace hlasm_plugin::parser_library::context
{

code_scope * hlasm_context::curr_scope()
{
	return &scope_stack_.top();
}


hlasm_context::hlasm_context() : empty_id(ids.add(object_traits<C_t>::default_v()))
{
	scope_stack_.push(code_scope());
}

const code_scope::set_sym_storage & hlasm_context::globals() const
{
	return globals_;
}

var_sym_ptr hlasm_context::get_var_sym(id_index name)
{
	auto tmp = curr_scope()->variables.find(name);
	if (tmp != curr_scope()->variables.end())
		return tmp->second;
	else if (curr_scope()->is_in_macro())
	{
		auto m_tmp = curr_scope()->this_macro->named_params.find(name);
		if (m_tmp != curr_scope()->this_macro->named_params.end())
			return m_tmp->second;
	}

	return var_sym_ptr();
}

void hlasm_context::add_sequence_symbol(sequence_symbol seq_sym)
{
	if (curr_scope()->sequence_symbols.find(seq_sym.name) == curr_scope()->sequence_symbols.end())
		curr_scope()->sequence_symbols.insert({ seq_sym.name,seq_sym });
}


sequence_symbol hlasm_context::get_sequence_symbol(id_index name)
{
	auto tmp = curr_scope()->sequence_symbols.find(name);
	if (tmp != curr_scope()->sequence_symbols.end())
		return tmp->second;
	else
		return sequence_symbol::EMPTY;
}

void hlasm_context::set_branch_counter(int value)
{
	curr_scope()->branch_counter = value;
}

int hlasm_context::get_branch_counter()
{
	return curr_scope()->branch_counter;
}

void hlasm_context::decrement_branch_counter()
{
	--curr_scope()->branch_counter;
}

void hlasm_context::add_mnemonic(id_index mnemo, id_index op_code)
{
	auto tmp = opcode_mnemo_.find(op_code);
	if (tmp != opcode_mnemo_.end())
		opcode_mnemo_.insert_or_assign(mnemo, tmp->second);
	else
	{
		if (macros_.find(op_code) != macros_.end())
		{
			opcode_mnemo_.insert_or_assign(mnemo, op_code);
			return;
		}

		auto ca_instr = std::find(instruction::ca_instructions.begin(), instruction::ca_instructions.end(), *op_code);
		if (ca_instr != instruction::ca_instructions.end())
		{
			opcode_mnemo_.insert_or_assign(mnemo, op_code);
			return;
		}

		auto asm_instr = std::find_if(instruction::assembler_instructions.begin(), instruction::assembler_instructions.end(), [=](auto& instr) { return instr.name == *op_code; });
		if (asm_instr != instruction::assembler_instructions.end())
		{
			opcode_mnemo_.insert_or_assign(mnemo, op_code);
			return;
		}

		auto mach_instr = std::find_if(instruction::machine_instructions.begin(), instruction::machine_instructions.end(), [=](auto& instr) { return instr.name == *op_code; });
		if (mach_instr != instruction::machine_instructions.end())
		{
			opcode_mnemo_.insert_or_assign(mnemo, op_code);
			return;
		}

		throw std::invalid_argument("undefined operation code");
	}
}

void hlasm_context::remove_mnemonic(id_index mnemo)
{
	opcode_mnemo_.insert_or_assign(mnemo, empty_id);
}

id_index hlasm_context::get_mnemonic_opcode(id_index mnemo) const
{
	auto tmp = opcode_mnemo_.find(mnemo);
	if (tmp != opcode_mnemo_.end())
		return tmp->second;
	else
		return nullptr;
}

const macro_definition& hlasm_plugin::parser_library::context::hlasm_context::add_macro(id_index name, id_index label_param_name, std::vector<macro_arg> params, semantics::statement_block definition, location location)
{
	return *macros_.insert_or_assign(name, std::make_unique< macro_definition>(name, label_param_name, std::move(params), std::move(definition), location)).first->second.get();
}

const hlasm_plugin::parser_library::context::hlasm_context::macro_storage& hlasm_plugin::parser_library::context::hlasm_context::macros()
{
	return macros_;
}

bool hlasm_context::is_in_macro() const
{
	return scope_stack_.top().is_in_macro();
}

macro_invo_ptr hlasm_context::enter_macro(id_index name, macro_data_ptr label_param_data, std::vector<macro_arg>  params)
{
	auto tmp = macros_.find(name);
	if (tmp != macros_.end())
	{
		auto invo((tmp->second->call(std::move(label_param_data), std::move(params))));
		scope_stack_.push(code_scope(invo));
		return invo;
	}
	else
	{
		return macro_invo_ptr();
		//TODO look for libraries
	}
}

void hlasm_context::leave_macro()
{
	scope_stack_.pop();
}

macro_invo_ptr hlasm_context::this_macro()
{
	if (is_in_macro())
		return curr_scope()->this_macro;
	return macro_invo_ptr();
}
}
