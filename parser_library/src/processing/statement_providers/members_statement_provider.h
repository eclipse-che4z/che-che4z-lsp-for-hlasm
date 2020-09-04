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

#include "context/cached_statement.h"
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
        context::hlasm_context& hlasm_ctx,
        statement_fields_parser& parser,
        workspaces::parse_lib_provider& lib_provider,
        processing::processing_state_listener& listener);

    virtual void process_next(statement_processor& processor) override;

protected:
    context::hlasm_context& hlasm_ctx;
    statement_fields_parser& parser;
    workspaces::parse_lib_provider& lib_provider;
    processing::processing_state_listener& listener;

    virtual context::cached_statement_storage* get_next() = 0;

private:
    const semantics::instruction_si& retrieve_instruction(context::cached_statement_storage& cache) const;

    void fill_cache(context::cached_statement_storage& cache,
        const semantics::deferred_statement& def_stmt,
        const processing_status& status);

    void preprocess_deferred(statement_processor& processor, context::cached_statement_storage& cache);

    template<typename T>
    void do_process_statement(statement_processor& processor, T statement)
    {
        if (processor.kind == processing_kind::ORDINARY
            && try_trigger_attribute_lookahead(*statement, { hlasm_ctx, lib_provider }, listener))
            return;

        processor.process_statement(std::move(statement));
    }
};

} // namespace hlasm_plugin::parser_library::processing

#endif
