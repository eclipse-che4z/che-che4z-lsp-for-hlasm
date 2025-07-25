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

#ifndef HLASMPLUGIN_PARSERLIBRARY_INSTRUCTIONS_CONTEXT_TYPE_H
#define HLASMPLUGIN_PARSERLIBRARY_INSTRUCTIONS_CONTEXT_TYPE_H

namespace hlasm_plugin::parser_library::context {

// enum class for all instruction types
enum class instruction_type
{
    MACH,
    MNEMO,
    ASM,
    MAC,
    CA,
    UNDEF
};

} // namespace hlasm_plugin::parser_library::context

#endif
