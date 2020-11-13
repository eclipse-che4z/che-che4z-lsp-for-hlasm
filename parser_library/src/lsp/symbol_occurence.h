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

#ifndef LSP_SYMBOL_OCCURENCE_H
#define LSP_SYMBOL_OCCURENCE_H

#include <vector>

#include "context/id_storage.h"
#include "range.h"

namespace hlasm_plugin::parser_library::lsp {

enum class occurence_kind
{
    ORD,
    VAR,
    SEQ,
    INSTR
};

struct symbol_occurence
{
    occurence_kind kind;
    context::id_index name;
    range occurence_range;
};

using occurence_storage = std::vector<symbol_occurence>;

} // namespace hlasm_plugin::parser_library::lsp

#endif