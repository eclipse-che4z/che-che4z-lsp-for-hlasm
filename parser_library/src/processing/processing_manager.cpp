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

#include "processing_manager.h"

#include <assert.h>

#include "parsing/parser_impl.h"
#include "statement_processors/copy_processor.h"
#include "statement_processors/empty_processor.h"
#include "statement_processors/lookahead_processor.h"
#include "statement_processors/macrodef_processor.h"
#include "statement_processors/ordinary_processor.h"
#include "statement_providers/copy_statement_provider.h"
#include "statement_providers/macro_statement_provider.h"

namespace hlasm_plugin::parser_library::processing {

processing_manager::processing_manager(std::unique_ptr<opencode_provider> base_provider,
    context::hlasm_context& hlasm_ctx,
    const workspaces::library_data data,
    std::string file_name,
    workspaces::parse_lib_provider& lib_provider,
    statement_fields_parser& parser,
    processing_tracer* tracer)
    : diagnosable_ctx(hlasm_ctx)
    , hlasm_ctx_(hlasm_ctx)
    , lib_provider_(lib_provider)
    , opencode_prov_(*base_provider)
    , tracer_(tracer)

{
    switch (data.proc_kind)
    {
        case processing_kind::ORDINARY:
            provs_.emplace_back(std::make_unique<macro_statement_provider>(hlasm_ctx, parser, lib_provider, *this));
            procs_.emplace_back(
                std::make_unique<ordinary_processor>(hlasm_ctx, *this, lib_provider, *this, parser, tracer_));
            break;
        case processing_kind::COPY:
            hlasm_ctx.push_statement_processing(processing_kind::COPY, std::move(file_name));
            procs_.emplace_back(
                std::make_unique<copy_processor>(hlasm_ctx, *this, copy_start_data { data.library_member }));
            break;
        case processing_kind::MACRO:
            hlasm_ctx.push_statement_processing(processing_kind::MACRO, std::move(file_name));
            procs_.emplace_back(std::make_unique<macrodef_processor>(
                hlasm_ctx, *this, lib_provider, macrodef_start_data(data.library_member)));
            break;
        default:
            break;
    }

    provs_.emplace_back(std::make_unique<copy_statement_provider>(hlasm_ctx, parser, lib_provider, *this));
    provs_.emplace_back(std::move(base_provider));
}

void update_metrics(processing_kind proc_kind, statement_provider_kind prov_kind, performance_metrics& metrics)
{
    switch (proc_kind)
    {
        case processing_kind::ORDINARY:
            switch (prov_kind)
            {
                case statement_provider_kind::COPY:
                    metrics.copy_statements++;
                    break;
                case statement_provider_kind::OPEN:
                    metrics.open_code_statements++;
                    break;
                case statement_provider_kind::MACRO:
                    metrics.macro_statements++;
                    break;
            }
            break;
        case processing_kind::LOOKAHEAD:
            metrics.lookahead_statements++;
            break;
        case processing_kind::COPY:
            metrics.copy_def_statements++;
            break;
        case processing_kind::MACRO:
            metrics.macro_def_statements++;
            break;
    }
}

void processing_manager::start_processing(std::atomic<bool>* cancel)
{
    while (!procs_.empty())
    {
        if (cancel && *cancel)
            break;

        statement_processor& proc = *procs_.back();
        statement_provider& prov = find_provider();

        if ((prov.finished() && proc.terminal_condition(prov.kind)) || proc.finished())
        {
            finish_processor();
            continue;
        }

        update_metrics(proc.kind, prov.kind, hlasm_ctx_.metrics);
        prov.process_next(proc);
    }
}

bool processing_manager::attr_lookahead_active() const
{
    auto look_pr = dynamic_cast<lookahead_processor*>(&*procs_.back());
    return look_pr && look_pr->action == lookahead_action::ORD;
}

statement_provider& processing_manager::find_provider()
{
    if (attr_lookahead_active())
    {
        auto& opencode_prov = **(provs_.end() - 1);
        auto& copy_prov = **(provs_.end() - 2);
        auto& prov = !copy_prov.finished() ? copy_prov : opencode_prov;
        return prov;
    }

    for (auto& prov : provs_)
    {
        if (!prov->finished())
            return *prov;
    }

    return *provs_.back();
}

void processing_manager::finish_processor()
{
    procs_.back()->end_processing();
    collect_diags_from_child(*procs_.back());
    procs_.pop_back();
}

void processing_manager::start_macro_definition(const macrodef_start_data start)
{
    hlasm_ctx_.push_statement_processing(processing_kind::MACRO);
    procs_.emplace_back(std::make_unique<macrodef_processor>(hlasm_ctx_, *this, lib_provider_, start));
}

void processing_manager::finish_macro_definition(macrodef_processing_result result)
{
    if (!result.invalid)
        hlasm_ctx_.add_macro(result.prototype.macro_name,
            result.prototype.name_param,
            std::move(result.prototype.symbolic_params),
            std::move(result.definition),
            std::move(result.nests),
            std::move(result.sequence_symbols),
            std::move(result.definition_location));
}

void processing_manager::start_lookahead(lookahead_start_data start)
{
    // jump to the statement where the previous lookahead stopped
    if (hlasm_ctx_.current_source().end_index < lookahead_stop_.end_index)
        perform_opencode_jump(
            context::source_position(lookahead_stop_.end_line + 1, lookahead_stop_.end_index), lookahead_stop_);

    hlasm_ctx_.push_statement_processing(processing_kind::LOOKAHEAD);
    procs_.emplace_back(
        std::make_unique<lookahead_processor>(hlasm_ctx_, *this, *this, lib_provider_, std::move(start)));
}

void processing_manager::finish_lookahead(lookahead_processing_result result)
{
    lookahead_stop_ = hlasm_ctx_.current_source().create_snapshot();

    if (result.action == lookahead_action::SEQ)
    {
        if (result.success)
            jump_in_statements(result.symbol_name, result.symbol_range);
        else
        {
            perform_opencode_jump(result.statement_position, std::move(result.snapshot));

            add_diagnostic(diagnostic_op::error_E047(*result.symbol_name, result.symbol_range));
        }
    }
    else
    {
        if (hlasm_ctx_.current_scope().this_macro)
            --hlasm_ctx_.current_scope().this_macro->current_statement;

        perform_opencode_jump(result.statement_position, std::move(result.snapshot));
    }
}

void processing_manager::start_copy_member(copy_start_data start)
{
    procs_.emplace_back(std::make_unique<copy_processor>(hlasm_ctx_, *this, std::move(start)));
}

void processing_manager::finish_copy_member(copy_processing_result result)
{
    hlasm_ctx_.add_copy_member(result.member_name,
        result.invalid_member ? context::statement_block() : std::move(result.definition),
        std::move(result.definition_location));
}

void processing_manager::jump_in_statements(context::id_index target, range symbol_range)
{
    auto symbol = hlasm_ctx_.get_sequence_symbol(target);
    if (!symbol)
    {
        if (hlasm_ctx_.is_in_macro())
        {
            add_diagnostic(diagnostic_op::error_E047(*target, symbol_range));
        }
        else
        {
            auto&& [statement_position, snapshot] = hlasm_ctx_.get_end_snapshot();
            start_lookahead(
                lookahead_start_data(target, symbol_range, std::move(statement_position), std::move(snapshot)));
        }
    }
    else
    {
        if (symbol->kind == context::sequence_symbol_kind::MACRO)
        {
            assert(hlasm_ctx_.is_in_macro());
            hlasm_ctx_.scope_stack().back().this_macro->current_statement =
                (int)symbol->access_macro_symbol()->statement_offset - 1;
        }
        else
        {
            auto opencode_symbol = symbol->access_opencode_symbol();

            perform_opencode_jump(opencode_symbol->statement_position, opencode_symbol->snapshot);
        }

        hlasm_ctx_.decrement_branch_counter();
    }
}

void processing_manager::register_sequence_symbol(context::id_index target, range symbol_range)
{
    if (!attr_lookahead_active() && hlasm_ctx_.is_in_macro())
        return;

    auto symbol = hlasm_ctx_.get_opencode_sequence_symbol(target);
    auto new_symbol = create_opencode_sequence_symbol(target, symbol_range);

    if (!symbol)
    {
        hlasm_ctx_.add_sequence_symbol(std::move(new_symbol));
    }
    else if (!(*symbol->access_opencode_symbol() == *new_symbol))
    {
        add_diagnostic(diagnostic_op::error_E045(*target, symbol_range));
    }
}

std::unique_ptr<context::opencode_sequence_symbol> processing_manager::create_opencode_sequence_symbol(
    context::id_index name, range symbol_range)
{
    auto symbol_pos = symbol_range.start;
    location loc(symbol_pos, hlasm_ctx_.processing_stack().back().proc_location.file);

    auto&& [statement_position, snapshot] = hlasm_ctx_.get_begin_snapshot(attr_lookahead_active());

    return std::make_unique<context::opencode_sequence_symbol>(name, loc, statement_position, std::move(snapshot));
}

void processing_manager::perform_opencode_jump(
    context::source_position statement_position, context::source_snapshot snapshot)
{
    opencode_prov_.rewind_input(statement_position);

    hlasm_ctx_.apply_source_snapshot(std::move(snapshot));
}

void processing_manager::collect_diags() const
{
    for (auto& proc : procs_)
        collect_diags_from_child(*proc);

    collect_diags_from_child(dynamic_cast<parsing::parser_impl&>(*provs_.back()));
}

} // namespace hlasm_plugin::parser_library::processing
