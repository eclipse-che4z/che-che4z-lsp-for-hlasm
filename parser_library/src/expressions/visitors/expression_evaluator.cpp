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

#include "expression_evaluator.h"
#include "expression_analyzer.h"
#include "../../processing/context_manager.h"
#include "../../ebcdic_encoding.h"

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::expressions;

expression_evaluator::expression_evaluator(evaluation_context eval_ctx)
	: diagnosable_ctx(eval_ctx.hlasm_ctx), eval_ctx_(eval_ctx), resolved_refs_(nullptr) {}

expr_ptr expression_evaluator::evaluate_expression(antlr4::ParserRuleContext* expr_context)
{
	expression_analyzer analyzer(eval_ctx_);

	auto refs = analyzer.get_undefined_symbol_references(expr_context);

	resolved_refs_ = &eval_ctx_.attr_provider.lookup_forward_attribute_references(std::move(refs));

	auto result =  visit(expr_context).as<expr_ptr>();

	if (result->diag)
	{
		result->diag->diag_range = semantics::range_provider().get_range(expr_context);
		add_diagnostic(*result->diag);
	}

	return result;
}

std::vector<expr_ptr> expression_evaluator::evaluate_expressions(std::vector<antlr4::ParserRuleContext*> exprs_context)
{
	expression_analyzer analyzer(eval_ctx_);

	processing::attribute_provider::forward_reference_storage collected_refs;

	for (auto& expr : exprs_context)
	{
		auto refs = analyzer.get_undefined_symbol_references(expr);

		for (auto& ref : refs)
			collected_refs.insert(ref);
	}

	resolved_refs_ = &eval_ctx_.attr_provider.lookup_forward_attribute_references(std::move(collected_refs));

	std::vector<expr_ptr> sublist;

	for (auto expr_ctx : exprs_context)
	{
		auto e = visit(expr_ctx).as<expr_ptr>();
		if (e->diag)
		{
			e->diag->diag_range = semantics::range_provider().get_range(expr_ctx);
			add_diagnostic(*e->diag);
		}
		sublist.push_back(std::move(e));
	}

	return sublist;
}

std::string expression_evaluator::concatenate_chain(const semantics::concat_chain& chain)
{
	expression_analyzer analyzer(eval_ctx_);

	auto tmp = analyzer.get_undefined_symbol_references(chain);

	resolved_refs_ = &eval_ctx_.attr_provider.lookup_forward_attribute_references(std::move(tmp));

	return concatenate(chain);
}

void expression_evaluator::collect_diags() const
{
}

antlrcpp::Any expression_evaluator::visitExpr(generated::hlasmparser::ExprContext * ctx)
{
	return antlrcpp::Any(expression::evaluate(visit(ctx->expr_p_space_c())));
}

antlrcpp::Any expression_evaluator::visitExpr_test(generated::hlasmparser::Expr_testContext * ctx)
{
	return visit(ctx->expr_statement());
}

antlrcpp::Any expression_evaluator::visitExpr_statement(generated::hlasmparser::Expr_statementContext * ctx)
{
	std::string res;
	
	if (ctx->tmp != nullptr)
	{
		res = visit(ctx->tmp).as<std::string>();
		res.push_back('\n');
	}
	res.append(visit(ctx->expr_p()).as<expr_ptr>()->get_str_val());
	return res;
}

antlrcpp::Any expression_evaluator::visitExpr_p(generated::hlasmparser::Expr_pContext * ctx)
{
	if (ctx->expr_sContext != nullptr)
		return visit(ctx->expr_sContext);
	else if (ctx->children.at(0)->getText() == "+")
		return visit(ctx->expr_p());
	else
		return (-*visitE(ctx->expr_p()));
}

antlrcpp::Any expression_evaluator::visitExpr_s(generated::hlasmparser::Expr_sContext * ctx)
{
	if (ctx->t != nullptr)
		return visit(ctx->t);

	auto a = visitE(ctx->tmp);
	auto b = visitE(ctx->term_c());

	if (ctx->children.at(1)->getText() == "+")
		return (*a + *b);
	else
		return (*a - *b);
}

antlrcpp::Any expression_evaluator::visitTerm_c(generated::hlasmparser::Term_cContext * ctx)
{
	if (ctx->t != nullptr)
		return visit(ctx->t);

	auto a = visitE(ctx->tmp);
	auto b = visitE(ctx->term());

	if (ctx->children.at(1)->getText() == "*")
		return (*a * *b);
	else
		return (*a / *b);
}

antlrcpp::Any expression_evaluator::visitTerm(generated::hlasmparser::TermContext * ctx)
{

	if (ctx->var_symbolContext != nullptr)
	{
		auto value = get_var_sym_value(ctx->var_symbolContext->vs.get());
		switch (value.type)
		{
		case context::SET_t_enum::A_TYPE:
			return (expr_ptr)make_arith(value.access_a());
		case context::SET_t_enum::B_TYPE:
			return (expr_ptr)make_logic(value.access_b());
		case context::SET_t_enum::C_TYPE:
			return (expr_ptr)arithmetic_expression::from_string(value.access_c(), false);
		default:
			return (expr_ptr)make_arith(0);
		}
	}

	if (ctx->expr() != nullptr)
		return visit(ctx->expr());

	if (ctx->ca_string() != nullptr)
		return (expr_ptr)visit(ctx->ca_string()).as<char_ptr>();

	if (ctx->data_attribute() != nullptr)
		return visit(ctx->data_attribute());

	if (ctx->string() != nullptr)
		return expression::self_defining_term(
			ctx->children.at(0)->getText(),
			visit(ctx->string()).as<std::string>(),
			false); /*TODO: dbcs*/

	if (ctx->id_sub() != nullptr)
	{
		return visit(ctx->id_sub());
	}

	if (ctx->children.size() == 1)
	{
		return visit(ctx->children.at(0));
	}

	assert(ctx->exception);
	return (expr_ptr)make_arith(0);
}

antlrcpp::Any expression_evaluator::visitId_sub(generated::hlasmparser::Id_subContext* ctx)
{
	auto subscript = visit(ctx->subscriptContext).as <std::vector<expr_ptr>>();
	if (subscript.empty())
	{
		auto symbol_name = ctx->id_no_dotContext->name;
		if (keyword_expression::is_keyword(*symbol_name))
			return static_cast<expr_ptr>(std::make_unique<keyword_expression>(*symbol_name));

		auto tmp_symbol = eval_ctx_.hlasm_ctx.ord_ctx.get_symbol(symbol_name);

		if (tmp_symbol && tmp_symbol->kind() == context::symbol_value_kind::ABS)
			return static_cast<expr_ptr>(make_arith(tmp_symbol->value().get_abs()));
		else
		{
			auto err_ex = make_arith(0);
			err_ex->diag = error_messages::e005();
			return static_cast<expr_ptr>(err_ex);
		}
	}
	else
	{
		assert(subscript.size() <= 2 && subscript.size() > 0);
		if (subscript.size() == 1)
			return subscript[0]->unary_operation(*ctx->id_no_dotContext->name);
		else
			return subscript[0]->binary_operation(*ctx->id_no_dotContext->name, subscript[1]);
	}
}

antlrcpp::Any expression_evaluator::visitExpr_p_comma_c(generated::hlasmparser::Expr_p_comma_cContext * ctx)
{
	std::vector<expr_ptr> exs;
	if (ctx->exs != nullptr)
		exs = visit(ctx->exs).as< std::vector<expr_ptr>>();
	auto expr = visit(ctx->expr_pContext);
	exs.push_back(expr.as<expr_ptr>());
	return exs;
}

antlrcpp::Any expression_evaluator::visitSubscript(generated::hlasmparser::SubscriptContext * ctx)
{
	if (ctx->children.size() >= 3)
	{
		return visit(ctx->expr_p_comma_cContext).as<std::vector<expr_ptr>>();
	}

	return std::vector<expr_ptr>();
}

antlrcpp::Any expression_evaluator::visitString(generated::hlasmparser::StringContext * ctx)
{
	return ctx->string_ch_cContext->value;
}

antlrcpp::Any expression_evaluator::visitCa_string(generated::hlasmparser::Ca_stringContext * ctx)
{
	if (ctx->tmp != nullptr)
		return visit(ctx->tmp).as<char_ptr>()
		->append(visit(ctx->ca_string_b()).as<char_ptr>());
	else
		return visit(ctx->ca_string_b());
}

antlrcpp::Any expression_evaluator::visitCa_string_b(generated::hlasmparser::Ca_string_bContext * ctx)
{
	auto tmp = concatenate(ctx->string_ch_v_c()->chain);
	auto ex = make_char(tmp);
	expr_ptr s, e;
	if(ctx->substring()->e1 != nullptr)
		s = visit(ctx->substring()->e1);
	if (ctx->substring()->e2 != nullptr)
		e = visit(ctx->substring()->e2);
	return ex->substring(visit(ctx->ca_dupl_factor()).as<int>(), s, e);
}

antlrcpp::Any expression_evaluator::visitCa_dupl_factor(generated::hlasmparser::Ca_dupl_factorContext * ctx)
{
	if (ctx->expr_p() != nullptr)
		return visit(ctx->expr_p()).as<expr_ptr>()->get_numeric_value();
	else
		return 1;
}

antlrcpp::Any expression_evaluator::visitExpr_p_space_c(generated::hlasmparser::Expr_p_space_cContext * ctx)
{
	std::deque<expr_ptr> exs;
	if (ctx->exs != nullptr)
		exs = visit(ctx->exs).as< std::deque<expr_ptr>>();
	exs.push_back(visitE(ctx->expr_p()));
	return exs;
}

antlrcpp::Any expression_evaluator::visitData_attribute(generated::hlasmparser::Data_attributeContext * ctx)
{
	processing::context_manager mngr(eval_ctx_.hlasm_ctx);
	std::optional<context::SET_t> SET_val;
	context::id_index symbol_name = nullptr;

	auto attr = ctx->attribute;

	if (ctx->var_symbol())
	{
		auto [name, expr_subscript] = evaluate_var_sym(ctx->var_symbol()->vs.get());

		//get evaluated_subscript
		std::vector<size_t> subscript;
		for (auto tree : expr_subscript)
		{
			subscript.push_back(
				(size_t)mngr.convert_to<context::A_t>(
					tree->get_set_value(),
					semantics::range_provider().get_range(ctx->var_symbol()->vs->subscript[subscript.size()])
				)
			);
		}

		//get symbol
		auto symbol = eval_ctx_.hlasm_ctx.get_var_sym(name);

		//get value
		if (context::symbol_attributes::needs_ordinary(attr))
		{
			//get substituted name
			auto tmp_val = mngr.get_var_sym_value(name, expr_subscript, ctx->var_symbol()->vs->symbol_range);
			auto val = mngr.convert_to<context::C_t>(tmp_val, ctx->var_symbol()->vs->symbol_range);
			auto [valid,new_name] = mngr.try_get_symbol_name(val, ctx->var_symbol()->vs->symbol_range);

			if (!valid)
			{
				SET_val.emplace((attr == context::data_attr_kind::O) ? "U" : 0);
				symbol_name = new_name;
			}
			else
			{
				if (!eval_ctx_.hlasm_ctx.ord_ctx.get_symbol(new_name) && context::symbol_attributes::ordinary_allowed(attr)) // requires attribute lookahead
					SET_val.emplace(lookup_variable_symbol_attribute(attr, new_name, ctx->var_symbol()->vs->symbol_range));
				else
					SET_val.emplace(eval_ctx_.hlasm_ctx.get_data_attribute(attr, new_name));
			}
		}
		else if (attr == context::data_attr_kind::T)
		{
			if (!mngr.test_symbol_for_read(symbol, expr_subscript, ctx->var_symbol()->vs->symbol_range))
				SET_val.emplace(std::string("U"));

			auto t_attr_value = eval_ctx_.hlasm_ctx.get_data_attribute(attr, symbol, subscript).access_c();
			if (t_attr_value == "U" && symbol)
			{
				auto tmp_val = mngr.get_var_sym_value(name, expr_subscript, ctx->var_symbol()->vs->symbol_range);
				auto val = mngr.convert_to<context::C_t>(tmp_val, ctx->var_symbol()->vs->symbol_range);
				auto [valid, new_name] = processing::context_manager(eval_ctx_.hlasm_ctx).try_get_symbol_name(val, ctx->var_symbol()->vs->symbol_range);

				if (valid && !eval_ctx_.hlasm_ctx.ord_ctx.get_symbol(new_name)) // requires attribute lookahead
					SET_val.emplace(lookup_variable_symbol_attribute(attr, new_name, ctx->var_symbol()->vs->symbol_range));
				else
					SET_val.emplace(std::string("U"));
			}
			else
				SET_val.emplace(t_attr_value);
		}
		else
		{
			if (attr == context::data_attr_kind::K && !mngr.test_symbol_for_read(symbol, expr_subscript, ctx->var_symbol()->vs->symbol_range))
				SET_val.emplace(1);
			else
				SET_val.emplace(eval_ctx_.hlasm_ctx.get_data_attribute(attr, symbol, subscript));
		}

	}
	else if (ctx->id())
	{
		symbol_name = ctx->id()->name;
		if (context::symbol_attributes::ordinary_allowed(attr))
		{
			auto symbol = eval_ctx_.hlasm_ctx.ord_ctx.get_symbol(symbol_name);

			SET_val.emplace(get_ord_attr_value(attr, symbol, symbol_name, semantics::range_provider().get_range(ctx)));
		}
		else if (attr == context::data_attr_kind::D || attr == context::data_attr_kind::O)
		{
			SET_val.emplace(eval_ctx_.hlasm_ctx.get_data_attribute(attr, ctx->id()->name));
		}
		else
		{
			SET_val.emplace(context::symbol_attributes::default_value(attr));
			add_diagnostic(diagnostic_op::error_E066(semantics::range_provider().get_range(ctx)));
		}
	}

	if (attr == context::data_attr_kind::O && SET_val->type == context::SET_t_enum::C_TYPE && SET_val->access_c() == "U" && symbol_name) //check for external libraries
	{
		std::string right_value = eval_ctx_.lib_provider.has_library(*symbol_name, eval_ctx_.hlasm_ctx) ? "S" : "U";
		SET_val.emplace(right_value);
	}

	collect_diags_from_child(mngr);

	if(SET_val)
		switch (SET_val->type)
		{
		case context::SET_t_enum::A_TYPE:
			return static_cast<expr_ptr>(make_arith(SET_val->access_a()));
		case context::SET_t_enum::B_TYPE:
			return static_cast<expr_ptr>(make_logic(SET_val->access_b()));
		case context::SET_t_enum::C_TYPE:
			return static_cast<expr_ptr>(make_char(SET_val->access_c()));
		default:
			break;
		}

	return static_cast<expr_ptr>(make_arith(0));
}

antlrcpp::Any expression_evaluator::visitNum(generated::hlasmparser::NumContext* ctx)
{
	return (expr_ptr)make_arith(ctx->value);
}

context::SET_t expression_evaluator::get_ord_attr_value(context::data_attr_kind attr, const context::symbol* symbol, context::id_index symbol_name, range symbol_range)
{
	if (!symbol)
	{
		auto it = resolved_refs_->find(symbol_name);
		if (it != resolved_refs_->end())
			symbol = &it->second;
	}

	if (!symbol)
	{
		if (attr == context::data_attr_kind::T)
			return std::string("U");
		else
		{
			add_diagnostic(diagnostic_op::warning_W013(symbol_range));
			return context::symbol_attributes::default_value(attr);
		}
	}
	else
	{
		if (attr == context::data_attr_kind::T)
			return std::string({ (char)ebcdic_encoding::e2a[symbol->attributes().type()] });
		else if (!symbol->attributes().can_have_SI_attr() && (attr == context::data_attr_kind::S || attr == context::data_attr_kind::I))
		{
			add_diagnostic(diagnostic_op::error_E066(symbol_range));
			return context::symbol_attributes::default_value(attr);
		}
		else if (!symbol->attributes().is_defined(attr))
		{
			add_diagnostic(diagnostic_op::warning_W013(symbol_range));
			return context::symbol_attributes::default_value(attr);
		}
		else
			return symbol->attributes().get_attribute_value(attr);
	}
}

std::string expression_evaluator::concatenate(const semantics::concat_chain& chain)
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
			if (last_was_var)
			{
				last_was_var = false;
				continue;
			}
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

expr_ptr expression_evaluator::visitE(antlr4::ParserRuleContext * ctx)
{
	return visit(ctx).as<expr_ptr>();
}

std::string expression_evaluator::concat(semantics::char_str* str)
{
	return str->value;
}

std::string expression_evaluator::concat(semantics::var_sym* vs)
{
	return processing::context_manager(eval_ctx_.hlasm_ctx).
		convert_to<context::C_t>(get_var_sym_value(vs),vs->symbol_range);
}

std::string expression_evaluator::concat(semantics::dot*)
{
	return ".";
}

std::string expression_evaluator::concat(semantics::equals*)
{
	return "=";
}

std::string expression_evaluator::concat(semantics::sublist* sublist)
{
	std::string ret("(");
	for (size_t i = 0; i < sublist->list.size(); ++i)
	{
		ret.append(concatenate(sublist->list[i]));
		if (i != sublist->list.size() - 1) ret.append(",");
	}
	ret.append(")");

	return ret;
}

context::SET_t expression_evaluator::get_var_sym_value(semantics::var_sym* vs)
{
	processing::context_manager mngr(eval_ctx_.hlasm_ctx);

	context::id_index id;
	if(!vs->created)
		id = vs->access_basic()->name;
	else
	{
		auto [valid, name] = mngr.try_get_symbol_name(concatenate(vs->access_created()->created_name), vs->symbol_range);
		if (!valid)
			return context::SET_t();
		id = name;
	}

	std::vector<expr_ptr> subscript;

	for (const auto& tree : vs->subscript)
		subscript.push_back(visit(tree).as<expr_ptr>());

	auto value = mngr.get_var_sym_value(id, subscript, vs->symbol_range);

	collect_diags_from_child(mngr);

	return value;
}

std::pair<context::id_index, std::vector<expr_ptr>> expression_evaluator::evaluate_var_sym(semantics::var_sym* vs)
{
	expression_evaluator evaluator(eval_ctx_);

	context::id_index name;
	if (vs->created)
		name = eval_ctx_.hlasm_ctx.ids().add(concatenate_chain(vs->access_created()->created_name));
	else
		name = vs->access_basic()->name;
	
	std::vector<expr_ptr> subscript;

	for (auto& tree : vs->subscript)
		subscript.push_back(evaluator.evaluate_expression(tree));

	collect_diags_from_child(evaluator);

	return std::make_pair(name, std::move(subscript));
}

context::SET_t expression_evaluator::lookup_variable_symbol_attribute(context::data_attr_kind attr, context::id_index symbol_name, range symbol_range)
{
	auto res = eval_ctx_.attr_provider.lookup_forward_attribute_references({ symbol_name });

	context::symbol* symbol = nullptr;

	auto it = res.find(symbol_name);
	if (it != res.end())
		symbol = &it->second;

	return get_ord_attr_value(attr, symbol, symbol_name, symbol_range);
}
