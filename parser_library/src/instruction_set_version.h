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

#ifndef HLASMPARSER_PARSERLIBRARY_INSTRUCTION_SET_VERSION_H
#define HLASMPARSER_PARSERLIBRARY_INSTRUCTION_SET_VERSION_H

#include <algorithm>
#include <array>
#include <string>

// Available instruction sets versions
namespace hlasm_plugin::parser_library {
enum class instruction_set_version
{
    ZOP = 1,
    YOP,
    Z9,
    Z10,
    Z11,
    Z12,
    Z13,
    Z14,
    Z15,
    Z16,
    ESA,
    XA,
    _370,
    DOS,
    UNI
};
} // namespace hlasm_plugin::parser_library

#endif
