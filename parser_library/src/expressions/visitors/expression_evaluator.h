#ifndef SEMANTICS_EXPRESSION_EVALUATOR_H
#define SEMANTICS_EXPRESSION_EVALUATOR_H

#include "../../generated/hlasmparser.h"
#include "../../generated/hlasmparserBaseVisitor.h"
#include "../evaluation_context.h"
#include "../../diagnosable_ctx.h"

namespace hlasm_plugin::parser_library::expressions {
class expression_evaluator : public generated::hlasmparserBaseVisitor, public diagnosable_ctx
{
	evaluation_context eval_ctx_;
	const processing::attribute_provider::resolved_reference_storage* resolved_refs_;
public:
	expression_evaluator(evaluation_context eval_ctx);

	expr_ptr evaluate_expression(antlr4::ParserRuleContext* expr_context);

	std::vector<expr_ptr> evaluate_expressions(std::vector<antlr4::ParserRuleContext*> exprs_context);

	std::string concatenate_chain(const semantics::concat_chain& chain);

	virtual void collect_diags() const override;
private:
	std::string concatenate(const semantics::concat_chain& chain);

	expr_ptr visitE(antlr4::ParserRuleContext* ctx);

	virtual antlrcpp::Any visitExpr(generated::hlasmparser::ExprContext* ctx) override;
	virtual antlrcpp::Any visitExpr_test(generated::hlasmparser::Expr_testContext* ctx) override;
	virtual antlrcpp::Any visitExpr_statement(generated::hlasmparser::Expr_statementContext* ctx) override;
	virtual antlrcpp::Any visitExpr_p(generated::hlasmparser::Expr_pContext* ctx) override;
	virtual antlrcpp::Any visitExpr_s(generated::hlasmparser::Expr_sContext* ctx) override;
	virtual antlrcpp::Any visitExpr_p_space_c(generated::hlasmparser::Expr_p_space_cContext* ctx) override;
	virtual antlrcpp::Any visitTerm_c(generated::hlasmparser::Term_cContext* ctx) override;
	virtual antlrcpp::Any visitTerm(generated::hlasmparser::TermContext* ctx) override;
	virtual antlrcpp::Any visitId_sub(generated::hlasmparser::Id_subContext* ctx) override;
	virtual antlrcpp::Any visitExpr_p_comma_c(generated::hlasmparser::Expr_p_comma_cContext* ctx) override;
	virtual antlrcpp::Any visitSubscript(generated::hlasmparser::SubscriptContext* ctx) override;
	virtual antlrcpp::Any visitString(generated::hlasmparser::StringContext* ctx) override;
	virtual antlrcpp::Any visitCa_string(generated::hlasmparser::Ca_stringContext* ctx) override;
	virtual antlrcpp::Any visitCa_string_b(generated::hlasmparser::Ca_string_bContext* ctx) override;
	virtual antlrcpp::Any visitCa_dupl_factor(generated::hlasmparser::Ca_dupl_factorContext* ctx) override;
	virtual antlrcpp::Any visitData_attribute(generated::hlasmparser::Data_attributeContext* ctx) override;
	virtual antlrcpp::Any visitNum(generated::hlasmparser::NumContext* ctx) override;

	context::SET_t get_ord_attr_value(context::data_attr_kind attr, const context::symbol* symbol, context::id_index symbol_name, range symbol_range);

	std::string concat(semantics::char_str* str);
	std::string concat(semantics::var_sym* vs);
	std::string concat(semantics::dot*);
	std::string concat(semantics::equals*);
	std::string concat(semantics::sublist* sublist);

	context::SET_t get_var_sym_value(semantics::var_sym* vs);
	std::pair<context::id_index, std::vector<expr_ptr>> evaluate_var_sym(semantics::var_sym* vs);

	context::SET_t lookup_variable_symbol_attribute(context::data_attr_kind attr, context::id_index symbol_name, range symbol_range);

};
}


#endif