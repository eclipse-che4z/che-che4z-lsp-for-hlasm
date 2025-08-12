/*
 * Copyright (c) 2025 Broadcom.
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

#ifndef HLASMPLUGIN_PARSERLIBRARY_CHECKING_DATA_CHECK_H
#define HLASMPLUGIN_PARSERLIBRARY_CHECKING_DATA_CHECK_H

#include <memory>
#include <span>

namespace hlasm_plugin::parser_library {
class diagnostic_collector;
struct range;

namespace context {
class dependency_solver;
} // namespace context

namespace instructions {
class assembler_instruction;
} // namespace instructions

namespace semantics {
struct operand;
} // namespace semantics

} // namespace hlasm_plugin::parser_library

namespace hlasm_plugin::parser_library::checking {

void check_data_instruction_operands(const instructions::assembler_instruction& ai,
    std::span<const std::unique_ptr<semantics::operand>> ops,
    const range& stmt_range,
    context::dependency_solver& dep_solver,
    diagnostic_collector& diags);

} // namespace hlasm_plugin::parser_library::checking

#endif
