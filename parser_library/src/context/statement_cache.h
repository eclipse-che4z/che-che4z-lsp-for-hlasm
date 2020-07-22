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

namespace hlasm_plugin {
namespace parser_library {
namespace context {

// storage used to store one deferred statement in many parsed formats
// used by macro and copy definition to avoid multiple re-parsing of a deferrend stataments
class statement_cache
{
public:
    // pair of processing format and reparsed statement
    // processing format serves as an identifier of reparsing kind
    using cached_statement_t = std::pair<processing::processing_form, shared_stmt_ptr>;

private:
    std::vector<cached_statement_t> cache_;
    shared_stmt_ptr base_stmt_;

public:
    statement_cache(shared_stmt_ptr base);

    bool contains(processing::processing_form format) const;

    void insert(processing::processing_form format, shared_stmt_ptr statement);

    shared_stmt_ptr get(processing::processing_form format) const;

    shared_stmt_ptr get_base() const;
};

using cached_block = std::vector<statement_cache>;

} // namespace context
} // namespace parser_library
} // namespace hlasm_plugin
#endif
