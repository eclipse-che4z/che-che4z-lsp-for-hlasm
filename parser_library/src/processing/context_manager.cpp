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

#include "context_manager.h"
#include "lexing/lexer.h"
#include "expressions/visitors/expression_evaluator.h"
#include "context/variables/system_variable.h"

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::processing;

context_manager::context_manager(context::hlasm_context& hlasm_ctx)
	: diagnosable_ctx(hlasm_ctx), hlasm_ctx(hlasm_ctx) {}

context::SET_t context_manager::evaluate_expression(antlr4::ParserRuleContext* expr_context, expressions::evaluation_context eval_ctx) const
{
	expressions::expression_evaluator evaluator(eval_ctx);

	auto result = evaluator.evaluate_expression(expr_context);

	collect_diags_from_child(evaluator);

	return result->get_set_value();
}

context::SET_t context_manager::convert(context::SET_t source, context::SET_t_enum target_type, range value_range) const
{
	expressions::expr_ptr tmp_e;
	using namespace context;
	switch (target_type)
	{
	case SET_t_enum::A_TYPE:
		switch (source.type)
		{
		case SET_t_enum::A_TYPE:
			return source.access_a();
		case SET_t_enum::B_TYPE:
			return (int)source.access_b();
		case SET_t_enum::C_TYPE:
			tmp_e = expressions::arithmetic_expression::from_string(source.access_c(), false);
			if (!tmp_e->diag)
				return tmp_e->get_numeric_value();
			break;
		default:
			break;
		}
		break;
	case SET_t_enum::B_TYPE:
		switch (source.type)
		{
		case SET_t_enum::A_TYPE:
			return (bool)source.access_a();
		case SET_t_enum::B_TYPE:
			return source.access_b();
		case SET_t_enum::C_TYPE:
			tmp_e = expressions::arithmetic_expression::from_string(source.access_c(), false);
			if (!tmp_e->diag)
				return tmp_e->get_numeric_value();
			break;
		default:
			break;
		}
		break;
	case context::SET_t_enum::C_TYPE:
		switch (source.type)
		{
		case SET_t_enum::A_TYPE:
			return std::to_string(std::abs(source.access_a()));
		case SET_t_enum::B_TYPE:
			return source.access_b() ? std::string("1") : std::string("0");
		case SET_t_enum::C_TYPE:
			return std::move(source.access_c());
		default:
			break;
		}
		break;
	default:
		break;
	}

	auto diag = error_messages::e001();
	diag->diag_range = value_range;
	add_diagnostic(std::move(*diag));
	return SET_t();
}

context::id_index context_manager::concatenate(const semantics::concat_chain& chain, expressions::evaluation_context eval_ctx) const
{
	return hlasm_ctx.ids().add(
		concatenate_str(chain,eval_ctx)
	);
}

std::string context_manager::concatenate_str(const semantics::concat_chain& chain, expressions::evaluation_context eval_ctx) const
{
	expressions::expression_evaluator evaluator(eval_ctx);

	auto result = evaluator.concatenate_chain(chain);

	collect_diags_from_child(evaluator);

	return result;
}

context::macro_data_ptr context_manager::create_macro_data(const semantics::concat_chain& chain) const
{
	auto tmp = semantics::concatenation_point::contains_var_sym(chain);
	if (tmp)
	{
		add_diagnostic(diagnostic_op::error_E064(tmp->symbol_range));
		return std::make_unique<context::macro_param_data_dummy>();
	}

	return create_macro_data(chain, semantics::concatenation_point::to_string);
}

context::macro_data_ptr context_manager::create_macro_data(const semantics::concat_chain& chain, expressions::evaluation_context eval_ctx) const
{
	expressions::expression_evaluator evaluator(eval_ctx);

	auto data = create_macro_data(chain, [&](const auto& chain) {return evaluator.concatenate_chain(chain); });

	collect_diags_from_child(evaluator);

	return data;
}

context::SET_t context_manager::get_var_sym_value(const semantics::var_sym& symbol, expressions::evaluation_context eval_ctx) const
{
	auto id = symbol.created ?
		concatenate(symbol.access_created()->created_name,eval_ctx)
		: symbol.access_basic()->name;

	expressions::expression_evaluator evaluator(eval_ctx);

	auto subscript = evaluator.evaluate_expressions(symbol.subscript);

	collect_diags_from_child(evaluator);

	return get_var_sym_value(id, subscript, symbol.symbol_range);
}

context::SET_t context_manager::get_var_sym_value(context::id_index name, const expressions::expr_list& subscript, const range& symbol_range) const
{
	auto var = hlasm_ctx.get_var_sym(name);

	bool ok = test_symbol_for_read(var, subscript, symbol_range);

	if (!ok)
		return context::SET_t();

	if (auto set_sym = var->access_set_symbol_base())
	{
		size_t idx = 0;

		if (subscript.empty())
		{
			switch (set_sym->type)
			{
			case context::SET_t_enum::A_TYPE:
				return set_sym->access_set_symbol<context::A_t>()->get_value();
				break;
			case context::SET_t_enum::B_TYPE:
				return set_sym->access_set_symbol<context::B_t>()->get_value();
				break;
			case context::SET_t_enum::C_TYPE:
				return set_sym->access_set_symbol<context::C_t>()->get_value();
				break;
			default:
				return context::SET_t();
				break;
			}
		}
		else
		{
			idx = (size_t)(subscript[0] ? subscript[0]->get_numeric_value() - 1 : 0);

			switch (set_sym->type)
			{
			case context::SET_t_enum::A_TYPE:
				return set_sym->access_set_symbol<context::A_t>()->get_value(idx);
				break;
			case context::SET_t_enum::B_TYPE:
				return set_sym->access_set_symbol<context::B_t>()->get_value(idx);
				break;
			case context::SET_t_enum::C_TYPE:
				return set_sym->access_set_symbol<context::C_t>()->get_value(idx);
				break;
			default:
				return context::SET_t();
				break;
			}
		}
	}
	else if (auto mac_par = var->access_macro_param_base())
	{
		std::vector<size_t> tmp;
		for (auto& e : subscript)
		{
			tmp.push_back((size_t)e->get_numeric_value());
		}
		return mac_par->get_value(tmp);
	}
	return context::SET_t();
}

context_manager::name_result context_manager::try_get_symbol_name(const semantics::var_sym* symbol, expressions::evaluation_context eval_ctx) const
{
	if (!symbol->created)
		return make_pair(true, symbol->access_basic()->name);
	else
		return try_get_symbol_name(
			concatenate_str(symbol->access_created()->created_name, eval_ctx),
			symbol->symbol_range
		);
}

context_manager::name_result context_manager::try_get_symbol_name(const std::string& symbol, range symbol_range) const
{
	size_t i;
	for (i = 0; i < symbol.size(); ++i)
		if (!lexer::ord_char(symbol[i]) || !(i != 0 || !isdigit(symbol[i])))
			break;

	if (i==0 || i > 63)
	{
		add_diagnostic(diagnostic_op::error_E065(symbol_range));
		return std::make_pair(false, context::id_storage::empty_id);
	}

	return std::make_pair(true, hlasm_ctx.ids().add(symbol.substr(0, i)));
}

bool context_manager::test_symbol_for_read(context::var_sym_ptr var,const expressions::expr_list& subscript, const range& symbol_range) const
{
	if (!var)
	{
		add_diagnostic(diagnostic_op::error_E010("variable", symbol_range)); //error - unknown name of variable
		return false;
	}

	if (auto set_sym = var->access_set_symbol_base())
	{
		if (subscript.size() > 1)
		{
			add_diagnostic(diagnostic_op::error_E020("variable symbol subscript", symbol_range)); //error - too many operands
			return false;
		}

		if ((set_sym->is_scalar && subscript.size() == 1) || (!set_sym->is_scalar && subscript.size() == 0))
		{
			add_diagnostic(diagnostic_op::error_E013("subscript error", symbol_range)); //error - inconsistent format of subcript
			return false;
		}

		if (!set_sym->is_scalar && (!subscript[0] || subscript[0]->get_numeric_value() < 1))
		{
			add_diagnostic(diagnostic_op::error_E012("subscript value has to be 1 or more", symbol_range)); //error - subscript is less than 1
			return false;
		}
	}
	else if (auto mac_par = var->access_macro_param_base())
	{
		for (size_t i = 0; i < subscript.size(); ++i)
		{
			auto& e = subscript[i];
			if (!e || e->get_numeric_value() < 1)
			{
				if (i == 0 && e && e->get_numeric_value() == 0 && dynamic_cast<context::system_variable*>(mac_par)) // if syslist and subscript = 0, ok
					continue;

				add_diagnostic(diagnostic_op::error_E012("subscript value has to be 1 or more", symbol_range)); //error - subscript is less than 1
				return false;
			}
		}
	}

	return true;
}

void context_manager::collect_diags() const {}

context::macro_data_ptr context_manager::create_macro_data(const semantics::concat_chain& chain,
	const std::function<std::string(const semantics::concat_chain& chain)>& to_string) const
{
	context_manager mngr(hlasm_ctx);

	if (chain.size() == 0)
		return std::make_unique<context::macro_param_data_dummy>();
	else if (chain.size() > 1 || (chain.size() == 1 && chain[0]->type != semantics::concat_type::SUB))
		return std::make_unique<context::macro_param_data_single>(to_string(chain));

	const auto& inner_chains = chain[0]->access_sub()->list;

	std::vector<context::macro_data_ptr> sublist;

	for (auto& inner_chain : inner_chains)
	{
		sublist.push_back(create_macro_data(inner_chain, to_string));
	}
	return std::make_unique<context::macro_param_data_composite>(std::move(sublist));
}
