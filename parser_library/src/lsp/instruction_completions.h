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

#ifndef HLASMPLUGIN_PARSERLIBRARY_INSTRUCTION_COMPLETIONS_H
#define HLASMPLUGIN_PARSERLIBRARY_INSTRUCTION_COMPLETIONS_H

#include <utility>
#include <vector>

#include "completion_item.h"
#include "context/instruction.h"

namespace hlasm_plugin::parser_library::lsp {

extern const std::vector<std::pair<completion_item, context::instruction_set_affiliation>> instruction_completion_items;

} // namespace hlasm_plugin::parser_library::lsp

#endif
