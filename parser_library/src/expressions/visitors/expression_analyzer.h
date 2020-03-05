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

#ifndef SEMANTICS_EXPRESSION_ANALYZER_H
#define SEMANTICS_EXPRESSION_ANALYZER_H

#include "hlasmparser.h"
#include "hlasmparserBaseVisitor.h"
#include "../evaluation_context.h"

namespace hlasm_plugin::parser_library::expressions {

//visitor for analyzing any undefined symbol attribute references
class expression_analyzer : public generated::hlasmparserBaseVisitor
{
	evaluation_context eval_ctx_;
public:
	expression_analyzer(evaluation_context eval_ctx);

	std::set<context::id_index> get_undefined_symbol_references(antlr4::ParserRuleContext* expr_context);
	std::set<context::id_index> get_undefined_symbol_references(const semantics::concat_chain& chain);
	std::set<context::id_index> get_undefined_symbol_references(const semantics::var_sym& symbol);
private:
	std::set<context::id_index> visit_ref(antlr4::ParserRuleContext* expr_context);

	virtual antlrcpp::Any visitExpr(generated::hlasmparser::ExprContext* ctx) override;
	virtual antlrcpp::Any visitExpr_p(generated::hlasmparser::Expr_pContext* ctx) override;
	virtual antlrcpp::Any visitExpr_s(generated::hlasmparser::Expr_sContext* ctx) override;
	virtual antlrcpp::Any visitExpr_p_space_c(generated::hlasmparser::Expr_p_space_cContext* ctx) override;
	virtual antlrcpp::Any visitTerm_c(generated::hlasmparser::Term_cContext* ctx) override;
	virtual antlrcpp::Any visitTerm(generated::hlasmparser::TermContext* ctx) override;
	virtual antlrcpp::Any visitId_sub(generated::hlasmparser::Id_subContext* ctx) override;
	virtual antlrcpp::Any visitExpr_p_comma_c(generated::hlasmparser::Expr_p_comma_cContext* ctx) override;
	virtual antlrcpp::Any visitSubscript(generated::hlasmparser::SubscriptContext* ctx) override;
	virtual antlrcpp::Any visitCa_string(generated::hlasmparser::Ca_stringContext* ctx) override;
	virtual antlrcpp::Any visitCa_string_b(generated::hlasmparser::Ca_string_bContext* ctx) override;
	virtual antlrcpp::Any visitCa_dupl_factor(generated::hlasmparser::Ca_dupl_factorContext* ctx) override;
	virtual antlrcpp::Any visitData_attribute(generated::hlasmparser::Data_attributeContext* ctx) override;
};

}
#endif
