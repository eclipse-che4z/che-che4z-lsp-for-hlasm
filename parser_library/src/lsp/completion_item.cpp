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

#include "completion_item.h"

namespace hlasm_plugin::parser_library {

completion_item::completion_item(std::string label,
    std::string detail,
    std::string insert_text,
    std::string documentation,
    completion_item_kind kind,
    bool snippet,
    std::string suggestion_for)
    : label(std::move(label))
    , detail(std::move(detail))
    , insert_text(std::move(insert_text))
    , documentation(std::move(documentation))
    , kind(kind)
    , snippet(snippet)
    , suggestion_for(std::move(suggestion_for))
{}

bool completion_item::operator==(const completion_item&) const noexcept = default;

} // namespace hlasm_plugin::parser_library
