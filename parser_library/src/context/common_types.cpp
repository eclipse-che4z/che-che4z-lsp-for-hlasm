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

#include "common_types.h"

#include <cassert>

namespace hlasm_plugin::parser_library::context {

bool SET_t::operator==(const SET_t& r) const noexcept
{
    if (value_type != r.value_type)
        return false;

    switch (value_type)
    {
        case SET_t_enum::A_TYPE:
        case SET_t_enum::B_TYPE:
            return a_value == r.a_value;
        case SET_t_enum::C_TYPE:
            return c_value == r.c_value;
        case SET_t_enum::UNDEF_TYPE:
            return true;
        default:
            assert(false);
            return false;
    }
}

} // namespace hlasm_plugin::parser_library::context
