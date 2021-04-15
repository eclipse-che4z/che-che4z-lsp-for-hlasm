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

#ifndef PROCESSING_MEMBERS_STATEMENT_PROVIDER_H
#define PROCESSING_MEMBERS_STATEMENT_PROVIDER_H

#include "context/hlasm_context.h"
#include "expressions/evaluation_context.h"
#include "processing/processing_state_listener.h"
#include "processing/statement_fields_parser.h"
#include "statement_provider.h"

namespace hlasm_plugin::parser_library::processing {

// common class for copy and macro statement providers (provider of copy and macro members)
class members_statement_provider : public statement_provider
{
public:
    members_statement_provider(const statement_provider_kind kind,
        analyzing_context ctx,
        statement_fields_parser& parser,
        workspaces::parse_lib_provider& lib_provider,
        processing::processing_state_listener& listener);

    virtual context::shared_stmt_ptr get_next(const statement_processor& processor) override;

protected:
    analyzing_context ctx;
    statement_fields_parser& parser;
    workspaces::parse_lib_provider& lib_provider;
    processing::processing_state_listener& listener;

    virtual context::statement_cache* get_next() = 0;

private:
    const semantics::instruction_si& retrieve_instruction(const context::statement_cache& cache) const;

    void fill_cache(context::statement_cache& cache,
        const semantics::deferred_statement& def_stmt,
        const processing_status& status);

    context::shared_stmt_ptr preprocess_deferred(const statement_processor& processor, context::statement_cache& cache);
};

} // namespace hlasm_plugin::parser_library::processing

#endif
