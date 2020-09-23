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

#ifndef SEMANTICS_OPERAND_VISITOR_H
#define SEMANTICS_OPERAND_VISITOR_H

#include "operand.h"

// the file contains base visitor class over operands

namespace hlasm_plugin::parser_library::semantics {

class operand_visitor
{
public:
    virtual void visit(const empty_operand& op) = 0;
    virtual void visit(const model_operand& op) = 0;
    virtual void visit(const expr_machine_operand& op) = 0;
    virtual void visit(const address_machine_operand& op) = 0;
    virtual void visit(const expr_assembler_operand& op) = 0;
    virtual void visit(const using_instr_assembler_operand& op) = 0;
    virtual void visit(const complex_assembler_operand& op) = 0;
    virtual void visit(const string_assembler_operand& op) = 0;
    virtual void visit(const data_def_operand& op) = 0;
    virtual void visit(const var_ca_operand& op) = 0;
    virtual void visit(const expr_ca_operand& op) = 0;
    virtual void visit(const seq_ca_operand& op) = 0;
    virtual void visit(const branch_ca_operand& op) = 0;
    virtual void visit(const macro_operand_chain& op) = 0;
    virtual void visit(const macro_operand_string& op) = 0;
};

} // namespace hlasm_plugin::parser_library::semantics

#endif
