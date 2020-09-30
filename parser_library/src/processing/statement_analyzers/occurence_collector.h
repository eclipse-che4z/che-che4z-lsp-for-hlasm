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

#include "semantics/operand_visitor.h"

namespace hlasm_plugin::parser_library::processing {

enum class occurence_kind
{
    ORD,
    VAR,
    SEQ,
    INSTR
};

struct symbol_occurence
{
    occurence_kind kind;
    context::id_index name;
    range occurence;
};

class occurence_collector : public semantics::operand_visitor
{
    const occurence_kind collector_kind_;

    std::vector<symbol_occurence> occurences_;

public:
    virtual void visit(const semantics::empty_operand& op) override;
    virtual void visit(const semantics::model_operand& op) override;
    virtual void visit(const semantics::expr_machine_operand& op) override;
    virtual void visit(const semantics::address_machine_operand& op) override;
    virtual void visit(const semantics::expr_assembler_operand& op) override;
    virtual void visit(const semantics::using_instr_assembler_operand& op) override;
    virtual void visit(const semantics::complex_assembler_operand& op) override;
    virtual void visit(const semantics::string_assembler_operand& op) override;
    virtual void visit(const semantics::data_def_operand& op) override;
    virtual void visit(const semantics::var_ca_operand& op) override;
    virtual void visit(const semantics::expr_ca_operand& op) override;
    virtual void visit(const semantics::seq_ca_operand& op) override;
    virtual void visit(const semantics::branch_ca_operand& op) override;
    virtual void visit(const semantics::macro_operand_chain& op) override;
    virtual void visit(const semantics::macro_operand_string& op) override;

    static std::vector<symbol_occurence> get_occurences(occurence_kind kind, const semantics::concat_chain& chain);

    static std::vector<symbol_occurence> get_occurences(expressions::mach_expr_ptr mach_expr);
};

} // namespace hlasm_plugin::parser_library::processing

#endif
