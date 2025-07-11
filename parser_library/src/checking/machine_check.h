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

#ifndef HLASMPLUGIN_PARSERLIBRARY_CHECKING_MACHINE_CHECK_H
#define HLASMPLUGIN_PARSERLIBRARY_CHECKING_MACHINE_CHECK_H

#include <memory>

#include "instructions/instruction.h"
#include "range.h"

namespace hlasm_plugin::parser_library {
class diagnostic_collector;
namespace context {
class dependency_solver;
}
namespace semantics {
struct operand;
}
} // namespace hlasm_plugin::parser_library

namespace hlasm_plugin::parser_library::checking {
void check_machine_instruction_operands(const instructions::machine_instruction& mi,
    std::string_view mi_name,
    std::span<const std::unique_ptr<semantics::operand>> ops,
    const range& stmt_range,
    context::dependency_solver& dep_solver,
    diagnostic_collector& diags);
void check_mnemonic_code_operands(const instructions::mnemonic_code& mn,
    std::string_view mi_name,
    std::span<const std::unique_ptr<semantics::operand>> ops,
    const range& stmt_range,
    context::dependency_solver& dep_solver,
    diagnostic_collector& diags);

struct machine_operand final
{
    int displacement;
    int first_op;
    int second_op;
    bool valid;
    [[maybe_unused]] bool first_op_derived;
    unsigned char source;
};

} // namespace hlasm_plugin::parser_library::checking

#endif
