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

#include "id_storage.h"

#include "common_types.h"

using namespace hlasm_plugin::parser_library::context;

id_index id_storage::small_id(std::string_view value)
{
    char buffer[id_index::buffer_size];
    char* end = std::transform(value.begin(), value.end(), buffer, [](unsigned char c) { return (char)toupper(c); });
    return id_index(std::string_view(buffer, end - buffer));
}

size_t id_storage::size() const { return lit_.size(); }

bool id_storage::empty() const { return lit_.empty(); }

std::optional<id_index> id_storage::find(std::string_view value) const
{
    if (value.size() < id_index::buffer_size)
        return small_id(value);

    if (auto tmp = lit_.find(to_upper_copy(std::string(value))); tmp != lit_.end())
        return id_index(&*tmp);
    else
        return std::nullopt;
}

id_index id_storage::add(std::string_view value)
{
    if (value.size() < id_index::buffer_size)
        return small_id(value);

    return id_index(&*lit_.insert(to_upper_copy(std::string(value))).first);
}

id_index id_storage::add(std::string&& value)
{
    if (value.size() < id_index::buffer_size)
        return small_id(value);

    to_upper(value);

    return id_index(&*lit_.insert(std::move(value)).first);
}
