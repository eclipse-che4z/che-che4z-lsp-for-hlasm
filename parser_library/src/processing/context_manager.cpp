#include "context_manager.h"
#include "../expressions/expression_visitor.h"

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::processing;

context_manager::context_manager(context::hlasm_context& hlasm_ctx)
	: diagnosable_ctx(hlasm_ctx), hlasm_ctx(hlasm_ctx) {}


context::id_index context_manager::concatenate(const semantics::concat_chain& chain) const
{
	return hlasm_ctx.ids().add(
		concatenate_str(chain)
	);
}

std::string context_manager::concatenate_str(const semantics::concat_chain& chain) const
{
	std::string result;
	bool last_was_var = false;

	for (auto& point : chain)
	{
		if (!point) continue;
		switch (point->type)
		{
		case semantics::concat_type::STR:
			last_was_var = false;
			result.append(concat(point->access_str()));
			break;
		case semantics::concat_type::DOT:
			if (last_was_var) continue;
			last_was_var = false;
			result.append(concat(point->access_dot()));
			break;
		case semantics::concat_type::EQU:
			last_was_var = false;
			result.append(concat(point->access_equ()));
			break;
		case semantics::concat_type::VAR:
			last_was_var = true;
			result.append(concat(point->access_var()));
			break;
		case semantics::concat_type::SUB:
			last_was_var = false;
			result.append(concat(point->access_sub()));
			break;
		default:
			break;
		}
	}
	return result;
}

context::SET_t context_manager::get_var_sym_value(const semantics::var_sym& symbol) const
{
	auto id = symbol.created ?
		concatenate(symbol.access_created()->created_name)
		: symbol.access_basic()->name;

	std::vector<expressions::expr_ptr> subscript;						//TODO check expr errors
	for (auto tree : symbol.subscript)
		subscript.push_back(evaluate_expression_tree(tree));

	auto var = hlasm_ctx.get_var_sym(id);

	bool ok = test_var_sym(var, subscript, symbol.symbol_range);

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
		std::vector<int> tmp;
		for (auto& e : subscript)
		{
			tmp.push_back(e->get_numeric_value() - 1);
		}
		return mac_par->get_value(tmp);
	}
	return context::SET_t();
}

context::id_index context_manager::get_symbol_name(const semantics::var_sym* symbol)
{
	return symbol->created ?
		concatenate(symbol->access_created()->created_name)
		: symbol->access_basic()->name;
	//TODO check
}

context::id_index context_manager::get_symbol_name(const std::string& symbol)
{
	return hlasm_ctx.ids().add(symbol);
	//TODO check
}

expressions::expr_ptr context_manager::evaluate_expression_tree(antlr4::ParserRuleContext* expr_context) const
{
	expressions::expression_visitor expression_evaluator(hlasm_ctx);

	auto e = expression_evaluator.visit(expr_context).as<expressions::expr_ptr>();

	if (e->diag)
		add_diagnostic(
			diagnostic_op{ 
				e->diag->severity, std::move(e->diag->code), 
				std::move(e->diag->message), semantics::range_provider(range()).get_range(expr_context)});

	return e;
}


std::string context_manager::concat(semantics::char_str* str) const
{
	return str->value;
}

std::string context_manager::concat(semantics::var_sym* vs) const
{
	return get_var_sym_value(*vs).to<context::C_t>();
}

std::string context_manager::concat(semantics::dot*) const
{
	return ".";
}

std::string context_manager::concat(semantics::equals*) const
{
	return "=";
}

std::string context_manager::concat(semantics::sublist* sublist) const
{
	std::string ret("(");
	for (size_t i = 0; i < sublist->list.size(); ++i)
	{
		for (auto& point : sublist->list[i])
		{
			if (!point) continue;

			switch (point->type)
			{
			case semantics::concat_type::STR:
				ret.append(concat(point->access_str()));
				break;
			case semantics::concat_type::DOT:
				ret.append(concat(point->access_dot()));
				break;
			case semantics::concat_type::VAR:
				ret.append(concat(point->access_var()));
				break;
			case semantics::concat_type::SUB:
				ret.append(concat(point->access_sub()));
				break;
			}
		}
		if (i != sublist->list.size() - 1) ret.append(",");
	}
	ret.append(")");

	return ret;
}

bool context_manager::test_var_sym(context::var_sym_ptr var,const expressions::expr_list& subscript, const range& symbol_range) const
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
		std::vector<size_t> tmp;
		for (auto& e : subscript)
		{
			if (!e || e->get_numeric_value() < 1)
			{
				add_diagnostic(diagnostic_op::error_E012("subscript value has to be 1 or more", symbol_range)); //error - subscript is less than 1
				return false;
			}
		}
	}

	return true;
}

void context_manager::collect_diags() const {}
