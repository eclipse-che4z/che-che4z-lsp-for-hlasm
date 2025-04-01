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

#include "statement_cache.h"

#include "semantics/statement.h"

namespace hlasm_plugin::parser_library::context {

statement_cache::statement_cache(shared_stmt_ptr base) noexcept
    : base_stmt_(std::move(base))
{}

const statement_cache::cached_statement_t& statement_cache::insert(
    processing::processing_status_cache_key key, cached_statement_t statement)
{
    return cache_.emplace_back(key, std::move(statement)).second;
}

const statement_cache::cached_statement_t* statement_cache::get(
    processing::processing_status_cache_key key) const noexcept
{
    for (const auto& entry : cache_)
        if (entry.first == key)
            return &entry.second;
    return nullptr;
}

} // namespace hlasm_plugin::parser_library::context
