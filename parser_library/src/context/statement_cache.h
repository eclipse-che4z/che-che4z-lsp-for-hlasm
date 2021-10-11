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

#ifndef CONTEXT_PROCESSING_STATEMENT_CACHE_H
#define CONTEXT_PROCESSING_STATEMENT_CACHE_H

#include "diagnostic.h"
#include "hlasm_statement.h"
#include "processing/op_code.h"

namespace hlasm_plugin::parser_library::semantics {
struct complete_statement;
}

namespace hlasm_plugin::parser_library::context {

// storage used to store one deferred statement in many parsed formats
// used by macro and copy definition to avoid multiple re-parsing of a deferrend stataments
class statement_cache
{
public:
    struct cached_statement_t
    {
        std::shared_ptr<semantics::complete_statement> stmt;
        std::vector<diagnostic_op> diags;
    };
    // pair of processing format and reparsed statement
    // processing format serves as an identifier of reparsing kind
    using cache_t = std::pair<processing::processing_status_cache_key, cached_statement_t>;

private:
    std::vector<cache_t> cache_;
    shared_stmt_ptr base_stmt_;

public:
    statement_cache(shared_stmt_ptr base);

    bool contains(processing::processing_status_cache_key key) const;

    void insert(processing::processing_status_cache_key key, cached_statement_t statement);

    const cached_statement_t* get(processing::processing_status_cache_key key) const;

    shared_stmt_ptr get_base() const;
};

using cached_block = std::vector<statement_cache>;

} // namespace hlasm_plugin::parser_library::context
#endif
