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

#ifndef HLASMPLUGIN_PARSERLIBRARY_CONTEXT_INSTRUCTION_TYPE_H
#define HLASMPLUGIN_PARSERLIBRARY_CONTEXT_INSTRUCTION_TYPE_H

namespace hlasm_plugin {
namespace parser_library {
namespace context {

// enum class for all instruction types
enum class instruction_type
{
    MACH,
    ASM,
    MAC,
    CA,
    UNDEF
};

} // namespace context
} // namespace parser_library
} // namespace hlasm_plugin

#endif
