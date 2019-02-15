#ifndef SEMANTICS_EXPRESSION_VISITOR_H
#define SEMANTICS_EXPRESSION_VISITOR_H

#include "../generated/hlasmparser.h"
#include "../generated/hlasmparserBaseVisitor.h"

namespace hlasm_plugin::parser_library::context {
	class expression_visitor : public hlasm_plugin::parser_library::generated::hlasmparserBaseVisitor
	{
	public:
		virtual antlrcpp::Any visitExpr(hlasm_plugin::parser_library::generated::hlasmparser::ExprContext *ctx) override;
		virtual antlrcpp::Any visitExpr_test(hlasm_plugin::parser_library::generated::hlasmparser::Expr_testContext *ctx) override;
		virtual antlrcpp::Any visitExpr_statement(hlasm_plugin::parser_library::generated::hlasmparser::Expr_statementContext *ctx) override;
		virtual antlrcpp::Any visitExpr_p(hlasm_plugin::parser_library::generated::hlasmparser::Expr_pContext *ctx) override;
		virtual antlrcpp::Any visitExpr_s(hlasm_plugin::parser_library::generated::hlasmparser::Expr_sContext *ctx) override;
		virtual antlrcpp::Any visitExpr_p_space_c(hlasm_plugin::parser_library::generated::hlasmparser::Expr_p_space_cContext *ctx) override;
		virtual antlrcpp::Any visitTerm_c(hlasm_plugin::parser_library::generated::hlasmparser::Term_cContext * ctx) override;
		virtual antlrcpp::Any visitTerm(hlasm_plugin::parser_library::generated::hlasmparser::TermContext * ctx) override;
		virtual antlrcpp::Any visitExpr_p_comma_c(hlasm_plugin::parser_library::generated::hlasmparser::Expr_p_comma_cContext * ctx) override;
		virtual antlrcpp::Any visitSubscript(hlasm_plugin::parser_library::generated::hlasmparser::SubscriptContext * ctx) override;
		virtual antlrcpp::Any visitString(hlasm_plugin::parser_library::generated::hlasmparser::StringContext * ctx) override;
		virtual antlrcpp::Any visitCa_string(hlasm_plugin::parser_library::generated::hlasmparser::Ca_stringContext * ctx) override;
		virtual antlrcpp::Any visitCa_string_b(hlasm_plugin::parser_library::generated::hlasmparser::Ca_string_bContext * ctx) override;
		virtual antlrcpp::Any visitCa_dupl_factor(hlasm_plugin::parser_library::generated::hlasmparser::Ca_dupl_factorContext * ctx) override;
		virtual antlrcpp::Any visitData_attribute(hlasm_plugin::parser_library::generated::hlasmparser::Data_attributeContext * ctx) override;
		void set_semantic_analyzer(semantics::semantic_analyzer* analyzer);
	private:
		semantics::semantic_analyzer* analyzer_;
		hlasm_plugin::parser_library::semantics::expr_ptr visitE(antlr4::ParserRuleContext* ctx);
	};
}


#endif