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

#ifndef SEMANTICS_EXPRESSION_EVALUATOR_H
#define SEMANTICS_EXPRESSION_EVALUATOR_H

#include "../evaluation_context.h"
#include "diagnosable_ctx.h"
#include "hlasmparser.h"
#include "hlasmparserBaseVisitor.h"

namespace hlasm_plugin::parser_library::expressions {

/**
 * visitor that evaluates expressions
 * matches grammar (for each grammar rule there is a visit...() function
 * that is called when a rule is matched in grammar)
 * */
class expression_evaluator : public parsing::hlasmparserBaseVisitor, public diagnosable_ctx
{
    evaluation_context eval_ctx_;
    // storage of resolved symbol attribute references
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

    /**
     * entry expression rule
     * children are expr_p_space_c
     * expr_p_space_c are a space separated sub-expressions
     * this is needed as some tokens might represent a keyword (such as AND) or a variable name
     * this depends solely on the position of the token within a expr
     * and it is known after evaluation of the left-hand side of the expression
     * */
    virtual antlrcpp::Any visitExpr(parsing::hlasmparser::ExprContext* ctx) override;
    /**
     * helper grammar rule used in unit tests
     * */
    virtual antlrcpp::Any visitExpr_test(parsing::hlasmparser::Expr_testContext* ctx) override;
    virtual antlrcpp::Any visitExpr_statement(parsing::hlasmparser::Expr_statementContext* ctx) override;
    virtual antlrcpp::Any visitExpr_p(parsing::hlasmparser::Expr_pContext* ctx) override;
    virtual antlrcpp::Any visitExpr_s(parsing::hlasmparser::Expr_sContext* ctx) override;
    /**
     * generates space separated expressions
     * these are evaluated in visitExpr()
     * */
    virtual antlrcpp::Any visitExpr_p_space_c(parsing::hlasmparser::Expr_p_space_cContext* ctx) override;
    virtual antlrcpp::Any visitTerm_c(parsing::hlasmparser::Term_cContext* ctx) override;
    virtual antlrcpp::Any visitTerm(parsing::hlasmparser::TermContext* ctx) override;
    virtual antlrcpp::Any visitId_sub(parsing::hlasmparser::Id_subContext* ctx) override;

    virtual antlrcpp::Any visitExpr_p_comma_c(parsing::hlasmparser::Expr_p_comma_cContext* ctx) override;
    virtual antlrcpp::Any visitSubscript(parsing::hlasmparser::SubscriptContext* ctx) override;
    virtual antlrcpp::Any visitString(parsing::hlasmparser::StringContext* ctx) override;
    virtual antlrcpp::Any visitCa_string(parsing::hlasmparser::Ca_stringContext* ctx) override;
    virtual antlrcpp::Any visitCa_string_b(parsing::hlasmparser::Ca_string_bContext* ctx) override;
    virtual antlrcpp::Any visitCa_dupl_factor(parsing::hlasmparser::Ca_dupl_factorContext* ctx) override;
    virtual antlrcpp::Any visitData_attribute(parsing::hlasmparser::Data_attributeContext* ctx) override;
    virtual antlrcpp::Any visitNum(parsing::hlasmparser::NumContext* ctx) override;

    context::SET_t get_ord_attr_value(
        context::data_attr_kind attr, const context::symbol* symbol, context::id_index symbol_name, range symbol_range);

    std::string concat(semantics::char_str_conc* str);
    std::string concat(semantics::var_sym_conc* vs);
    std::string concat(semantics::dot_conc*);
    std::string concat(semantics::equals_conc*);
    std::string concat(semantics::sublist_conc* sublist);

    context::SET_t get_var_sym_value(semantics::var_sym_conc* vs);
    std::pair<context::id_index, std::vector<expr_ptr>> evaluate_var_sym(semantics::var_sym_conc* vs);

    context::SET_t lookup_variable_symbol_attribute(
        context::data_attr_kind attr, context::id_index symbol_name, range symbol_range);
};
} // namespace hlasm_plugin::parser_library::expressions


#endif