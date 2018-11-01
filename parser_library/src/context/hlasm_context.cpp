#include "hlasm_context.h"
#include "instruction.h"

using namespace hlasm_plugin::parser_library::context;

code_scope * hlasm_plugin::parser_library::context::hlasm_context::curr_scope()
{
	return &scope_stack_.top();
}


hlasm_plugin::parser_library::context::hlasm_context::hlasm_context() : empty_id(ids.add(object_traits<C_t>::default_v()))
{
	scope_stack_.push(code_scope());
}

const code_scope::set_sym_storage & hlasm_plugin::parser_library::context::hlasm_context::globals() const
{
	return globals_;
}

var_sym_ptr hlasm_plugin::parser_library::context::hlasm_context::get_var_sym(id_index name)
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

void hlasm_plugin::parser_library::context::hlasm_context::add_sequence_symbol(sequence_symbol seq_sym)
{
	if (curr_scope()->sequence_symbols.find(seq_sym.name) == curr_scope()->sequence_symbols.end())
		curr_scope()->sequence_symbols.insert({ seq_sym.name,seq_sym });
	else
	{
		//ERR already defined sequence symbol
	}
}


sequence_symbol hlasm_plugin::parser_library::context::hlasm_context::get_sequence_symbol(id_index name)
{
	auto tmp = curr_scope()->sequence_symbols.find(name);
	if (tmp != curr_scope()->sequence_symbols.end())
		return tmp->second;
	else
		return sequence_symbol::EMPTY;
}

void hlasm_plugin::parser_library::context::hlasm_context::set_branch_counter(int value)
{
	curr_scope()->branch_counter = value;
}

int hlasm_plugin::parser_library::context::hlasm_context::get_branch_counter()
{
	return curr_scope()->branch_counter;
}

void hlasm_plugin::parser_library::context::hlasm_context::decrement_branch_counter()
{
	--curr_scope()->branch_counter;
}

void hlasm_plugin::parser_library::context::hlasm_context::add_mnemonic(id_index mnemo, id_index op_code)
{
	auto tmp = opcode_mnemo_.find(op_code);
	if (tmp != opcode_mnemo_.end())
		opcode_mnemo_.insert_or_assign(mnemo, tmp->second);
	else
	{
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

		if (macros_.find(op_code) != macros_.end())
		{
			opcode_mnemo_.insert_or_assign(mnemo, op_code);
			return;
		}
		//ERROR not known op code
	}
}

void hlasm_plugin::parser_library::context::hlasm_context::remove_mnemonic(id_index mnemo)
{
	opcode_mnemo_.insert_or_assign(mnemo, empty_id);
}

id_index hlasm_plugin::parser_library::context::hlasm_context::get_mnemonic_opcode(id_index mnemo) const
{
	auto tmp = opcode_mnemo_.find(mnemo);
	if (tmp != opcode_mnemo_.end())
		return tmp->second;
	else
		return nullptr;
}

const macro_definition& hlasm_plugin::parser_library::context::hlasm_context::add_macro(id_index name, id_index label_param_name, std::vector<macro_arg> params, antlr4::ParserRuleContext * derivation_tree)
{
	return macros_.insert_or_assign(name, macro_definition(name, label_param_name, std::move(params), derivation_tree)).first->second;
}

bool hlasm_plugin::parser_library::context::hlasm_context::is_in_macro() const
{
	return scope_stack_.top().is_in_macro();
}

macro_invo_ptr hlasm_context::enter_macro(id_index name, macro_data_ptr label_param_data, std::vector<macro_arg>  params)
{
	auto tmp = macros_.find(name);
	if (tmp != macros_.end())
	{
		auto invo((tmp->second.call(std::move(label_param_data), std::move(params))));
		scope_stack_.push(code_scope(invo));
		return invo;
	}
	else
	{
		return macro_invo_ptr();
		//TODO look for libraries
	}
}

void hlasm_plugin::parser_library::context::hlasm_context::leave_macro()
{
	scope_stack_.pop();
}

macro_invo_ptr hlasm_plugin::parser_library::context::hlasm_context::this_macro()
{
	if (is_in_macro())
		return curr_scope()->this_macro;
	return macro_invo_ptr();
}
