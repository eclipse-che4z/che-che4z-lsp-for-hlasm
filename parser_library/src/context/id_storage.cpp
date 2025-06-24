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

#include <memory>

#include "utils/string_operations.h"

using namespace hlasm_plugin::parser_library::context;

id_index id_storage::small_id(std::string_view value)
{
    char buf[id_index::buffer_size];
    const auto [_, end] = std::ranges::transform(value, buf, [](unsigned char c) { return utils::upper_cased[c]; });
    return id_index(std::string_view(buf, end - buf));
}

size_t id_storage::size() const { return lit_.size(); }

bool id_storage::empty() const { return lit_.empty(); }

std::optional<id_index> id_storage::find(std::string_view value) const
{
    if (value.size() < id_index::buffer_size)
        return small_id(value);

    if (auto tmp = lit_.find(utils::to_upper_copy(std::string(value))); tmp != lit_.end())
        return id_index(std::to_address(tmp));
    else
        return std::nullopt;
}

id_index id_storage::add(std::string_view value)
{
    if (value.size() < id_index::buffer_size)
        return small_id(value);

    return add(std::string(value));
}

id_index id_storage::add(std::string&& value)
{
    if (value.size() < id_index::buffer_size)
        return small_id(value);

    utils::to_upper(value);

    return id_index(std::to_address(lit_.insert(std::move(value)).first));
}
