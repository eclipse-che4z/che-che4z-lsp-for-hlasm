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

#include "sequence_symbol.h"

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::context;

const macro_sequence_symbol* sequence_symbol::access_macro_symbol() const
{
    return kind == sequence_symbol_kind::MACRO ? static_cast<const macro_sequence_symbol*>(this) : nullptr;
}

const opencode_sequence_symbol* sequence_symbol::access_opencode_symbol() const
{
    return kind == sequence_symbol_kind::OPENCODE ? static_cast<const opencode_sequence_symbol*>(this) : nullptr;
}

sequence_symbol::sequence_symbol(id_index name, const sequence_symbol_kind kind, location symbol_location)
    : name(name)
    , symbol_location(std::move(symbol_location))
    , kind(kind)
{}

opencode_sequence_symbol::opencode_sequence_symbol(
    id_index name, location loc, source_position statement_position, source_snapshot snapshot)
    : sequence_symbol(name, sequence_symbol_kind::OPENCODE, std::move(loc))
    , statement_position(statement_position)
    , snapshot(std::move(snapshot))
{}

bool opencode_sequence_symbol::operator==(const opencode_sequence_symbol& oth) const
{
    return snapshot == oth.snapshot;
}

macro_sequence_symbol::macro_sequence_symbol(id_index name, location loc, size_t statement_offset)
    : sequence_symbol(name, sequence_symbol_kind::MACRO, std::move(loc))
    , statement_offset(statement_offset)
{}
