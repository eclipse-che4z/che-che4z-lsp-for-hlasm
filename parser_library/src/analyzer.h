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

#ifndef HLASMPARSER_PARSERLIBRARY_ANALYZER_H
#define HLASMPARSER_PARSERLIBRARY_ANALYZER_H

#include "context/hlasm_context.h"
#include "diagnosable_ctx.h"
#include "hlasmparser.h"
#include "lexing/token_stream.h"
#include "parsing/parser_error_listener.h"
#include "processing/processing_manager.h"
#include "workspaces/parse_lib_provider.h"

namespace hlasm_plugin {
namespace parser_library {

// this class analyzes provided text and produces diagnostics and highlighting info with respect to provided context
class analyzer : public diagnosable_ctx
{
    context::ctx_ptr hlasm_ctx_;
    context::hlasm_context& hlasm_ctx_ref_;

    parsing::parser_error_listener listener_;

    semantics::lsp_info_processor lsp_proc_;

    lexing::input_source input_;
    lexing::lexer lexer_;
    lexing::token_stream tokens_;
    parsing::hlasmparser* parser_;

    processing::processing_manager mngr_;

public:
    analyzer(const std::string& text,
        std::string file_name,
        context::hlasm_context& hlasm_ctx,
        workspaces::parse_lib_provider& lib_provider,
        const workspaces::library_data data,
        bool collect_hl_info = false);

    analyzer(const std::string& text,
        std::string file_name = "",
        workspaces::parse_lib_provider& lib_provider = workspaces::empty_parse_lib_provider::instance,
        processing::processing_tracer* tracer = nullptr,
        bool collect_hl_info = false);

    context::hlasm_context& context();
    parsing::hlasmparser& parser();
    semantics::lsp_info_processor& lsp_processor();

    void analyze(std::atomic<bool>* cancel = nullptr);

    void collect_diags() const override;
    const performance_metrics& get_metrics();

private:
    analyzer(const std::string& text,
        std::string file_name,
        workspaces::parse_lib_provider& lib_provider,
        context::hlasm_context* hlasm_ctx,
        const workspaces::library_data data,
        bool own_ctx,
        processing::processing_tracer* tracer,
        bool collect_hl_info);
};

} // namespace parser_library
} // namespace hlasm_plugin
#endif
