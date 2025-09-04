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

#ifndef CONTEXT_SEQUENCE_SYMBOL_H
#define CONTEXT_SEQUENCE_SYMBOL_H

#include "source_snapshot.h"
#include "statement_id.h"

namespace hlasm_plugin::parser_library::context {

// struct representing sequence symbol that is in open-code
struct opencode_sequence_symbol final
{
    // location of the symbol in code
    location symbol_location;
    // position in opencode
    source_position statement_position;
    // snapshot of source for recovery
    source_snapshot snapshot;
};

// struct representing sequence symbol that is in macro code
struct macro_sequence_symbol final
{
    // location of the symbol in code
    location symbol_location;
    // offset from start of the macro
    statement_id statement_offset;
};

} // namespace hlasm_plugin::parser_library::context

#endif
