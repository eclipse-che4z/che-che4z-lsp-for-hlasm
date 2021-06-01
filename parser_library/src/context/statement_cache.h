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

#include "hlasm_statement.h"
#include "processing/processing_format.h"
#include "diagnostic.h"

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
    using cache_t = std::pair<processing::processing_form, cached_statement_t>;

private:
    std::vector<cache_t> cache_;
    shared_stmt_ptr base_stmt_;

public:
    statement_cache(shared_stmt_ptr base);

    bool contains(processing::processing_form format) const;

    void insert(processing::processing_form format, cached_statement_t statement);

    const cached_statement_t & get(processing::processing_form format) const;

    shared_stmt_ptr get_base() const;
};

using cached_block = std::vector<statement_cache>;

} // namespace hlasm_plugin::parser_library::context
#endif
