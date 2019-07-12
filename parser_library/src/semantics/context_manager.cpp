#include "context_manager.h"
#include "expression_visitor.h"

namespace hlasm_plugin::parser_library::semantics
{

using namespace context;

context_manager::context_manager(context::ctx_ptr ctx, parse_lib_provider & lib_provider) : diagnosable_ctx(ctx), ctx_(std::move(ctx)), lib_provider_(lib_provider)
{
	init_instr();
}

context_manager::context_manager(context::ctx_ptr ctx) : context_manager::context_manager(ctx, empty_parse_lib_provider::instance)
{
}

hlasm_context & context_manager::ctx()
{
	return *ctx_;
}

SET_t context_manager::get_var_sym_value(var_sym symbol) const
{
	//todo created
	auto id = symbol.created ? get_id(concatenate(std::move(symbol.created_name))) : get_id(std::move(symbol.name));

	std::vector<expr_ptr> subscript;
	for (auto tree : symbol.subscript)
		subscript.push_back(evaluate_expression_tree(tree));

	auto range = std::move(symbol.range);


	auto var = ctx_->get_var_sym(id);
	if (!var)
	{
		add_diagnostic(diagnostic_s::error_E010("", "variable", convert_range(range))); //error - unknown name of variable
		return SET_t();
	}

	if (auto set_sym = var->access_set_symbol_base())
	{
		if (subscript.size() > 1)
		{
			add_diagnostic(diagnostic_s::error_E020("", "variable symbol subscript", convert_range(range))); //error - too many operands
			return SET_t();
		}

		if ((set_sym->is_scalar && subscript.size() == 1) || (!set_sym->is_scalar && subscript.size() == 0))
		{
			add_diagnostic(diagnostic_s::error_E013("", "subscript error", convert_range(range))); //error - inconsistent format of subcript
			return SET_t();
		}

		if (!set_sym->is_scalar && (!subscript[0] || subscript[0]->get_numeric_value() < 1))
		{
			add_diagnostic(diagnostic_s::error_E012("", "subscript value has to be 1 or more", convert_range(range))); //error - subscript is less than 1
			return SET_t();
		}

		size_t idx = 0;
		if (subscript.empty())
		{
			switch (set_sym->type())
			{
			case context::SET_t_enum::A_TYPE:
				return set_sym->access_set_symbol<A_t>()->get_value();
				break;
			case context::SET_t_enum::B_TYPE:
				return set_sym->access_set_symbol<B_t>()->get_value();
				break;
			case context::SET_t_enum::C_TYPE:
				return set_sym->access_set_symbol<C_t>()->get_value();
				break;
			default:
				return SET_t();
				break;
			}
		}
		else
		{
			idx = (size_t)(subscript[0] ? subscript[0]->get_numeric_value() - 1 : 0);

			switch (set_sym->type())
			{
			case context::SET_t_enum::A_TYPE:
				return set_sym->access_set_symbol<A_t>()->get_value(idx);
				break;
			case context::SET_t_enum::B_TYPE:
				return set_sym->access_set_symbol<B_t>()->get_value(idx);
				break;
			case context::SET_t_enum::C_TYPE:
				return set_sym->access_set_symbol<C_t>()->get_value(idx);
				break;
			default:
				return SET_t();
				break;
			}
		}
	}
	else if (auto mac_par = var->access_macro_param_base())
	{
		std::vector<size_t> tmp;
		for (auto& e : subscript)
		{
			if (!e || e->get_numeric_value() < 1)
			{
				add_diagnostic(diagnostic_s::error_E012("", "subscript value has to be 1 or more", convert_range(range))); //error - subscript is less than 1
				return SET_t();
			}
			tmp.push_back((size_t)(e->get_numeric_value() - 1));
		}
		return mac_par->get_value(tmp);
	}
	return SET_t();
}

id_index context_manager::get_id(std::string name) const
{
	//ERR check if valid id
	return ctx_->ids.add(std::move(name));
}

std::string context_manager::to_string(concat_chain chain)
{
	std::string ret;
	for (auto& point : chain)
	{
		switch (point->get_type())
		{
		case concat_type::DOT:
			ret.push_back('.');
			break;
		case concat_type::STR:
			ret.append(std::move(point->access_str()->value));
			break;
		case concat_type::VAR:
			ret.push_back('&');
			ret.append(std::move(point->access_var()->name));
			break;
		default:
			break;
		}
	}
	return ret;
}

std::string context_manager::concatenate(concat_chain chain) const
{
	std::string result;
	bool last_was_var = false;

	for (auto& point : chain)
	{
		if (!point) continue;
		switch (point->get_type())
		{
		case concat_type::STR:
			last_was_var = false;
			result.append(concat(point->access_str()));
			break;
		case concat_type::DOT:
			if (last_was_var) continue;
			last_was_var = false;
			result.append(concat(point->access_dot()));
			break;
		case concat_type::EQUALS:
			last_was_var = false;
			result.append(concat(point->access_equ()));
			break;
		case concat_type::VAR:
			last_was_var = true;
			result.append(concat(point->access_var()));
			break;
		case concat_type::SUB:
			last_was_var = false;
			result.append(concat(point->access_sub()));
			break;
		default:
			break;
		}
	}

	return result;
}

expr_ptr context_manager::evaluate_expression_tree(antlr4::ParserRuleContext * expr_context) const
{
	expression_visitor expression_evaluator(*this);

	auto e = expression_evaluator.visit(expr_context).as<expr_ptr>();

	if (e->diag)
	{
		symbol_range r = symbol_range::get_range(expr_context);
		add_diagnostic(diagnostic_s{ "", {{r.begin_ln, r.begin_col},{r.end_ln, r.end_col}}, e->diag->severity, std::move(e->diag->code), "HLASM Plugin", std::move(e->diag->message), {} });
	}
	return e;
}

hlasm_plugin::parser_library::context::macro_invo_ptr hlasm_plugin::parser_library::semantics::context_manager::enter_macro(context::id_index opcode, label_semantic_info label, operand_remark_semantic_info operands)
{
	auto args = process_macro_args(opcode, std::move(label), std::move(operands));
	auto name_param(std::move(args.back()));
	args.pop_back();
	return ctx().enter_macro(opcode, std::move(name_param.data), std::move(args));
}

std::vector<macro_arg> context_manager::process_macro_args(context::id_index opcode, label_semantic_info label, operand_remark_semantic_info operands)
{
	macro_data_ptr label_value;

	//label
	switch (label.type)
	{
	case label_type::SEQ:
	case label_type::EMPTY:
		label_value = nullptr;
		break;
	case label_type::CONC:
	case label_type::ORD:
	case label_type::MAC:
		label_value = std::make_unique<macro_param_data_single>(std::move(label.name));
		break;
	case label_type::VAR:
		label_value = std::make_unique<macro_param_data_single>(
			get_var_sym_value(std::move(label.variable_symbol)).to<C_t>()
			);
		break;
	default:
		break;
	}

	//op
	std::vector<macro_arg> args;
	std::vector<id_index> keyword_params;

	for (auto& op : operands.operands)
	{
		if (!op || op->type == operand_type::EMPTY)
		{
			args.push_back({ nullptr,nullptr });
			continue;
		}

		auto tmp = op->access_mac_op();
		assert(tmp);

		clear_concat_chain(tmp->chain);

		if (tmp->chain.size() >= 2 &&
			tmp->chain[0]->get_type() == concat_type::STR && tmp->chain[1]->get_type() == concat_type::EQUALS)
		{
			auto id = get_id(std::move(tmp->chain[0]->access_str()->value));
			auto named = ctx().macros().find(opcode)->second->named_params().find(id);
			if (named == ctx().macros().find(opcode)->second->named_params().end() || named->second->param_type() == context::macro_param_type::POS_PAR_TYPE)
			{
				add_diagnostic(diagnostic_s::error_E010("", "keyword parameter", convert_range(tmp->range))); //error - unknown name of keyword parameter

				//MACROCASE TODO
				auto name = std::move(tmp->chain[0]->access_str()->value);

				tmp->chain.erase(tmp->chain.begin());
				tmp->chain.erase(tmp->chain.begin());

				args.push_back({ std::make_unique<macro_param_data_single>(std::move(name) + "=" + to_string(std::move(tmp->chain))),nullptr });
			}
			else
			{
				if (std::find(keyword_params.begin(), keyword_params.end(), id) != keyword_params.end())
				{
					add_diagnostic(diagnostic_s::error_E011("", "Keyword", convert_range(tmp->range))); // error - keyword already defined
				}
				else
				{
					keyword_params.push_back(id);
				}

				tmp->chain.erase(tmp->chain.begin());
				tmp->chain.erase(tmp->chain.begin());

				args.push_back({ create_macro_data(std::move(tmp->chain)),id });
			}
		}
		else
			args.push_back({ create_macro_data(std::move(tmp->chain)) ,nullptr });


	}

	args.push_back({ std::move(label_value),nullptr });

	return args;
}


std::string context_manager::concat(char_str * str) const
{
	return std::move(str->value);
}

std::string context_manager::concat(var_sym* vs) const
{
	return get_var_sym_value(std::move(*vs)).to<C_t>();
}

std::string context_manager::concat(dot*) const
{
	return ".";
}

std::string context_manager::concat(equals *) const
{
	return "=";
}

std::string context_manager::concat(sublist* sublist) const
{
	std::string ret("(");
	for (size_t i = 0; i < sublist->list.size(); ++i)
	{
		if (!sublist->list[i]) continue;

		switch (sublist->list[i]->get_type())
		{
		case concat_type::STR:
			ret.append(concat(sublist->list[i]->access_str()));
			break;
		case concat_type::DOT:
			ret.append(concat(sublist->list[i]->access_dot()));
			break;
		case concat_type::VAR:
			ret.append(concat(sublist->list[i]->access_var()));
			break;
		case concat_type::SUB:
			ret.append(concat(sublist->list[i]->access_sub()));
			break;
		}
		if (i != sublist->list.size() - 1) ret.append(",");
	}
	ret.append(")");
	return ret;
}

void context_manager::init_instr()
{
	if (instructions.size() != 0)
		return;

	for (const auto& mach_instr : instruction::machine_instructions)
	{
		auto id = ctx_->ids.add(mach_instr.first);
		instructions.insert({ id, { instruction_type::MACH, mach_instr.second->operands.size() == 0} });
	}
	for (const auto& asm_instr : instruction::assembler_instructions)
	{
		auto id = ctx_->ids.add(asm_instr.first);
		if (*id == "DC" || *id == "DS")
			instructions.insert({ id, { instruction_type::DAT, asm_instr.second.second == 0} });
		else
			instructions.insert({ id, { instruction_type::ASM, asm_instr.second.second == 0} });
	}
	for (size_t i = 0; i < instruction::ca_instructions.size(); ++i)
	{
		auto id = ctx_->ids.add(instruction::ca_instructions[i]);
		instructions.insert({ id, { instruction_type::CA,instruction::ca_instructions[i] == "ANOP"} });
	}
	for (size_t i = 0; i < instruction::macro_processing_instructions.size(); ++i)
	{
		auto id = ctx().ids.add(instruction::macro_processing_instructions[i]);
		instructions.insert({ id,  {instruction_type::CA,instruction::ca_instructions[i] != "AREAD" && instruction::ca_instructions[i] != "ASPACE" } });
	}
	for (const auto mnemo : instruction::mnemonic_codes)
	{
		auto id = ctx().ids.add(mnemo.first);
		instructions.insert({ id,  {instruction_type::MACH,false} });
	}
}

macro_data_ptr context_manager::create_macro_data(concat_chain chain)
{
	if (chain.size() == 0 || chain.size() > 1 || (chain.size() == 1 && chain[0]->get_type() != concat_type::SUB))
		return std::make_unique<macro_param_data_single>(concatenate(std::move(chain)));

	auto& inner_chain = chain[0]->access_sub()->list;

	std::vector<macro_data_ptr> sublist;

	for (auto& point : inner_chain)
	{
		concat_chain tmp;
		tmp.push_back(std::move(point));
		sublist.push_back(create_macro_data(std::move(tmp)));
	}
	return std::make_unique<macro_param_data_composite>(std::move(sublist));
}

op_code_info context_manager::get_opcode_info(std::string name)
{
	//if contains space
	/*
	auto id = ctx->ids.add(instr);

	macro_def_.defered_ops = curr_statement_.instr_info.type != instruction_type::CA && macro_def_.active;

	if (macro_def_.active && macro_def_.expecting_prototype)
	{
		curr_statement_.instr_info.id = id;
		curr_statement_.instr_info.type = instruction_type::MAC;
		curr_statement_.instr_info.has_no_ops = false;
		curr_statement_.instr_info.has_alt_format = true;
		macro_def_.defered_ops = false;
	}

	auto target = ctx->get_mnemonic_opcode(id);

	if (target == ctx->empty_id)
	{
		//ERROR no op code
		curr_statement_.instr_info.has_no_ops = true;
		curr_statement_.instr_info.id = target;
	}
	else if (target != nullptr)
		id = target;

	//TODO check for :MAC :ASM

	auto instr_info = instructions_.find(id);

	if (instr_info != instructions_.end())
	{
		curr_statement_.instr_info.id = id;
		curr_statement_.instr_info.type = instr_info->second.type;
		curr_statement_.instr_info.has_no_ops = instr_info->second.no_ops;
		curr_statement_.instr_info.has_alt_format = instr_info->second.type == instruction_type::CA || instr_info->second.type == instruction_type::MAC;
	}
	else
	{
		//TODO check for mac libs
		curr_statement_.instr_info.id = id;
		curr_statement_.instr_info.type = instruction_type::MAC;
		curr_statement_.instr_info.has_no_ops = false;
		curr_statement_.instr_info.has_alt_format = true;
	}

		add_diagnostic(diagnostic_s::error_E010("", "operation code", {})); //error - unknown operation code
	*/
	//TODO OPSYN

	if (name.empty())
	{
		op_code_info i;
		i.unknown = false;
		return i;
	}

	auto id = get_id(std::move(name));

	auto info = instructions.find(id);

	if (info != instructions.end())
	{
		return { id, info->second.type,info->second.no_ops };
	}

	if (ctx().macros().find(id) != ctx().macros().end())
	{
		return { id,instruction_type::MAC,false };
	}
	else
	{
		//TODO - library find is called every time an instrucion is parsed - may be very time consuming
		// introduce some list of all available macros.
		lib_provider_.parse_library(*id, ctx_);
		if (ctx_->macros().find(id) != ctx_->macros().end())
		{
			return { id,instruction_type::MAC,false };
		}
	}

	return op_code_info();
}

op_code_info context_manager::get_opcode_info(concat_chain model_name)
{
	return get_opcode_info(concatenate(std::move(model_name)));
}

op_code_info context_manager::get_opcode_info(instruction_semantic_info info)
{
	return info.type == instr_semantic_type::CONC ? get_opcode_info(std::move(info.model_name)) : get_opcode_info(std::move(info.ordinary_name));
}

hlasm_plugin::parser_library::range context_manager::convert_range(symbol_range sr)
{
	return { {sr.begin_ln, sr.begin_col}, {sr.end_ln, sr.end_col} };
}

void context_manager::collect_diags() const
{
}

void context_manager::set_var_sym_value_base(var_sym symbol, context::set_symbol_base* set_sym,std::vector<expr_ptr>& subscript, context::SET_t_enum type)
{
	for (auto tree : symbol.subscript)
		subscript.push_back(evaluate_expression_tree(tree));

	auto range = std::move(symbol.range);

	if (set_sym)
	{
		if (set_sym->type() != type)
		{
			add_diagnostic(diagnostic_s::error_E013("", "wrong type of variable symbol", convert_range(range))); //error - wrong type of variable symbol
		}

		if (subscript.size() > 1)
		{
			add_diagnostic(diagnostic_s::error_E020("", "variable symbol subscript", convert_range(range))); //error - too many operands
		}
		else if ((set_sym->is_scalar && subscript.size() == 1) || (!set_sym->is_scalar && subscript.size() == 0))
		{
			add_diagnostic(diagnostic_s::error_E013("", "subscript error", convert_range(range))); //error - inconsistent format of subcript
		}

		if (!set_sym->is_scalar && subscript.size() != 0 && subscript[0] && subscript[0]->get_numeric_value() < 1)
		{
			add_diagnostic(diagnostic_s::error_E012("", "subscript value has to be 1 or more", convert_range(range))); //error - subscript is less than 1
		}
	}
	else
	{
		add_diagnostic(diagnostic_s::error_E030("", "symbolic parameter", convert_range(range))); //error - can't write to symbolic param
	}
}

}