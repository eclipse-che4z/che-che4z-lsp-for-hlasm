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

#ifndef HLASMPLUGIN_PARSERLIBRARY_CHECKING_DATA_DEFINITION_OPERAND_H
#define HLASMPLUGIN_PARSERLIBRARY_CHECKING_DATA_DEFINITION_OPERAND_H

#include "data_def_fields.h"

namespace hlasm_plugin::parser_library::expressions {
class mach_expression;
} // namespace hlasm_plugin::parser_library::expressions

namespace hlasm_plugin::parser_library::checking {
struct data_definition_common final
{
    std::int32_t dupl_factor = 1;
    std::int32_t length = 0;
    std::int32_t exponent = 0;
    std::int32_t scale = 0;

    const range* rng_dupl_factor = nullptr;
    const range* rng_length = nullptr;
    const range* rng_exponent = nullptr;
    const range* rng_scale = nullptr;

    bool length_in_bits = false;

    [[nodiscard]] constexpr bool has_dupl_factor() const noexcept { return rng_dupl_factor != nullptr; }
    [[nodiscard]] constexpr bool has_length() const noexcept { return rng_length != nullptr; }
    [[nodiscard]] constexpr bool has_exponent() const noexcept { return rng_exponent != nullptr; }
    [[nodiscard]] constexpr bool has_scale() const noexcept { return rng_scale != nullptr; }
};

// Represents evaluated (resolved machine expressions) data definition operand suitable for checking.
struct data_definition_operand final
{
    nominal_value_t nominal_value;
};

} // namespace hlasm_plugin::parser_library::checking


#endif
