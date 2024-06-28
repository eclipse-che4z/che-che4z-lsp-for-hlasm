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

#include "library_info_transitional.h"

namespace hlasm_plugin::parser_library::processing {

members_statement_provider::members_statement_provider(const statement_provider_kind kind,
    const analyzing_context& ctx,
    statement_fields_parser& parser,
    parse_lib_provider& lib_provider,
    processing::processing_state_listener& listener,
    diagnostic_op_consumer& diag_consumer)
    : statement_provider(kind)
    , m_ctx(ctx)
    , m_parser(parser)
    , m_lib_provider(lib_provider)
    , m_listener(listener)
    , m_diagnoser(diag_consumer)
{}

context::shared_stmt_ptr members_statement_provider::get_next(const statement_processor& processor)
{
    if (finished())
        throw std::runtime_error("provider already finished");

    auto [cache, resolved_instruction] = get_next();

    if (!cache)
        return nullptr;

    if (processor.kind == processing_kind::ORDINARY)
    {
        if (const auto* instr = retrieve_instruction(*cache))
        {
            if (try_trigger_attribute_lookahead(*instr,
                    { *m_ctx.hlasm_ctx, library_info_transitional(m_lib_provider), drop_diagnostic_op },
                    m_listener,
                    std::move(lookahead_references)))
                return nullptr;
        }
    }

    context::shared_stmt_ptr stmt;

    switch (cache->get_base()->kind)
    {
        case context::statement_kind::RESOLVED:
            stmt = cache->get_base();
            break;
        case context::statement_kind::DEFERRED: {
            stmt = cache->get_base();
            const auto& current_instr = stmt->access_deferred()->instruction;
            if (!resolved_instruction.has_value())
                resolved_instruction.emplace(processor.resolve_instruction(current_instr));
            auto proc_status_o = processor.get_processing_status(*resolved_instruction, current_instr.field_range);
            if (!proc_status_o.has_value())
            {
                go_back(std::move(*resolved_instruction));
                return nullptr;
            }
            if (proc_status_o->first.form != processing_form::DEFERRED)
                stmt = preprocess_deferred(processor, *cache, *proc_status_o, std::move(stmt));
            break;
        }
        case context::statement_kind::ERROR:
            stmt = cache->get_base();
            break;
        default:
            break;
    }

    if (processor.kind == processing_kind::ORDINARY
        && try_trigger_attribute_lookahead(*stmt,
            { *m_ctx.hlasm_ctx, library_info_transitional(m_lib_provider), drop_diagnostic_op },
            m_listener,
            std::move(lookahead_references)))
        return nullptr;

    return stmt;
}

const semantics::instruction_si* members_statement_provider::retrieve_instruction(
    const context::statement_cache& cache) const
{
    switch (cache.get_base()->kind)
    {
        case context::statement_kind::RESOLVED:
            return &cache.get_base()->access_resolved()->instruction_ref();
        case context::statement_kind::DEFERRED:
            return &cache.get_base()->access_deferred()->instruction;
        case context::statement_kind::ERROR:
            return nullptr;
        default:
            return nullptr;
    }
}

// structure holding deferred statement that is now complete
struct statement_si_defer_done final
{
    statement_si_defer_done(std::shared_ptr<const semantics::deferred_statement> deferred_stmt,
        semantics::operands_si operands,
        semantics::remarks_si remarks,
        std::vector<semantics::literal_si> collected_literals)
        : deferred_stmt(std::move(deferred_stmt))
        , operands(std::move(operands))
        , remarks(std::move(remarks))
        , collected_literals(std::move(collected_literals))
    {}

    std::shared_ptr<const semantics::deferred_statement> deferred_stmt;

    semantics::operands_si operands;
    semantics::remarks_si remarks;
    std::vector<semantics::literal_si> collected_literals;
};

const context::statement_cache::cached_statement_t& members_statement_provider::fill_cache(
    context::statement_cache& cache,
    std::shared_ptr<const semantics::deferred_statement> def_stmt,
    const processing_status& status)
{
    context::statement_cache::cached_statement_t reparsed_stmt { {}, filter_cached_diagnostics(*def_stmt) };
    const auto& def_ops = def_stmt->deferred_operands;

    if (status.first.occurrence == operand_occurrence::ABSENT || status.first.form == processing_form::UNKNOWN
        || status.first.form == processing_form::IGNORED)
    {
        semantics::operands_si op(def_ops.field_range, semantics::operand_list());
        semantics::remarks_si rem(def_ops.field_range, {});

        reparsed_stmt.stmt = std::make_shared<statement_si_defer_done>(
            std::move(def_stmt), std::move(op), std::move(rem), std::vector<semantics::literal_si>());
    }
    else
    {
        diagnostic_consumer_transform diag_consumer(
            [&reparsed_stmt](diagnostic_op diag) { reparsed_stmt.diags.push_back(std::move(diag)); });
        auto [op, rem, lits] = m_parser.parse_operand_field(def_ops.value,
            false,
            semantics::range_provider(def_ops.field_range, semantics::adjusting_state::NONE),
            def_ops.logical_column,
            status,
            diag_consumer);

        reparsed_stmt.stmt = std::make_shared<statement_si_defer_done>(
            std::move(def_stmt), std::move(op), std::move(rem), std::move(lits));
    }
    return cache.insert(processing_status_cache_key(status), std::move(reparsed_stmt));
}

struct deferred_statement_adapter final : public resolved_statement
{
    deferred_statement_adapter(std::shared_ptr<const statement_si_defer_done> base_stmt, processing_status status)
        : base_stmt(std::move(base_stmt))
        , status(std::move(status))
    {}

    std::shared_ptr<const statement_si_defer_done> base_stmt;
    processing_status status;

    const range& stmt_range_ref() const override { return base_stmt->deferred_stmt->stmt_range; }
    const semantics::label_si& label_ref() const override { return base_stmt->deferred_stmt->label; }
    const semantics::instruction_si& instruction_ref() const override { return base_stmt->deferred_stmt->instruction; }
    const semantics::operands_si& operands_ref() const override { return base_stmt->operands; }
    const semantics::remarks_si& remarks_ref() const override { return base_stmt->remarks; }
    std::span<const semantics::literal_si> literals() const override { return base_stmt->collected_literals; }
    const op_code& opcode_ref() const override { return status.second; }
    processing_format format_ref() const override { return status.first; }
    std::span<const diagnostic_op> diagnostics() const override { return {}; }
};


context::shared_stmt_ptr members_statement_provider::preprocess_deferred(const statement_processor& processor,
    context::statement_cache& cache,
    processing_status status,
    context::shared_stmt_ptr base_stmt)
{
    const auto& def_stmt = *base_stmt->access_deferred();

    processing_status_cache_key key(status);

    const auto* cache_item = cache.get(key);
    if (!cache_item)
        cache_item = &fill_cache(cache, { std::move(base_stmt), &def_stmt }, status);

    if (processor.kind != processing_kind::LOOKAHEAD)
    {
        for (const diagnostic_op& diag : cache_item->diags)
            m_diagnoser.add_diagnostic(diag);
    }

    return std::make_shared<deferred_statement_adapter>(cache_item->stmt, status);
}

} // namespace hlasm_plugin::parser_library::processing
