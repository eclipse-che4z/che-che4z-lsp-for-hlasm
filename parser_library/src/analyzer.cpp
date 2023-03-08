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

#include "hlasmparser_multiline.h"
#include "lsp/lsp_context.h"
#include "processing/opencode_provider.h"
#include "processing/preprocessor.h"
#include "utils/task.h"

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::lexing;
using namespace hlasm_plugin::parser_library::parsing;
using namespace hlasm_plugin::parser_library::workspaces;


analyzing_context& analyzer_options::get_context()
{
    if (std::holds_alternative<asm_option>(ctx_source))
    {
        auto h_ctx = std::make_shared<context::hlasm_context>(file_loc,
            std::move(std::get<asm_option>(ctx_source)),
            ids_init ? std::move(ids_init) : std::make_shared<context::id_storage>());
        ctx_source = analyzing_context {
            h_ctx,
            std::make_unique<lsp::lsp_context>(h_ctx),
        };
    }
    return std::get<analyzing_context>(ctx_source);
}

context::hlasm_context& analyzer_options::get_hlasm_context() { return *get_context().hlasm_ctx; }

workspaces::parse_lib_provider& analyzer_options::get_lib_provider() const
{
    if (lib_provider)
        return *lib_provider;
    else
        return workspaces::empty_parse_lib_provider::instance;
}

std::unique_ptr<processing::preprocessor> analyzer_options::get_preprocessor(processing::library_fetcher asm_lf,
    diagnostic_op_consumer& diag_consumer,
    semantics::source_info_processor& src_proc) const
{
    const auto transform_preprocessor = [&asm_lf, &diag_consumer, &src_proc](const preprocessor_options& po) {
        return std::visit(
            [&asm_lf, &diag_consumer, &src_proc](const auto& p) -> std::unique_ptr<processing::preprocessor> {
                return processing::preprocessor::create(p, asm_lf, &diag_consumer, src_proc);
            },
            po);
    };
    if (preprocessor_args.empty())
        return {};
    else if (preprocessor_args.size() == 1)
        return transform_preprocessor(preprocessor_args.front());

    struct combined_preprocessor final : processing::preprocessor
    {
        std::vector<std::unique_ptr<processing::preprocessor>> pp;

        utils::value_task<document> generate_replacement(document doc) override
        {
            reset();

            for (const auto& p : pp)
                doc = co_await p->generate_replacement(std::move(doc));

            co_return doc;
        }

        std::vector<std::shared_ptr<semantics::preprocessor_statement_si>> take_statements() override
        {
            for (const auto& p : pp)
                set_statements(p->take_statements());

            return preprocessor::take_statements();
        }

        const std::vector<std::unique_ptr<included_member_details>>& view_included_members() override
        {
            for (const auto& p : pp)
                capture_included_members(*p);

            return preprocessor::view_included_members();
        }
    } tmp;

    std::transform(
        preprocessor_args.begin(), preprocessor_args.end(), std::back_inserter(tmp.pp), transform_preprocessor);

    return std::make_unique<combined_preprocessor>(std::move(tmp));
}

analyzer::analyzer(std::string_view text, analyzer_options opts)
    : diagnosable_ctx(opts.get_hlasm_context())
    , ctx_(std::move(opts.get_context()))
    , src_proc_(opts.collect_hl_info == collect_highlighting_info::yes)
    , field_parser_(ctx_.hlasm_ctx.get())
    , mngr_(
          std::make_unique<processing::opencode_provider>(text,
              ctx_,
              opts.get_lib_provider(),
              mngr_,
              mngr_,
              src_proc_,
              *this,
              opts.get_preprocessor(
                  [libs = &opts.get_lib_provider()](std::string library)
                      -> utils::value_task<std::optional<std::pair<std::string, utils::resource::resource_location>>> {
                      bool called = false;
                      std::optional<std::pair<std::string, utils::resource::resource_location>> result;
                      libs->get_library(library, [&result, &called](auto v) {
                          result = std::move(v);
                          called = true;
                      });
                      if (!called)
                          co_await utils::task::suspend();
                      co_return result;
                  },
                  *this,
                  src_proc_),
              opts.parsing_opencode == file_is_opencode::yes ? processing::opencode_provider_options { true, 10 }
                                                             : processing::opencode_provider_options {},
              opts.vf_monitor,
              vf_handles_),
          ctx_,
          opts.library_data,
          opts.file_loc,
          text,
          opts.get_lib_provider(),
          field_parser_,
          std::move(opts.fade_messages))
{}

analyzing_context analyzer::context() const { return ctx_; }

context::hlasm_context& analyzer::hlasm_ctx() { return *ctx_.hlasm_ctx; }

parsing::hlasmparser_multiline& analyzer::parser() { return mngr_.opencode_parser(); }

size_t analyzer::debug_syntax_errors() { return mngr_.opencode_parser().getNumberOfSyntaxErrors(); }

lines_info analyzer::take_semantic_tokens() { return src_proc_.take_semantic_tokens(); }

void analyzer::analyze() { co_analyze().run(); }

hlasm_plugin::utils::task analyzer::co_analyze() &
{
    co_await mngr_.co_step();

    src_proc_.finish();
}

void analyzer::collect_diags() const
{
    collect_diags_from_child(mngr_);
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
