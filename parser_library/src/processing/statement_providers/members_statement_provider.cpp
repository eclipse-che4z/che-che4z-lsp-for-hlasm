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
    analyzing_context ctx,
    statement_fields_parser& parser,
    workspaces::parse_lib_provider& lib_provider,
    processing::processing_state_listener& listener,
    diagnostic_op_consumer& diag_consumer)
    : statement_provider(kind)
    , ctx(std::move(ctx))
    , parser(parser)
    , lib_provider(lib_provider)
    , listener(listener)
    , diagnoser(diag_consumer)
{}

context::shared_stmt_ptr members_statement_provider::get_next(const statement_processor& processor)
{
    if (finished())
        throw std::runtime_error("provider already finished");

    auto cache = get_next();

    if (!cache)
        return nullptr;

    if (processor.kind == processing_kind::ORDINARY
        && try_trigger_attribute_lookahead(retrieve_instruction(*cache), { *ctx.hlasm_ctx, lib_provider }, listener))
        return nullptr;

    context::shared_stmt_ptr stmt;

    switch (cache->get_base()->kind)
    {
        case context::statement_kind::RESOLVED:
            stmt = cache->get_base();
            break;
        case context::statement_kind::DEFERRED:
            stmt = preprocess_deferred(processor, *cache);
            break;
        default:
            assert(false);
            break;
    }


    if (processor.kind == processing_kind::ORDINARY
        && try_trigger_attribute_lookahead(*stmt, { *ctx.hlasm_ctx, lib_provider }, listener))
        return nullptr;

    return stmt;
}

const semantics::instruction_si& members_statement_provider::retrieve_instruction(
    const context::statement_cache& cache) const
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

void members_statement_provider::fill_cache(
    context::statement_cache& cache, const semantics::deferred_statement& def_stmt, const processing_status& status)
{
    context::statement_cache::cached_statement_t reparsed_stmt;
    auto def_impl = std::dynamic_pointer_cast<const semantics::statement_si_deferred>(cache.get_base());

    if (status.first.occurence == operand_occurence::ABSENT || status.first.form == processing_form::UNKNOWN
        || status.first.form == processing_form::IGNORED)
    {
        semantics::operands_si op(def_stmt.deferred_ref().field_range, semantics::operand_list());
        semantics::remarks_si rem(def_stmt.deferred_ref().field_range, {});

        reparsed_stmt.stmt =
            std::make_shared<semantics::statement_si_defer_done>(def_impl, std::move(op), std::move(rem));
    }
    else
    {
        diagnostic_consumer_transform diag_consumer(
            [&reparsed_stmt](diagnostic_op diag) { reparsed_stmt.diags.push_back(std::move(diag)); });
        auto [op, rem] = parser.parse_operand_field(def_stmt.deferred_ref().value,
            false,
            semantics::range_provider(def_stmt.deferred_ref().field_range, semantics::adjusting_state::NONE),
            status,
            diag_consumer);

        reparsed_stmt.stmt =
            std::make_shared<semantics::statement_si_defer_done>(def_impl, std::move(op), std::move(rem));
    }
    cache.insert(status.first.form, reparsed_stmt);
}

context::shared_stmt_ptr members_statement_provider::preprocess_deferred(
    const statement_processor& processor, context::statement_cache& cache)
{
    const auto& def_stmt = *cache.get_base()->access_deferred();

    auto status = processor.get_processing_status(def_stmt.instruction_ref());

    if (status.first.form == processing_form::DEFERRED)
        return cache.get_base();

    if (!cache.contains(status.first.form))
        fill_cache(cache, def_stmt, status);

    const auto& cache_item = cache.get(status.first.form);

    for (const diagnostic_op& diag : cache_item->diags)
        diagnoser.add_diagnostic(diag);

    return std::make_shared<resolved_statement_impl>(cache_item->stmt, status);
}

} // namespace hlasm_plugin::parser_library::processing
