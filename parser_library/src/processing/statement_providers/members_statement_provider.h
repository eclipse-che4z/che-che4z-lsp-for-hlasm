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

#include <utility>

#include "processing/processing_state_listener.h"
#include "processing/statement_fields_parser.h"
#include "statement_provider.h"

namespace hlasm_plugin::parser_library {
class parse_lib_provider;
} // namespace hlasm_plugin::parser_library

namespace hlasm_plugin::parser_library::context {
struct copy_member_invocation;
struct macro_invocation;
} // namespace hlasm_plugin::parser_library::context

namespace hlasm_plugin::parser_library::workspaces {
class parse_lib_provider;
} // namespace hlasm_plugin::parser_library::workspaces

namespace hlasm_plugin::parser_library::processing {

// common class for copy and macro statement providers (provider of copy and macro members)
class members_statement_provider : public statement_provider
{
    std::vector<context::id_index> lookahead_references;

public:
    members_statement_provider(const statement_provider_kind kind,
        const analyzing_context& ctx,
        statement_fields_parser& parser,
        parse_lib_provider& lib_provider,
        processing::processing_state_listener& listener,
        diagnostic_op_consumer& diag_consumer);

    context::shared_stmt_ptr get_next(const statement_processor& processor) override;


protected:
    analyzing_context m_ctx;
    statement_fields_parser& m_parser;
    parse_lib_provider& m_lib_provider;
    processing::processing_state_listener& m_listener;
    diagnostic_op_consumer& m_diagnoser;
    std::optional<std::optional<context::id_index>> m_resolved_instruction;

    virtual std::pair<context::statement_cache*, std::optional<std::optional<context::id_index>>> get_next() = 0;
    virtual std::vector<diagnostic_op> filter_cached_diagnostics(
        const semantics::deferred_statement& stmt, bool no_operands) const = 0;
    void go_back(std::optional<context::id_index> ri) { m_resolved_instruction.emplace(std::move(ri)); }

private:
    const semantics::instruction_si* retrieve_instruction(const context::statement_cache& cache) const;

    const context::statement_cache::cached_statement_t& fill_cache(context::statement_cache& cache,
        std::shared_ptr<const semantics::deferred_statement> def_stmt,
        const processing_status& status);

    context::shared_stmt_ptr preprocess_deferred(const statement_processor& processor,
        context::statement_cache& cache,
        processing_status status,
        context::shared_stmt_ptr base_stmt);
};

} // namespace hlasm_plugin::parser_library::processing

#endif
