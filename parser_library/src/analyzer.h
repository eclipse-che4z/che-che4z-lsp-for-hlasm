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

enum class collect_highlighting_info : bool
{
    no,
    yes,
};

enum class file_is_opencode : bool
{
    no,
    yes,
};

class analyzer_options
{
    std::string file_name = "";
    workspaces::parse_lib_provider* lib_provider = nullptr;
    std::variant<asm_option, analyzing_context> ctx_source;
    workspaces::library_data library_data = { processing::processing_kind::ORDINARY, context::id_storage::empty_id };
    collect_highlighting_info collect_hl_info = collect_highlighting_info::no;
    file_is_opencode parsing_opencode = file_is_opencode::no;

    void set(std::string fn) { file_name = std::move(fn); }
    void set(workspaces::parse_lib_provider* lp) { lib_provider = lp; }
    void set(asm_option ao) { ctx_source = std::move(ao); }
    void set(analyzing_context ac) { ctx_source = std::move(ac); }
    void set(workspaces::library_data ld) { library_data = std::move(ld); }
    void set(collect_highlighting_info hi) { collect_hl_info = hi; }
    void set(file_is_opencode f_oc) { parsing_opencode = f_oc; }

    context::hlasm_context& get_hlasm_context();
    analyzing_context& get_context();
    workspaces::parse_lib_provider& get_lib_provider();

    friend class analyzer;

public:
    analyzer_options() = default;
    analyzer_options(const analyzer_options&) = default;
    analyzer_options(analyzer_options&&) = default;
    analyzer_options& operator=(const analyzer_options&) = default;
    analyzer_options& operator=(analyzer_options&&) = default;

    template<typename... Args>
    explicit analyzer_options(Args&&... args)
    {
        constexpr const auto string_cnt = (0 + ... + std::is_same_v<std::decay_t<Args>, std::string>);
        constexpr const auto lib_cnt = (0 + ... + std::is_same_v<std::decay_t<Args>, workspaces::parse_lib_provider*>);
        constexpr const auto ao_cnt = (0 + ... + std::is_same_v<std::decay_t<Args>, asm_option>);
        constexpr const auto ac_cnt = (0 + ... + std::is_same_v<std::decay_t<Args>, analyzing_context>);
        constexpr const auto lib_data_cnt = (0 + ... + std::is_same_v<std::decay_t<Args>, workspaces::library_data>);
        constexpr const auto hi_cnt = (0 + ... + std::is_same_v<std::decay_t<Args>, collect_highlighting_info>);
        constexpr const auto f_oc_cnt = (0 + ... + std::is_same_v<std::decay_t<Args>, file_is_opencode>);

        static_assert(string_cnt <= 1, "Duplicate argument provided");
        static_assert(lib_cnt <= 1, "Duplicate argument provided");
        static_assert(ao_cnt <= 1, "Duplicate argument provided");
        static_assert(ac_cnt <= 1, "Duplicate argument provided");
        static_assert(hi_cnt <= 1, "Duplicate argument provided");
        static_assert(lib_data_cnt <= 1, "Duplicate argument provided");
        static_assert(f_oc_cnt <= 1, "Duplicate argument provided");
        static_assert(!(ao_cnt && ac_cnt), "Do not specify both asm_option and analyzing_context");

        (set(std::forward<Args>(args)), ...);
    }
};

// this class analyzes provided text and produces diagnostics and highlighting info with respect to provided context
class analyzer : public diagnosable_ctx
{
    analyzing_context ctx_;

    parsing::parser_error_listener listener_;

    semantics::source_info_processor src_proc_;

    processing::statement_fields_parser field_parser_;

    processing::processing_manager mngr_;

public:
    analyzer(const std::string& text, analyzer_options opts = {});

    analyzing_context context();
    context::hlasm_context& hlasm_ctx();
    const semantics::source_info_processor& source_processor() const;

    void analyze(std::atomic<bool>* cancel = nullptr);

    void collect_diags() const override;
    const performance_metrics& get_metrics() const;

    void register_stmt_analyzer(processing::statement_analyzer* stmt_analyzer);

    parsing::hlasmparser& parser(); // for testing only
};

} // namespace hlasm_plugin::parser_library
#endif
