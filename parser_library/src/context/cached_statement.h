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

#ifndef CONTEXT_PROCESSING_CACHED_STATEMENT_H
#define CONTEXT_PROCESSING_CACHED_STATEMENT_H

#include "hlasm_statement.h"
#include "processing/processing_format.h"

namespace hlasm_plugin {
namespace parser_library {
namespace semantics {
struct statement_si_defer_done;
}
namespace context {

// storage used to store one deferred statement in many parsed formats
// used by macro and copy definition to avoid multiple re-parsing of a deferrend stataments
class cached_statement_storage
{
public:
    // reparsed statement type
    using cache_entry_t = std::shared_ptr<semantics::statement_si_defer_done>;
    // pair of processing format and reparsed statement
    // processing format serves as an identifier of reparsing kind
    using cached_statement_t = std::pair<processing::processing_form, cache_entry_t>;

private:
    std::vector<cached_statement_t> cache_;
    shared_stmt_ptr base_stmt_;

public:
    cached_statement_storage(shared_stmt_ptr base);

    bool contains(processing::processing_form format) const;

    void insert(processing::processing_form format, cache_entry_t statement);

    cache_entry_t get(processing::processing_form format) const;

    shared_stmt_ptr get_base() const;
};

using cached_block = std::vector<cached_statement_storage>;

} // namespace context
} // namespace parser_library
} // namespace hlasm_plugin
#endif
