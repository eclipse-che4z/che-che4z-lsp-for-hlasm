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

#ifndef CONTEXT_LITERAL_STORAGE_H
#define CONTEXT_LITERAL_STORAGE_H

#include <optional>
#include <string>
#include <unordered_set>

#include "id_index.h"

namespace hlasm_plugin::parser_library::context {
// storage for identifiers
// changes strings of identifiers to indexes of this storage class for easier and unified work
class id_storage
{
    std::unordered_set<std::string> lit_;

    static id_index small_id(std::string_view value);

public:
    size_t size() const;
    bool empty() const;

    std::optional<id_index> find(std::string_view val) const;

    id_index add(std::string_view value);
    id_index add(std::string&& value);
};

} // namespace hlasm_plugin::parser_library::context

#endif
