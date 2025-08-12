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

namespace hlasm_plugin::parser_library::checking {
// Represents evaluated (resolved machine expressions) data definition operand suitable for checking.
struct data_definition_operand final
{
    range operand_range;

    data_def_field<std::int32_t> dupl_factor;
    data_def_field<char> type;
    data_def_field<char> extension;
    data_def_length_t length;

    exponent_modifier_t exponent;
    scale_modifier_t scale;

    nominal_value_t nominal_value;
};

} // namespace hlasm_plugin::parser_library::checking


#endif
