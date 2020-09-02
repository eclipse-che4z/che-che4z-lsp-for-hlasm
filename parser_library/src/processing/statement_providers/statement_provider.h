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

#ifndef PROCESSING_STATEMENT_PROVIDER_H
#define PROCESSING_STATEMENT_PROVIDER_H

#include "expressions/evaluation_context.h"
#include "processing/processing_state_listener.h"
#include "processing/statement_processors/statement_processor.h"
#include "statement_provider_kind.h"

namespace hlasm_plugin::parser_library::processing {

class statement_provider;
using provider_ptr = std::unique_ptr<statement_provider>;

// interface for statement providers
// till they are finished they provide statements to statement processors
class statement_provider
{
public:
    const statement_provider_kind kind;

    statement_provider(const statement_provider_kind kind);

    // processes next statement with help of a processor
    virtual void process_next(statement_processor& processor) = 0;

    // checks whether provider has finished
    virtual bool finished() const = 0;

    virtual ~statement_provider() = default;

protected:
    static bool try_trigger_attribute_lookahead(const semantics::instruction_si& instruction,
        expressions::evaluation_context eval_ctx,
        processing::processing_state_listener& listener);
    static bool try_trigger_attribute_lookahead(const context::hlasm_statement& statement,
        expressions::evaluation_context eval_ctx,
        processing::processing_state_listener& listener);

private:
    static void trigger_attribute_lookahead(std::set<context::id_index> references,
        const expressions::evaluation_context& eval_ctx,
        processing::processing_state_listener& listener);

    static std::set<context::id_index> process_label(
        const semantics::label_si& label, expressions::evaluation_context& eval_ctx);
    static std::set<context::id_index> process_instruction(
        const semantics::instruction_si& instruction, expressions::evaluation_context& eval_ctx);
    static std::set<context::id_index> process_operands(
        const semantics::operands_si& operands, expressions::evaluation_context& eval_ctx);
};

} // namespace hlasm_plugin::parser_library::processing

#endif
