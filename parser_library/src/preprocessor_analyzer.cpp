/*
 * Copyright (c) 2022 Broadcom.
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

#include "preprocessor_analyzer.h"

#include "processing/statement_providers/endevor_statement_provider.h"

namespace hlasm_plugin::parser_library {

hlasm_plugin::parser_library::preprocessor_analyzer::preprocessor_analyzer(const std::string& text,
    size_t line_no,
    analyzing_context ctx,
    processing::processing_kind proc_kind,
    workspaces::parse_lib_provider& lib_provider)
    : analyzer_if(*ctx.hlasm_ctx)
    , m_line_no(line_no)
    , m_proc_kind(proc_kind)
    , m_src_proc(true)
{
    m_stmt_providers[processing::processing_kind::ENDEVOR] =
        std::make_unique<processing::endevor_statement_provider>(text, ctx, m_line_no, m_src_proc);

    m_endevor_processor = std::make_unique<processing::endevor_processor>(ctx, lib_provider);
    m_endevor_analyzer = std::make_unique<processing::endevor_analyzer>(ctx, text);
}

void preprocessor_analyzer::analyze(std::atomic<bool>* cancel)
{
    if (cancel && *cancel)
        return;

    if (m_stmt_providers.find(m_proc_kind) == m_stmt_providers.end())
        return;

    processing::statement_provider& stmt_provider = *m_stmt_providers[m_proc_kind];

    auto stmt = stmt_provider.get_next(*m_endevor_processor);
    if (stmt)
    {
        m_endevor_analyzer->analyze(
            *stmt, processing::statement_provider_kind::OPEN, processing::processing_kind::ENDEVOR);
        m_endevor_processor->process_statement(std::move(stmt));
    }
}

const semantics::source_info_processor& preprocessor_analyzer::source_processor() const { return m_src_proc; }

const performance_metrics& preprocessor_analyzer::get_metrics() const
{
    static performance_metrics empty_perf_metrics = {};

    return empty_perf_metrics;
}

void preprocessor_analyzer::collect_diags() const {}

} // namespace hlasm_plugin::parser_library