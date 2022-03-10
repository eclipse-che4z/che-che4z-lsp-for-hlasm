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

#include "variable.h"

using namespace hlasm_plugin::parser_library::debugging;

std::string variable::get_value() const
{
    if (value_)
        return *value_;
    else
        return get_string_value();
}

const std::string& variable::get_name() const
{
    if (name_)
        return *name_;
    else
        return get_string_name();
}
