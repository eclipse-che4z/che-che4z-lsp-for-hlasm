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

#include "analyzer.h"

#include "parsing/error_strategy.h"

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::lexing;
using namespace hlasm_plugin::parser_library::parsing;
using namespace hlasm_plugin::parser_library::workspaces;

analyzer::analyzer(const std::string& text,
    std::string file_name,
    analyzing_context ctx,
    parse_lib_provider& lib_provider,
    const library_data data,
    bool collect_hl_info)
    : diagnosable_ctx(*ctx.hlasm_ctx)
    , ctx_(ctx)
    , listener_(file_name)
    , src_proc_(collect_hl_info)
    , input_(text)
    , lexer_(&input_, &src_proc_, &ctx.hlasm_ctx->metrics)
    , tokens_(&lexer_)
    , parser_(new parsing::hlasmparser(&tokens_))
    , mngr_(std::unique_ptr<processing::opencode_provider>(parser_), ctx, data, file_name, text, lib_provider, *parser_)
{
    parser_->initialize(ctx, &src_proc_, &lib_provider, &mngr_);
    parser_->setErrorHandler(std::make_shared<error_strategy>());
    parser_->removeErrorListeners();
    parser_->addErrorListener(&listener_);
}

analyzer::analyzer(
    const std::string& text, std::string file_name, parse_lib_provider& lib_provider, bool collect_hl_info)
    : analyzer(text,
        file_name,
        analyzing_context {
            std::make_unique<context::hlasm_context>(file_name, lib_provider.get_asm_options(file_name)),
            std::make_unique<lsp::lsp_context>() },
        lib_provider,
        library_data { processing::processing_kind::ORDINARY, context::id_storage::empty_id },
        collect_hl_info)
{}

analyzing_context analyzer::context() { return ctx_; }

context::hlasm_context& analyzer::hlasm_ctx() { return *ctx_.hlasm_ctx; }

parsing::hlasmparser& analyzer::parser() { return *parser_; }

const semantics::source_info_processor& analyzer::source_processor() { return src_proc_; }

void analyzer::analyze(std::atomic<bool>* cancel)
{
    mngr_.start_processing(cancel);
    src_proc_.finish();
}

void analyzer::collect_diags() const
{
    collect_diags_from_child(mngr_);
    collect_diags_from_child(listener_);
}

const performance_metrics& analyzer::get_metrics()
{
    ctx_.hlasm_ctx->fill_metrics_files();
    return ctx_.hlasm_ctx->metrics;
}

void analyzer::register_stmt_analyzer(statement_analyzer* stmt_analyzer)
{
    mngr_.register_stmt_analyzer(stmt_analyzer);
}
