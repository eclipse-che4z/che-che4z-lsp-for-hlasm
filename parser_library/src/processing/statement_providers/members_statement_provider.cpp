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

#include "members_statement_provider.h"

namespace hlasm_plugin::parser_library::processing {

members_statement_provider::members_statement_provider(const statement_provider_kind kind,
    context::hlasm_context& hlasm_ctx,
    statement_fields_parser& parser,
    workspaces::parse_lib_provider& lib_provider,
    processing::processing_state_listener& listener)
    : statement_provider(kind)
    , hlasm_ctx(hlasm_ctx)
    , parser(parser)
    , lib_provider(lib_provider)
    , listener(listener)
{}

void members_statement_provider::process_next(statement_processor& processor)
{
    if (finished())
        throw std::runtime_error("provider already finished");

    auto cache = get_next();

    if (!cache)
        return;

    if (processor.kind == processing_kind::ORDINARY
        && try_trigger_attribute_lookahead(retrieve_instruction(*cache), { hlasm_ctx, lib_provider }, listener))
        return;

    context::shared_stmt_ptr statement;

    switch (cache->get_base()->kind)
    {
        case context::statement_kind::RESOLVED:
            statement = cache->get_base();
            break;
        case context::statement_kind::DEFERRED:
            statement = preprocess_deferred(processor, *cache);
            break;
        default:
            assert(false);
            break;
    }

    if (processor.kind == processing_kind::ORDINARY
        && try_trigger_attribute_lookahead(*statement, { hlasm_ctx, lib_provider }, listener))
        return;

    processor.process_statement(statement);
}

context::shared_stmt_ptr members_statement_provider::preprocess_deferred(
    statement_processor& processor, context::cached_statement_storage& cache)
{
    const auto& def_stmt = *cache.get_base()->access_deferred();

    auto status = processor.get_processing_status(def_stmt.instruction_ref());

    if (status.first.form == processing_form::DEFERRED)
    {
        return cache.get_base();
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
        return std::make_shared<resolved_statement_impl>(ptr, status.second, status.first);
    }
    else
    {
        auto ptr = cache.get(status.first.form);
        return std::make_shared<resolved_statement_impl>(ptr, status.second, status.first);
    }
}

const semantics::instruction_si& members_statement_provider::retrieve_instruction(
    context::cached_statement_storage& cache)
{
    switch (cache.get_base()->kind)
    {
        case context::statement_kind::RESOLVED:
            return cache.get_base()->access_resolved()->instruction_ref();
        case context::statement_kind::DEFERRED:
            return cache.get_base()->access_deferred()->instruction_ref();
        default:
            assert(false);
            throw std::runtime_error("unexpected statement_kind enum value");
    }
}

} // namespace hlasm_plugin::parser_library::processing
