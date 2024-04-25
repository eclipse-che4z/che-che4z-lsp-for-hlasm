/*
 * Copyright (c) 2021 Broadcom.
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

#include "document_symbol_item.h"

namespace hlasm_plugin::parser_library {

document_symbol_item::document_symbol_item(std::string name, document_symbol_kind kind, range symbol_range)
    : name(std::move(name))
    , kind(kind)
    , symbol_range(symbol_range)
    , symbol_selection_range(symbol_range)
{}
document_symbol_item::document_symbol_item(
    std::string name, document_symbol_kind kind, range symbol_range, std::vector<document_symbol_item> children)
    : name(std::move(name))
    , kind(kind)
    , symbol_range(symbol_range)
    , symbol_selection_range(symbol_range)
    , children(std::move(children))
{}

} // namespace hlasm_plugin::parser_library
