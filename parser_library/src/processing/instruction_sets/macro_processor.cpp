#include "macro_processor.h"
#include "../context_manager.h"
#include "../statement_processors/macrodef_processor.h"
#include <memory>

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::processing;

macro_processor::macro_processor(context::hlasm_context& hlasm_ctx,
	attribute_provider& attr_provider, branching_provider& branch_provider, parse_lib_provider& lib_provider)
	:instruction_processor(hlasm_ctx, attr_provider, branch_provider, lib_provider) {}

void macro_processor::process(context::shared_stmt_ptr stmt)
{
	auto args = get_args(*stmt->access_resolved());

	hlasm_ctx.enter_macro(stmt->access_resolved()->opcode_ref().value, std::move(args.name_param), std::move(args.symbolic_params));
}


void macro_processor::process(context::unique_stmt_ptr stmt)
{
	auto args = get_args(*stmt->access_resolved());

	hlasm_ctx.enter_macro(stmt->access_resolved()->opcode_ref().value, std::move(args.name_param), std::move(args.symbolic_params));
}

std::unique_ptr<context::macro_param_data_single> find_single_macro_param(const std::string& data, size_t& start)
{
	size_t begin = start;

	while (true)
	{
		start = data.find_first_of(",'()", start);

		if (start == std::string::npos)
			return nullptr;

		if (data[start] == '(')
		{
			size_t nest = 1;
			while (nest != 0)
			{
				++start;
				if (start == data.size())
					return nullptr;

				if (data[start] == '(') ++nest;
				if (data[start] == ')') --nest;
			}
			++start;
		}
		else if (data[start] == '\'')
		{
			start = data.find_first_of('\'', start + 1);

			if (start == std::string::npos)
				return nullptr;
			
			++start;
		}
		else
			break;
	}

	auto tmp_start = start;
	if (data[start] == ',')
		++start;

	return std::make_unique<context::macro_param_data_single>(data.substr(begin, tmp_start - begin));
}

context::macro_data_ptr macro_processor::string_to_macrodata(std::string data)
{
	if(data.empty())
		return std::make_unique<context::macro_param_data_dummy>();

	if (data.front() != '(' || data.back() != ')')
		return std::make_unique<context::macro_param_data_single>(std::move(data));


	std::stack<size_t> nests;
	std::stack<std::vector<context::macro_data_ptr>> macro_data;

	nests.push(0);
	macro_data.emplace();

	while (true)
	{
		auto begin = nests.top();

		if (begin == data.size())
			return std::make_unique<context::macro_param_data_single>(std::move(data));

		if (data[begin] == '(')
		{
			nests.push(begin + 1);
			macro_data.emplace();
		}
		else if (data[begin] == ')')
		{
			++begin;
			nests.pop();

			auto vec = std::move(macro_data.top());
			macro_data.pop();

			if (begin != data.size() && data[begin] != ',' && data[begin] != ')')
			{
				auto tmp_single = find_single_macro_param(data, begin);

				if(tmp_single == nullptr)
					return std::make_unique<context::macro_param_data_single>(std::move(data));

				auto single = context::macro_param_data_composite(std::move(vec)).get_value() + tmp_single->get_value();

				macro_data.top().emplace_back(std::make_unique<context::macro_param_data_single>(std::move(single)));
			}
			else
				macro_data.top().emplace_back(std::make_unique<context::macro_param_data_composite>(std::move(vec)));

			nests.top() = begin + (begin != data.size() && data[begin] == ',' ? 1 : 0);

			if (nests.size() == 1)
			{
				break;
			}
		}
		else 
		{
			macro_data.top().push_back(find_single_macro_param(data,begin));
			nests.top() = begin;

			if(macro_data.top().back() == nullptr)
				return std::make_unique<context::macro_param_data_single>(std::move(data));
		}
	}

	if(nests.top() != data.size())
		return std::make_unique<context::macro_param_data_single>(std::move(data));

	assert(macro_data.size() == 1 && macro_data.top().size() == 1);

	return std::move(macro_data.top().front());
}

macro_arguments macro_processor::get_args(const resolved_statement& statement) const
{
	context_manager mngr(hlasm_ctx);

	context::macro_data_ptr label_value;

	macro_arguments args;

	//label
	switch (statement.label_ref().type)
	{
	case semantics::label_si_type::SEQ:
	case semantics::label_si_type::EMPTY:
		label_value = nullptr;
		break;
	case semantics::label_si_type::CONC:
		label_value = std::make_unique<context::macro_param_data_single>(
			mngr.concatenate_str(std::get<semantics::concat_chain>(statement.label_ref().value),eval_ctx)
			);
		break;
	case semantics::label_si_type::ORD:
	case semantics::label_si_type::MAC:
		label_value = std::make_unique<context::macro_param_data_single>(std::get<std::string>(statement.label_ref().value));
		break;
	case semantics::label_si_type::VAR:
		label_value = std::make_unique<context::macro_param_data_single>(
			mngr.convert_to<context::C_t>(
				mngr.get_var_sym_value(*std::get<semantics::vs_ptr>(statement.label_ref().value), eval_ctx),
				std::get<semantics::vs_ptr>(statement.label_ref().value)->symbol_range
				)
			);
		break;
	default:
		break;
	}

	args.name_param =  std::move(label_value);

	//op
	std::vector<context::id_index> keyword_params;

	for (const auto& op : statement.operands_ref().value)
	{
		if (op->type == semantics::operand_type::EMPTY)
		{
			args.symbolic_params.push_back({ std::make_unique<context::macro_param_data_dummy>(),nullptr });
			continue;
		}

		auto tmp = op->access_mac();
		assert(tmp);

		semantics::concatenation_point::clear_concat_chain(tmp->chain);

		if (tmp->chain.size() >= 2 &&
			tmp->chain[0]->type == semantics::concat_type::STR && tmp->chain[1]->type == semantics::concat_type::EQU
			&& context_manager(mngr).try_get_symbol_name(tmp->chain[0]->access_str()->value, range()).first)
		{
			auto [valid, id] = mngr.try_get_symbol_name(tmp->chain[0]->access_str()->value, op->operand_range);
			assert(valid);
			auto named = hlasm_ctx.macros().find(statement.opcode_ref().value)->second->named_params().find(id);
			if (named == hlasm_ctx.macros().find(statement.opcode_ref().value)->second->named_params().end() || named->second->param_type == context::macro_param_type::POS_PAR_TYPE)
			{
				add_diagnostic(diagnostic_op::error_E010("keyword parameter", tmp->operand_range)); //error - unknown name of keyword parameter

				//MACROCASE TODO
				auto name = tmp->chain[0]->access_str()->value;

				tmp->chain.erase(tmp->chain.begin());
				tmp->chain.erase(tmp->chain.begin());

				args.symbolic_params.push_back({ std::make_unique<context::macro_param_data_single>(name + "=" + semantics::concatenation_point::to_string(tmp->chain)),nullptr });
			}
			else
			{
				if (std::find(keyword_params.begin(), keyword_params.end(), id) != keyword_params.end())
				{
					add_diagnostic(diagnostic_op::error_E011("Keyword", tmp->operand_range)); // error - keyword already defined
				}
				else
				{
					keyword_params.push_back(id);
				}

				tmp->chain.erase(tmp->chain.begin());
				tmp->chain.erase(tmp->chain.begin());

				if (tmp->chain.size() == 1 && tmp->chain.front()->type == semantics::concat_type::VAR)
					args.symbolic_params.push_back({
						string_to_macrodata(mngr.convert_to<context::C_t>(
								mngr.get_var_sym_value(*tmp->chain.front()->access_var(),eval_ctx),
								tmp->chain.front()->access_var()->symbol_range)),
						id });
				else
					args.symbolic_params.push_back({ mngr.create_macro_data(std::move(tmp->chain),eval_ctx),id });
			}
		}
		else if (tmp->chain.size() == 1 && tmp->chain.front()->type == semantics::concat_type::VAR)
			args.symbolic_params.push_back({
				string_to_macrodata(mngr.convert_to<context::C_t>(
						mngr.get_var_sym_value(*tmp->chain.front()->access_var(),eval_ctx),
						tmp->chain.front()->access_var()->symbol_range)),
				nullptr });
		else
			args.symbolic_params.push_back({ mngr.create_macro_data(std::move(tmp->chain),eval_ctx) ,nullptr });
	}

	return args;
}

