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

#ifndef HLASMPLUGIN_PARSERLIBRARY_INSTRUCTION_CHECKER_H
#define HLASMPLUGIN_PARSERLIBRARY_INSTRUCTION_CHECKER_H

#include <span>
#include <string_view>

namespace hlasm_plugin::parser_library {
class diagnostic_collector;
struct range;
} // namespace hlasm_plugin::parser_library

namespace hlasm_plugin::parser_library::checking {
class asm_operand;
class machine_operand;

bool check_asm_ops(std::string_view instruction_name,
    std::span<const asm_operand* const> operand_vector,
    const range& stmt_range,
    const diagnostic_collector& add_diagnostic);

bool check_mach_ops(std::string_view instruction_name,
    std::span<const machine_operand* const> operand_vector,
    const range& stmt_range,
    const diagnostic_collector& add_diagnostic);

} // namespace hlasm_plugin::parser_library::checking

#endif
