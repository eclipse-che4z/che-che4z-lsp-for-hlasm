/*
 * Copyright (c) 2022 Broadcom.
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

#include "statement_fields.h"


namespace hlasm_plugin::parser_library::semantics {

void label_si::resolve(diagnostic_op_consumer& diag)
{
    switch (type)
    {
        case label_si_type::ORD:
            break;
        case label_si_type::SEQ:
            break;
        case label_si_type::VAR:
            std::get<vs_ptr>(value)->resolve(diag);
            break;
        case label_si_type::MAC:
            break;
        case label_si_type::CONC:
            for (const auto& c : std::get<concat_chain>(value))
                c->resolve(diag);
            break;
        case label_si_type::EMPTY:
            break;
    }
}

void instruction_si::resolve(diagnostic_op_consumer& diag)
{
    switch (type)
    {
        case instruction_si_type::ORD:
            break;
        case instruction_si_type::CONC:
            for (const auto& c : std::get<concat_chain>(value))
                c->resolve(diag);
            break;
        case instruction_si_type::EMPTY:
            break;
    }
}

} // namespace hlasm_plugin::parser_library::semantics
