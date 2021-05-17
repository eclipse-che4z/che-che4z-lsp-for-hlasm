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

#include <variant>

#include "analyzing_context.h"
#include "context/hlasm_context.h"
#include "diagnosable_ctx.h"
#include "hlasmparser.h"
#include "lexing/token_stream.h"
#include "lsp/lsp_context.h"
#include "parsing/parser_error_listener.h"
#include "processing/processing_manager.h"
#include "workspaces/parse_lib_provider.h"

namespace hlasm_plugin::parser_library {
struct analyzer_options
{
    std::string file_name = "";
    workspaces::parse_lib_provider* lib_provider = nullptr;
    std::variant<asm_option, analyzing_context> ctx_source;
    workspaces::library_data library_data = { processing::processing_kind::ORDINARY, context::id_storage::empty_id };
    bool collect_hl_info = false;

    context::hlasm_context& get_hlasm_context();
    analyzing_context& get_context();
    workspaces::parse_lib_provider& get_lib_provider();
};

// this class analyzes provided text and produces diagnostics and highlighting info with respect to provided context
class analyzer : public diagnosable_ctx
{
    analyzing_context ctx_;

    parsing::parser_error_listener listener_;

    semantics::source_info_processor src_proc_;

    // lexing::input_source input_;
    // lexing::lexer lexer_;
    // lexing::token_stream tokens_;
    // parsing::hlasmparser* parser_;

    processing::statement_fields_parser field_parser_;

    processing::processing_manager mngr_;

public:
    analyzer(const std::string& text, analyzer_options opts = { "", nullptr, asm_option {} });

    analyzing_context context();
    context::hlasm_context& hlasm_ctx();
    parsing::hlasmparser& parser();
    bool feed_line();
    const semantics::source_info_processor& source_processor() const;

    void analyze(std::atomic<bool>* cancel = nullptr);

    void collect_diags() const override;
    const performance_metrics& get_metrics() const;

    void register_stmt_analyzer(processing::statement_analyzer* stmt_analyzer);
};

} // namespace hlasm_plugin::parser_library
#endif
