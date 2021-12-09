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

#ifndef HLASMPLUGIN_PARSERLIBRARY_PROCESSING_DATA_DEF_POSTPONED_STATEMENT_H
#define HLASMPLUGIN_PARSERLIBRARY_PROCESSING_DATA_DEF_POSTPONED_STATEMENT_H

#include "checking/data_definition/data_def_type_base.h"
#include "context/ordinary_assembly/dependable.h"
#include "postponed_statement_impl.h"

namespace hlasm_plugin::parser_library::processing {

template<checking::data_instr_type instr_type>
struct data_def_postponed_statement : public postponed_statement_impl, public context::resolvable
{
    data_def_postponed_statement(rebuilt_statement stmt, context::processing_stack_t stmt_location_stack);

    static int32_t get_operands_length(const semantics::operand_list& operands, context::dependency_solver& _solver);

    // Inherited via resolvable
    context::dependency_collector get_dependencies(context::dependency_solver& solver) const override;

    context::symbol_value resolve(context::dependency_solver& solver) const override;
};

} // namespace hlasm_plugin::parser_library::processing
#endif
