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

#include "data_definition_operand.h"

#include "checking/diagnostic_collector.h"

using namespace hlasm_plugin::parser_library::checking;
using namespace hlasm_plugin::parser_library;

const data_def_type* data_definition_operand::access_data_def_type() const
{
    return data_def_type::access_data_def_type(type.value, extension.value);
}

std::pair<const data_def_type*, bool> data_definition_operand::check_type_and_extension(
    const diagnostic_collector& add_diagnostic) const
{
    auto found = data_def_type::types_and_extensions.find({ type.value, extension.present ? extension.value : '\0' });

    if (found != data_def_type::types_and_extensions.end())
        return { found->second.get(), true };

    if (extension.present)
    {
        found = data_def_type::types_and_extensions.find({ type.value, '\0' });
        if (found != data_def_type::types_and_extensions.end())
        {
            add_diagnostic(diagnostic_op::error_D013(extension.rng, std::string(1, type.value)));
            return { found->second.get(), false };
        }
    }

    add_diagnostic(diagnostic_op::error_D012(type.rng));
    return { nullptr, false };
}
