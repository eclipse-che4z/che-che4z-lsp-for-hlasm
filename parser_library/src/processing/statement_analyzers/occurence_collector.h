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

#ifndef PROCESSING_OCCURENCE_COLLECTOR_H
#define PROCESSING_OCCURENCE_COLLECTOR_H

#include "expressions/conditional_assembly/ca_expr_visitor.h"
#include "expressions/mach_expr_visitor.h"
#include "lsp/symbol_occurence.h"
#include "semantics/operand_visitor.h"

namespace hlasm_plugin::parser_library::processing {

class occurence_collector : public semantics::operand_visitor,
                            public expressions::mach_expr_visitor,
                            public expressions::ca_expr_visitor
{
public:
    lsp::occurence_kind collector_kind;
    context::hlasm_context& hlasm_ctx;
    std::vector<lsp::symbol_occurence>& occurences;


    occurence_collector(
        lsp::occurence_kind collector_kind, context::hlasm_context& hlasm_ctx, lsp::occurence_storage& storage);

    void visit(const semantics::empty_operand& op) override;
    void visit(const semantics::model_operand& op) override;
    void visit(const semantics::expr_machine_operand& op) override;
    void visit(const semantics::address_machine_operand& op) override;
    void visit(const semantics::expr_assembler_operand& op) override;
    void visit(const semantics::using_instr_assembler_operand& op) override;
    void visit(const semantics::complex_assembler_operand& op) override;
    void visit(const semantics::string_assembler_operand& op) override;
    void visit(const semantics::data_def_operand& op) override;
    void visit(const semantics::var_ca_operand& op) override;
    void visit(const semantics::expr_ca_operand& op) override;
    void visit(const semantics::seq_ca_operand& op) override;
    void visit(const semantics::branch_ca_operand& op) override;
    void visit(const semantics::macro_operand_chain& op) override;
    void visit(const semantics::macro_operand_string& op) override;

    void get_occurence(const semantics::variable_symbol& var);
    void get_occurence(const semantics::seq_sym& seq);
    void get_occurence(context::id_index ord, const range& ord_range);
    void get_occurence(const semantics::concat_chain& chain);

private:
    void visit(const expressions::mach_expr_constant& expr) override;
    void visit(const expressions::mach_expr_data_attr& expr) override;
    void visit(const expressions::mach_expr_symbol& expr) override;
    void visit(const expressions::mach_expr_location_counter& expr) override;
    void visit(const expressions::mach_expr_self_def& expr) override;
    void visit(const expressions::mach_expr_default& expr) override;

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
