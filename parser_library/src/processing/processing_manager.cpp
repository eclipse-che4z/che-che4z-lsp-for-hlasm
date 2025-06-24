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

#include <cassert>
#include <memory>

#include "context/hlasm_context.h"
#include "diagnostic_tools.h"
#include "lsp/lsp_context.h"
#include "lsp/text_data_view.h"
#include "parsing/parser_impl.h"
#include "statement_analyzers/lsp_analyzer.h"
#include "statement_processors/copy_processor.h"
#include "statement_processors/lookahead_processor.h"
#include "statement_processors/macrodef_processor.h"
#include "statement_processors/ordinary_processor.h"
#include "statement_providers/copy_statement_provider.h"
#include "statement_providers/macro_statement_provider.h"
#include "utils/task.h"

namespace hlasm_plugin::parser_library::processing {

processing_manager::processing_manager(std::unique_ptr<opencode_provider> base_provider,
    const analyzing_context& ctx,
    processing::processing_kind proc_kind,
    std::string dep_name,
    utils::resource::resource_location file_loc,
    std::string_view file_text,
    parse_lib_provider& lib_provider,
    statement_fields_parser& parser,
    std::shared_ptr<std::vector<fade_message>> fade_msgs,
    output_handler* output,
    diagnosable_ctx& diag_ctx)
    : ctx_(ctx)
    , hlasm_ctx_(*ctx_.hlasm_ctx)
    , lib_provider_(lib_provider)
    , opencode_prov_(*base_provider)
    , diag_ctx(diag_ctx)
    , lsp_analyzer_(*ctx_.hlasm_ctx, *ctx_.lsp_ctx, file_text)
    , stms_analyzers_({ &lsp_analyzer_ })
    , file_loc_(file_loc)
    , m_fade_msgs(std::move(fade_msgs))
{
    switch (proc_kind)
    {
        case processing_kind::ORDINARY:
            provs_.emplace_back(
                std::make_unique<macro_statement_provider>(ctx_, parser, lib_provider, *this, diag_ctx));
            procs_.emplace_back(std::make_unique<ordinary_processor>(
                ctx_, *this, lib_provider, *this, parser, opencode_prov_, *this, output, diag_ctx));
            break;
        case processing_kind::COPY:
            start_copy_member(copy_start_data { ctx.hlasm_ctx->add_id(std::move(dep_name)), std::move(file_loc) });
            break;
        case processing_kind::MACRO:
            start_macro_definition(
                macrodef_start_data(ctx.hlasm_ctx->add_id(std::move(dep_name))), std::move(file_loc));
            break;
        default:
            break;
    }

    provs_.emplace_back(std::make_unique<copy_statement_provider>(ctx_, parser, lib_provider, *this, diag_ctx));
    provs_.emplace_back(std::move(base_provider));

    opencode_prov_.onetime_action();
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

utils::task processing_manager::co_step()
{
    while (!procs_.empty())
    {
        if (helper_task_.valid())
            co_await std::exchange(helper_task_, {});

        statement_processor& proc = *procs_.back();
        statement_provider& prov = find_provider();

        if ((prov.finished() && proc.terminal_condition(prov.kind)) || proc.finished())
        {
            finish_processor();
            continue;
        }

        if (auto stmt = prov.get_next(proc))
        {
            update_metrics(proc.kind, prov.kind, hlasm_ctx_.metrics);
            for (auto& a : stms_analyzers_)
                if (a->analyze(*stmt, prov.kind, proc.kind, false))
                    co_await utils::task::suspend();

            proc.process_statement(std::move(stmt));
        }

        co_await utils::task::yield();
    }
}

void processing_manager::register_stmt_analyzer(statement_analyzer* stmt_analyzer)
{
    stms_analyzers_.push_back(stmt_analyzer);
}

void processing_manager::aread_cb(size_t line, std::string_view text) const
{
    for (auto& a : stms_analyzers_)
        a->analyze_aread_line(file_loc_, line, text);
}

void processing_manager::run_analyzers(const context::hlasm_statement& statement, bool evaluated_model) const
{
    run_analyzers(statement, find_provider().kind, procs_.back()->kind, evaluated_model);
}

void processing_manager::run_analyzers(const context::hlasm_statement& statement,
    statement_provider_kind prov_kind,
    processing_kind proc_kind,
    bool evaluated_model) const
{
    for (auto& a : stms_analyzers_)
        a->analyze(statement, prov_kind, proc_kind, evaluated_model);
}

bool processing_manager::attr_lookahead_active() const
{
    return procs_.back()->kind == processing_kind::LOOKAHEAD
        && static_cast<lookahead_processor*>(std::to_address(procs_.back()))->action == lookahead_action::ORD;
}

bool processing_manager::seq_lookahead_active() const
{
    return procs_.back()->kind == processing_kind::LOOKAHEAD
        && static_cast<lookahead_processor*>(std::to_address(procs_.back()))->action == lookahead_action::SEQ;
}

bool processing_manager::lookahead_active() const { return procs_.back()->kind == processing_kind::LOOKAHEAD; }

statement_provider& processing_manager::find_provider() const
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
    procs_.pop_back();
}

void processing_manager::finish_preprocessor()
{
    auto preproc = opencode_prov_.get_preprocessor();
    if (!preproc)
        return;

    for (const auto& stmt : preproc->take_statements())
    {
        if (!stmt)
            continue;

        if (m_fade_msgs && stmt->m_details.instruction.preproc_specific_r)
            m_fade_msgs->emplace_back(fade_message::preprocessor_statement(
                file_loc_.get_uri(), *stmt->m_details.instruction.preproc_specific_r));

        lsp_analyzer_.analyze(*stmt);
    }

    for (const auto& inc_member_details : preproc->view_included_members())
    {
        assert(inc_member_details);

        static const context::statement_block stmt_block;

        ctx_.lsp_ctx->add_copy(std::make_shared<context::copy_member>(hlasm_ctx_.add_id(inc_member_details->name),
                                   stmt_block,
                                   location(position(0, 0), inc_member_details->loc)),
            lsp::text_data_view(inc_member_details->text));
    }
}

void processing_manager::start_macro_definition(macrodef_start_data start)
{
    start_macro_definition(std::move(start), std::nullopt);
}

void processing_manager::finish_macro_definition(macrodef_processing_result result)
{
    context::macro_def_ptr mac;
    if (!result.invalid)
        mac = hlasm_ctx_.add_macro(result.prototype.macro_name,
            result.prototype.name_param,
            std::move(result.prototype.symbolic_params),
            std::move(result.definition),
            std::move(result.nests),
            std::move(result.sequence_symbols),
            std::move(result.definition_location),
            std::move(result.used_copy_members),
            result.external);

    lsp_analyzer_.macrodef_finished(std::move(mac), std::move(result));
}

void processing_manager::start_lookahead(lookahead_start_data start)
{
    // jump to the statement where the previous lookahead stopped
    if (hlasm_ctx_.current_source().end_index < lookahead_stop_.begin_index
        && (!hlasm_ctx_.in_opencode() || hlasm_ctx_.current_ainsert_id() <= lookahead_stop_ainsert_id))
        perform_opencode_jump(context::source_position(lookahead_stop_.end_index), lookahead_stop_);

    hlasm_ctx_.push_statement_processing(processing_kind::LOOKAHEAD);
    procs_.emplace_back(
        std::make_unique<lookahead_processor>(ctx_, *this, *this, lib_provider_, std::move(start), diag_ctx));
}

void processing_manager::finish_lookahead(lookahead_processing_result result)
{
    for (auto it : m_pending_seq_redifinitions)
    {
        auto& [state, diags] = it->second;
        if (state == pending_seq_redifinition_state::lookahead_pending)
            state = diags.empty() ? pending_seq_redifinition_state::lookahead_done
                                  : pending_seq_redifinition_state::diagnostics;
    }
    m_pending_seq_redifinitions.clear();

    lookahead_stop_ = hlasm_ctx_.current_source().create_snapshot();
    lookahead_stop_ainsert_id = hlasm_ctx_.current_ainsert_id();

    if (result.action == lookahead_action::SEQ)
    {
        if (result.success)
            jump_in_statements(result.symbol_name, result.symbol_range);
        else
        {
            perform_opencode_jump(result.statement_position, std::move(result.snapshot));

            diag_ctx.add_diagnostic(
                diagnostic_op::error_E047(result.symbol_name.to_string_view(), result.symbol_range));
        }
    }
    else
    {
        if (hlasm_ctx_.current_scope().this_macro)
            --hlasm_ctx_.current_scope().this_macro->current_statement.value;

        perform_opencode_jump(result.statement_position, std::move(result.snapshot));
    }
}

void processing_manager::start_copy_member(copy_start_data start)
{
    hlasm_ctx_.push_statement_processing(processing_kind::COPY, std::move(start.member_loc));
    procs_.emplace_back(std::make_unique<copy_processor>(ctx_, *this, std::move(start), diag_ctx));
}

void processing_manager::finish_copy_member(copy_processing_result result)
{
    auto member = hlasm_ctx_.add_copy_member(result.member_name,
        result.invalid_member ? context::statement_block() : std::move(result.definition),
        std::move(result.definition_location));

    lsp_analyzer_.copydef_finished(std::move(member), std::move(result));
}

void processing_manager::finish_opencode()
{
    finish_preprocessor();
    lsp_analyzer_.opencode_finished(lib_provider_);
}

std::optional<bool> processing_manager::request_external_processing(
    context::id_index name, processing_kind proc_kind, std::function<void(bool)> callback)
{
    const auto key = std::pair(name, proc_kind);
    if (auto it = m_external_requests.find(key); it != m_external_requests.end())
    {
        if (callback)
            callback(it->second);
        return it->second;
    }

    auto next_task = lib_provider_.parse_library(name.to_string(), ctx_, proc_kind)
                         .then([this, key, callback = std::move(callback)](bool result) {
                             m_external_requests.insert_or_assign(key, result);
                             if (callback)
                                 callback(result);
                         });
    helper_task_ = helper_task_.valid() && !helper_task_.done() ? std::move(next_task).then(std::move(helper_task_))
                                                                : std::move(next_task);

    return std::nullopt;
}

void processing_manager::schedule_helper_task(utils::task t)
{
    if (!t.valid() || t.done())
        return;
    if (!helper_task_.valid() || helper_task_.done())
        helper_task_ = std::move(t);
    else
        helper_task_ = std::move(helper_task_).then(std::move(t));
}

void processing_manager::start_macro_definition(
    macrodef_start_data start, std::optional<utils::resource::resource_location> file_loc)
{
    if (file_loc)
        hlasm_ctx_.push_statement_processing(processing_kind::MACRO, std::move(*file_loc));
    else
        hlasm_ctx_.push_statement_processing(processing_kind::MACRO);

    lsp_analyzer_.macrodef_started(start);
    procs_.emplace_back(std::make_unique<macrodef_processor>(ctx_, *this, *this, std::move(start), diag_ctx));
}

void processing_manager::jump_in_statements(context::id_index target, range symbol_range)
{
    auto symbol = hlasm_ctx_.get_sequence_symbol(target);
    if (!symbol)
    {
        if (hlasm_ctx_.is_in_macro())
        {
            diag_ctx.add_diagnostic(diagnostic_op::error_E047(target.to_string_view(), symbol_range));
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
                context::statement_id { symbol->access_macro_symbol()->statement_offset.value - 1 };
        }
        else
        {
            if (auto it = m_lookahead_seq_redifinitions.find(target); it != m_lookahead_seq_redifinitions.end()
                && it->second.first != pending_seq_redifinition_state::lookahead_done)
            {
                for (auto& d : it->second.second)
                    diag_ctx.add_raw_diagnostic(std::move(d));
                it->second.second.clear();
            }
            else
            {
                auto opencode_symbol = symbol->access_opencode_symbol();

                perform_opencode_jump(opencode_symbol->statement_position, opencode_symbol->snapshot);
            }
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
        hlasm_ctx_.add_opencode_sequence_symbol(std::move(new_symbol));
        if (lookahead_active())
            m_pending_seq_redifinitions.emplace_back(m_lookahead_seq_redifinitions.try_emplace(target).first);
    }
    else if (!(*symbol->access_opencode_symbol() == *new_symbol))
    {
        if (!lookahead_active())
            diag_ctx.add_diagnostic(diagnostic_op::error_E045(target.to_string_view(), symbol_range));
        else if (auto it = m_lookahead_seq_redifinitions.find(target); it == m_lookahead_seq_redifinitions.end()
                 || it->second.first != pending_seq_redifinition_state::lookahead_pending)
        {
            // already defined either in normal processing or previous lookahead, so silently ignore
        }
        else
        {
            it->second.second.push_back(add_stack_details(
                diagnostic_op::error_E045(target.to_string_view(), symbol_range), hlasm_ctx_.processing_stack()));
        }
    }
}

std::unique_ptr<context::opencode_sequence_symbol> processing_manager::create_opencode_sequence_symbol(
    context::id_index name, range symbol_range)
{
    auto loc = hlasm_ctx_.current_statement_location(false);
    loc.pos = symbol_range.start;

    auto&& [statement_position, snapshot] = hlasm_ctx_.get_begin_snapshot(lookahead_active());

    return std::make_unique<context::opencode_sequence_symbol>(
        name, std::move(loc), statement_position, std::move(snapshot));
}

void processing_manager::perform_opencode_jump(
    context::source_position statement_position, context::source_snapshot snapshot)
{
    opencode_prov_.rewind_input(statement_position);

    hlasm_ctx_.apply_source_snapshot(std::move(snapshot));
}

parsing::parser_holder& processing_manager::opencode_parser() // for testing only
{
    if (helper_task_.valid())
        std::exchange(helper_task_, {}).run();
    return opencode_prov_.parser();
}
void processing_manager::process_postponed_statements(
    const std::vector<std::pair<std::unique_ptr<context::postponed_statement>, context::dependency_evaluation_context>>&
        stmts)
{
    lsp_analyzer_.collect_branch_info(stmts, library_info_transitional(lib_provider_));
}

} // namespace hlasm_plugin::parser_library::processing
