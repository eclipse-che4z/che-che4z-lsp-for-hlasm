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

#ifndef PROCESSING_OCCURRENCE_COLLECTOR_H
#define PROCESSING_OCCURRENCE_COLLECTOR_H

#include "expressions/conditional_assembly/ca_expr_visitor.h"
#include "expressions/mach_expr_visitor.h"
#include "lsp/symbol_occurrence.h"
#include "semantics/operand_visitor.h"

namespace hlasm_plugin::parser_library::context {
class hlasm_context;
} // namespace hlasm_plugin::parser_library::context

namespace hlasm_plugin::parser_library::processing {

class occurrence_collector final : public semantics::operand_visitor,
                                   public expressions::mach_expr_visitor,
                                   public expressions::ca_expr_visitor
{
public:
    lsp::occurrence_kind collector_kind;
    context::hlasm_context& hlasm_ctx;
    std::vector<lsp::symbol_occurrence>& occurrences;
    bool evaluated_model;


    occurrence_collector(lsp::occurrence_kind collector_kind,
        context::hlasm_context& hlasm_ctx,
        std::vector<lsp::symbol_occurrence>& storage,
        bool evaluated_model);

    void visit(const semantics::empty_operand& op) override;
    void visit(const semantics::model_operand& op) override;
    void visit(const semantics::machine_operand& op) override;
    void visit(const semantics::expr_assembler_operand& op) override;
    void visit(const semantics::using_instr_assembler_operand& op) override;
    void visit(const semantics::complex_assembler_operand& op) override;
    void visit(const semantics::string_assembler_operand& op) override;
    void visit(const semantics::text_assembler_operand& op) override;
    void visit(const semantics::data_def_operand& op) override;
    void visit(const semantics::var_ca_operand& op) override;
    void visit(const semantics::expr_ca_operand& op) override;
    void visit(const semantics::seq_ca_operand& op) override;
    void visit(const semantics::branch_ca_operand& op) override;
    void visit(const semantics::macro_operand& op) override;

    void get_occurrence(const semantics::variable_symbol& var);
    void get_occurrence(const semantics::seq_sym& seq);
    void get_occurrence(context::id_index ord, const range& ord_range);
    void get_occurrence(const semantics::concat_chain& chain);
    void get_occurrence(const semantics::literal_si& var);

private:
    void visit(const expressions::mach_expr_constant& expr) override;
    void visit(const expressions::mach_expr_data_attr& expr) override;
    void visit(const expressions::mach_expr_data_attr_literal& expr) override;
    void visit(const expressions::mach_expr_symbol& expr) override;
    void visit(const expressions::mach_expr_location_counter& expr) override;
    void visit(const expressions::mach_expr_literal& expr) override;

    void visit(const expressions::ca_constant& expr) override;
    void visit(const expressions::ca_expr_list& expr) override;
    void visit(const expressions::ca_function& expr) override;
    void visit(const expressions::ca_string& expr) override;
    void visit(const expressions::ca_symbol& expr) override;
    void visit(const expressions::ca_symbol_attribute& expr) override;
    void visit(const expressions::ca_var_sym& expr) override;
};

} // namespace hlasm_plugin::parser_library::processing

#endif
