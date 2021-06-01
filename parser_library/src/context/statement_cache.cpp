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

statement_cache::statement_cache(shared_stmt_ptr base)
    : base_stmt_(std::move(base))
{}

bool statement_cache::contains(processing::processing_form format) const
{
    for (const auto& entry : cache_)
        if (entry.first == format)
            return true;
    return false;
}

void statement_cache::insert(processing::processing_form format, cached_statement_t statement)
{
    cache_.emplace_back(format, std::move(statement));
}

statement_cache::cached_statement_t statement_cache::get(processing::processing_form format) const
{
    for (const auto& entry : cache_)
        if (entry.first == format)
            return entry.second;
    return nullptr;
}

shared_stmt_ptr statement_cache::get_base() const { return base_stmt_; }

} // namespace hlasm_plugin::parser_library::context
