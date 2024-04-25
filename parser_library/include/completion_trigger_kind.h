/*
 * Copyright (c) 2024 Broadcom.
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

#ifndef HLASMPLUGIN_PARSERLIBRARY_COMPLETION_TRIGGER_KIND_H
#define HLASMPLUGIN_PARSERLIBRARY_COMPLETION_TRIGGER_KIND_H

namespace hlasm_plugin::parser_library {

enum class completion_trigger_kind
{
    invoked = 1,
    trigger_character = 2,
    trigger_for_incomplete_completions = 3
};

} // namespace hlasm_plugin::parser_library

#endif
