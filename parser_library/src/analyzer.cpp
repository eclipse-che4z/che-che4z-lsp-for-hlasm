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


analyzing_context& analyzer_options::get_context()
{
    if (std::holds_alternative<asm_option>(ctx_source))
    {
        ctx_source = analyzing_context {
            std::make_unique<context::hlasm_context>(file_name, std::move(std::get<asm_option>(ctx_source))),
            std::make_unique<lsp::lsp_context>(),
        };
    }
    return std::get<analyzing_context>(ctx_source);
}

context::hlasm_context& analyzer_options::get_hlasm_context() { return *get_context().hlasm_ctx; }

workspaces::parse_lib_provider& analyzer_options::get_lib_provider()
{
    if (lib_provider)
        return *lib_provider;
    else
        return workspaces::empty_parse_lib_provider::instance;
}

analyzer::analyzer(const std::string& text, analyzer_options opts)
    : diagnosable_ctx(opts.get_hlasm_context())
    , ctx_(std::move(opts.get_context()))
    , listener_(opts.file_name)
    , src_proc_(opts.collect_hl_info == collect_highlighting_info::yes)
    , field_parser_(ctx_.hlasm_ctx.get())
    , mngr_(std::make_unique<processing::opencode_provider>(text,
                ctx_,
                opts.get_lib_provider(),
                mngr_,
                src_proc_,
                listener_,
                opts.parsing_opencode == file_is_opencode::yes ? opencode_provider_options { true, 10 }
                                                               : opencode_provider_options {}),
          ctx_,
          opts.library_data,
          opts.file_name,
          text,
          opts.get_lib_provider(),
          field_parser_)
{}

analyzing_context analyzer::context() { return ctx_; }

context::hlasm_context& analyzer::hlasm_ctx() { return *ctx_.hlasm_ctx; }

parsing::hlasmparser& analyzer::parser() { return mngr_.opencode_parser(); }
bool analyzer::feed_line() { return mngr_.feed_opencode_line() != extract_next_logical_line_result::failed; }

const semantics::source_info_processor& analyzer::source_processor() const { return src_proc_; }

void analyzer::analyze(std::atomic<bool>* cancel)
{
    mngr_.start_processing(cancel);
    src_proc_.finish();
}

void analyzer::collect_diags() const
{
    collect_diags_from_child(mngr_);
    collect_diags_from_child(listener_);
    collect_diags_from_child(field_parser_);
}

const performance_metrics& analyzer::get_metrics() const
{
    ctx_.hlasm_ctx->fill_metrics_files();
    return ctx_.hlasm_ctx->metrics;
}

void analyzer::register_stmt_analyzer(statement_analyzer* stmt_analyzer)
{
    mngr_.register_stmt_analyzer(stmt_analyzer);
}
