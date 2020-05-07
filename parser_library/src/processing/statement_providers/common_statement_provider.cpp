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

#include "common_statement_provider.h"

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::processing;

common_statement_provider::common_statement_provider(
    const statement_provider_kind kind, context::hlasm_context& hlasm_ctx, statement_fields_parser& parser)
    : statement_provider(kind)
    , hlasm_ctx(hlasm_ctx)
    , parser(parser)
{}

void common_statement_provider::preprocess_deferred(
    statement_processor& processor, context::cached_statement_storage& cache)
{
    const auto& def_stmt = *cache.get_base()->access_deferred();

    auto status = processor.get_processing_status(def_stmt.instruction_ref());

    if (status.first.form == processing_form::DEFERRED)
    {
        processor.process_statement(cache.get_base());
    }
    else if (!cache.contains(status.first.form))
    {
        context::cached_statement_storage::cache_entry_t ptr;
        auto def_impl = std::dynamic_pointer_cast<const semantics::statement_si_deferred>(cache.get_base());

        if (status.first.occurence == operand_occurence::ABSENT || status.first.form == processing_form::UNKNOWN
            || status.first.form == processing_form::IGNORED)
        {
            semantics::operands_si op(def_stmt.deferred_range_ref(), semantics::operand_list());
            semantics::remarks_si rem(def_stmt.deferred_range_ref(), {});

            ptr = std::make_shared<semantics::statement_si_defer_done>(def_impl, std::move(op), std::move(rem));
        }
        else
        {
            auto [op, rem] = parser.parse_operand_field(&hlasm_ctx,
                def_stmt.deferred_ref(),
                false,
                semantics::range_provider(def_stmt.deferred_range_ref(), semantics::adjusting_state::NONE),
                status);

            ptr = std::make_shared<semantics::statement_si_defer_done>(def_impl, std::move(op), std::move(rem));
        }

        cache.insert(status.first.form, ptr);
        context::unique_stmt_ptr resolved = std::make_unique<resolved_statement_impl>(ptr, status.second, status.first);
        processor.process_statement(std::move(resolved));
    }
    else
    {
        auto ptr = cache.get(status.first.form);
        context::unique_stmt_ptr resolved = std::make_unique<resolved_statement_impl>(ptr, status.second, status.first);
        processor.process_statement(std::move(resolved));
    }
}
