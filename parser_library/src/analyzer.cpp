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

#include "context/hlasm_context.h"
#include "context/id_storage.h"
#include "diagnosable_ctx.h"
#include "empty_parse_lib_provider.h"
#include "lsp/lsp_context.h"
#include "parse_lib_provider.h"
#include "processing/opencode_provider.h"
#include "processing/preprocessor.h"
#include "processing/processing_manager.h"
#include "semantics/source_info_processor.h"
#include "utils/task.h"

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::lexing;
using namespace hlasm_plugin::parser_library::parsing;
using namespace hlasm_plugin::utils::resource;

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

parse_lib_provider& analyzer_options::get_lib_provider() const
{
    if (lib_provider)
        return *lib_provider;
    else
        return empty_parse_lib_provider::instance;
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

        [[nodiscard]] utils::value_task<document> generate_replacement(document doc) override
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

    std::ranges::transform(preprocessor_args, std::back_inserter(tmp.pp), transform_preprocessor);

    return std::make_unique<combined_preprocessor>(std::move(tmp));
}

struct analyzer::impl final
{
    impl(std::string_view text, analyzer_options&& opts)
        : diag_ctx(opts.get_hlasm_context(), opts.diag_limit.limit)
        , ctx(std::move(opts.get_context()))
        , src_proc(opts.collect_hl_info == collect_highlighting_info::yes)
        , field_parser(*ctx.hlasm_ctx)
        , mngr(std::make_unique<processing::opencode_provider>(text,
                   ctx,
                   opts.get_lib_provider(),
                   mngr,
                   mngr,
                   src_proc,
                   diag_ctx,
                   opts.get_preprocessor(
                       std::bind_front(&parse_lib_provider::get_library, &opts.get_lib_provider()), diag_ctx, src_proc),
                   opts.parsing_opencode == file_is_opencode::yes ? processing::opencode_provider_options { true, 10 }
                                                                  : processing::opencode_provider_options {},
                   opts.vf_monitor,
                   vf_handles),
              ctx,
              opts.dep_kind,
              std::move(opts.dep_name),
              opts.file_loc,
              text,
              opts.get_lib_provider(),
              field_parser,
              std::move(opts.fade_messages),
              opts.output,
              diag_ctx)
    {}

    diagnosable_ctx diag_ctx;

    analyzing_context ctx;

    semantics::source_info_processor src_proc;

    processing::statement_fields_parser field_parser;

    std::vector<std::pair<virtual_file_handle, utils::resource::resource_location>> vf_handles;

    processing::processing_manager mngr;

    auto& diags() noexcept { return diag_ctx.diags(); }
};

analyzer::analyzer(std::string_view text, analyzer_options opts)
    : m_impl(std::make_unique<impl>(text, std::move(opts)))
{}

analyzer::~analyzer() = default;

std::vector<std::pair<virtual_file_handle, resource_location>> analyzer::take_vf_handles()
{
    return std::move(m_impl->vf_handles);
}

analyzing_context analyzer::context() const { return m_impl->ctx; }

context::hlasm_context& analyzer::hlasm_ctx() { return *m_impl->ctx.hlasm_ctx; }

parsing::parser_holder& analyzer::parser() { return m_impl->mngr.opencode_parser(); }

semantics::lines_info analyzer::take_semantic_tokens() { return m_impl->src_proc.take_semantic_tokens(); }

void analyzer::analyze() { co_analyze().run(); }

hlasm_plugin::utils::task analyzer::co_analyze() &
{
    co_await m_impl->mngr.co_step();

    m_impl->src_proc.finish();
}

const performance_metrics& analyzer::get_metrics() const { return m_impl->ctx.hlasm_ctx->metrics; }

std::span<diagnostic> analyzer::diags() const noexcept { return m_impl->diags(); }

void analyzer::register_stmt_analyzer(processing::statement_analyzer* stmt_analyzer)
{
    m_impl->mngr.register_stmt_analyzer(stmt_analyzer);
}
