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

#include "dependant.h"

#include <cassert>

using namespace hlasm_plugin::parser_library::context;

bool attr_ref::operator==(const attr_ref& oth) const
{
    return attribute == oth.attribute && symbol_id == oth.symbol_id;
}