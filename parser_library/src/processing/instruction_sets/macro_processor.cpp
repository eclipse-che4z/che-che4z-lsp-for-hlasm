#include "macro_processor.h"
#include "../context_manager.h"
#include "../statement_processors/macrodef_processor.h"
#include <memory>

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::processing;

macro_processor::macro_processor(context::hlasm_context& hlasm_ctx)
	:instruction_processor(hlasm_ctx) {}

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
			mngr.concatenate_str(std::get<semantics::concat_chain>(statement.label_ref().value))
			);
		break;
	case semantics::label_si_type::ORD:
	case semantics::label_si_type::MAC:
		label_value = std::make_unique<context::macro_param_data_single>(std::get<std::string>(statement.label_ref().value));
		break;
	case semantics::label_si_type::VAR:
		label_value = std::make_unique<context::macro_param_data_single>(
			mngr.get_var_sym_value(*std::get<semantics::vs_ptr>(statement.label_ref().value)).to<context::C_t>()
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
		if (op->type == semantics::operand_type::EMPTY || op->type == semantics::operand_type::UNDEF)
		{
			args.symbolic_params.push_back({ nullptr,nullptr });
			continue;
		}

		auto tmp = op->access_mac();
		assert(tmp);

		semantics::concatenation_point::clear_concat_chain(tmp->chain);

		if (tmp->chain.size() >= 2 &&
			tmp->chain[0]->type == semantics::concat_type::STR && tmp->chain[1]->type == semantics::concat_type::EQU)
		{
			auto id = mngr.get_symbol_name(tmp->chain[0]->access_str()->value);
			auto named = hlasm_ctx.macros().find(statement.opcode_ref().value)->second->named_params().find(id);
			if (named == hlasm_ctx.macros().find(statement.opcode_ref().value)->second->named_params().end() || named->second->param_type() == context::macro_param_type::POS_PAR_TYPE)
			{
				add_diagnostic(diagnostic_s::error_E010("", "keyword parameter", tmp->operand_range)); //error - unknown name of keyword parameter

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
					add_diagnostic(diagnostic_s::error_E011("", "Keyword", tmp->operand_range)); // error - keyword already defined
				}
				else
				{
					keyword_params.push_back(id);
				}

				tmp->chain.erase(tmp->chain.begin());
				tmp->chain.erase(tmp->chain.begin());

				args.symbolic_params.push_back({ macrodef_processor::create_macro_data(std::move(tmp->chain),hlasm_ctx),id });
			}
		}
		else
			args.symbolic_params.push_back({ macrodef_processor::create_macro_data(std::move(tmp->chain),hlasm_ctx) ,nullptr });
	}

	return args;
}

