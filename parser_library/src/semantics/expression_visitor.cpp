#include "expression_visitor.h"

using namespace hlasm_plugin::parser_library::generated;
using namespace hlasm_plugin::parser_library::context;

inline antlrcpp::Any::Base::~Base(void) {}

antlrcpp::Any hlasm_plugin::parser_library::context::expression_visitor::visitExpr(hlasmparser::ExprContext * ctx)
{
	return antlrcpp::Any(expression::evaluate(visit(ctx->expr_p_space_c())));
}

antlrcpp::Any hlasm_plugin::parser_library::context::expression_visitor::visitExpr_test(hlasm_plugin::parser_library::generated::hlasmparser::Expr_testContext * ctx)
{
	return visit(ctx->expr_statement());
}

int temporary_counter___ = 0;

antlrcpp::Any hlasm_plugin::parser_library::context::expression_visitor::visitExpr_statement(hlasm_plugin::parser_library::generated::hlasmparser::Expr_statementContext * ctx)
{
	std::string res;
	
	if (ctx->tmp != nullptr)
	{
		++temporary_counter___;
		res = visit(ctx->tmp).as<std::string>();
		res.push_back('\n');
		
	}
	res.append(visit(ctx->expr()).as<expr_ptr>()->get_str_val());
	return res;
}

antlrcpp::Any hlasm_plugin::parser_library::context::expression_visitor::visitExpr_p(hlasm_plugin::parser_library::generated::hlasmparser::Expr_pContext * ctx)
{
	if (ctx->expr_sContext != nullptr)
		return visit(ctx->expr_sContext);
	else if (ctx->children.at(0)->getText() == "+")
		return visit(ctx->expr_p());
	else
		return (-*visitE(ctx->expr_p()));
}

antlrcpp::Any hlasm_plugin::parser_library::context::expression_visitor::visitExpr_s(hlasm_plugin::parser_library::generated::hlasmparser::Expr_sContext * ctx)
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

antlrcpp::Any hlasm_plugin::parser_library::context::expression_visitor::visitTerm_c(hlasm_plugin::parser_library::generated::hlasmparser::Term_cContext * ctx)
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

antlrcpp::Any hlasm_plugin::parser_library::context::expression_visitor::visitTerm(hlasm_plugin::parser_library::generated::hlasmparser::TermContext * ctx)
{

	if (ctx->var_symbolContext != nullptr)
	{
		auto var = ctx_mngr_.get_var_sym_value(ctx->var_symbolContext->vs);
		switch (var.type)
		{
		case context::SET_t_enum::A_TYPE:
			return (expr_ptr)make_arith(var.access_a());
		case context::SET_t_enum::B_TYPE:
			return (expr_ptr)make_logic(var.access_b());
		case context::SET_t_enum::C_TYPE:
			return (expr_ptr)make_char(var.access_c());
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

	if (ctx->idContext != nullptr)
	{
		auto subscript = visit(ctx->subscriptContext).as <symbol_guard<std::vector<expr_ptr>>>();
		if (!subscript)
			return expression::resolve_ord_symbol(ctx->idContext->name); /*TODO:DBCS*/
		else
		{
			assert(subscript.value.size() <= 2 && subscript.value.size() > 0);
			if (subscript.value.size() == 1)
				return subscript.value[0]->unary_operation(ctx->idContext->name);
			else
				return subscript.value[0]->binary_operation(ctx->idContext->name, subscript.value[1]);
		}
	}

	assert(false);
	return make_arith(0);
}

antlrcpp::Any hlasm_plugin::parser_library::context::expression_visitor::visitExpr_p_comma_c(hlasm_plugin::parser_library::generated::hlasmparser::Expr_p_comma_cContext * ctx)
{
	std::vector<expr_ptr> exs;
	if (ctx->exs != nullptr)
		exs = visit(ctx->exs).as< std::vector<expr_ptr>>();
	auto expr = visit(ctx->expr_pContext);
	exs.push_back(expr.as<expr_ptr>());
	return exs;
}

antlrcpp::Any hlasm_plugin::parser_library::context::expression_visitor::visitSubscript(hlasm_plugin::parser_library::generated::hlasmparser::SubscriptContext * ctx)
{
	symbol_guard<std::vector<expr_ptr>> g;
	g.valid = false;
	if (ctx->children.size() >= 3)
	{
		g.valid = true;
		g.value = visit(ctx->expr_p_comma_cContext).as<std::vector<expr_ptr>>();
	}
	return g;
}

antlrcpp::Any hlasm_plugin::parser_library::context::expression_visitor::visitString(hlasm_plugin::parser_library::generated::hlasmparser::StringContext * ctx)
{
	return ctx->string_ch_cContext->value;
}

antlrcpp::Any hlasm_plugin::parser_library::context::expression_visitor::visitCa_string(hlasm_plugin::parser_library::generated::hlasmparser::Ca_stringContext * ctx)
{
	if (ctx->tmp != nullptr)
		return visit(ctx->tmp).as<char_ptr>()
		->append(visit(ctx->ca_string_b()).as<char_ptr>());
	else
		return visit(ctx->ca_string_b());
}

antlrcpp::Any hlasm_plugin::parser_library::context::expression_visitor::visitCa_string_b(hlasm_plugin::parser_library::generated::hlasmparser::Ca_string_bContext * ctx)
{
	concat_chain new_chain;

	new_chain.insert(new_chain.end(), make_clone_iterator(ctx->string_ch_v_c()->chain.begin()), make_clone_iterator(ctx->string_ch_v_c()->chain.end()));

	auto tmp = ctx_mngr_.concatenate(std::move(new_chain));
	auto ex = make_char(tmp);
	expr_ptr s, e;
	if(ctx->substring()->e1 != nullptr)
		s = visit(ctx->substring()->e1);
	if (ctx->substring()->e2 != nullptr)
		e = visit(ctx->substring()->e2);
	return ex->substring(visit(ctx->ca_dupl_factor()).as<int>(), s, e);
}

antlrcpp::Any hlasm_plugin::parser_library::context::expression_visitor::visitCa_dupl_factor(hlasm_plugin::parser_library::generated::hlasmparser::Ca_dupl_factorContext * ctx)
{
	if (ctx->expr_p() != nullptr)
		return visit(ctx->expr_p()).as<expr_ptr>()->get_numeric_value();
	else
		return 1;
}

hlasm_plugin::parser_library::context::expression_visitor::expression_visitor(const semantics::context_manager& ctx_mngr) : ctx_mngr_(ctx_mngr) {}

antlrcpp::Any hlasm_plugin::parser_library::context::expression_visitor::visitExpr_p_space_c(hlasm_plugin::parser_library::generated::hlasmparser::Expr_p_space_cContext * ctx)
{
	std::deque<expr_ptr> exs;
	if (ctx->exs != nullptr)
		exs = visit(ctx->exs).as< std::deque<expr_ptr>>();
	exs.push_back(visitE(ctx->expr_p()));
	return exs;
}

antlrcpp::Any expression_visitor::visitData_attribute(hlasm_plugin::parser_library::generated::hlasmparser::Data_attributeContext * ctx)
{
	return static_cast<expr_ptr>(make_arith(0));
}

expr_ptr hlasm_plugin::parser_library::context::expression_visitor::visitE(antlr4::ParserRuleContext * ctx)
{
	return visit(ctx).as<expr_ptr>();
}

