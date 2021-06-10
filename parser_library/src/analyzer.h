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

#include <optional>
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
    std::optional<context::id_storage> ids_init;

    void set(std::string fn) { file_name = std::move(fn); }
    void set(workspaces::parse_lib_provider* lp) { lib_provider = lp; }
    void set(asm_option ao) { ctx_source = std::move(ao); }
    void set(analyzing_context ac) { ctx_source = std::move(ac); }
    void set(workspaces::library_data ld) { library_data = std::move(ld); }
    void set(collect_highlighting_info hi) { collect_hl_info = hi; }
    void set(file_is_opencode f_oc) { parsing_opencode = f_oc; }
    void set(context::id_storage ids) { ids_init.emplace(std::move(ids)); }

    context::hlasm_context& get_hlasm_context();
    analyzing_context& get_context();
    workspaces::parse_lib_provider& get_lib_provider();

    friend class analyzer;

public:
    analyzer_options() = default;
    analyzer_options(analyzer_options&&) = default;

    template<typename... Args>
    explicit analyzer_options(Args&&... args)
    {
        constexpr auto string_cnt = (0 + ... + std::is_convertible_v<std::decay_t<Args>, std::string>);
        constexpr auto lib_cnt = (0 + ... + std::is_convertible_v<std::decay_t<Args>, workspaces::parse_lib_provider*>);
        constexpr auto ao_cnt = (0 + ... + std::is_same_v<std::decay_t<Args>, asm_option>);
        constexpr auto ac_cnt = (0 + ... + std::is_same_v<std::decay_t<Args>, analyzing_context>);
        constexpr auto lib_data_cnt = (0 + ... + std::is_same_v<std::decay_t<Args>, workspaces::library_data>);
        constexpr auto hi_cnt = (0 + ... + std::is_same_v<std::decay_t<Args>, collect_highlighting_info>);
        constexpr auto f_oc_cnt = (0 + ... + std::is_same_v<std::decay_t<Args>, file_is_opencode>);
        constexpr auto ids_cnt = (0 + ... + std::is_same_v<std::decay_t<Args>, context::id_storage>);
        constexpr auto cnt = string_cnt + lib_cnt + ao_cnt + ac_cnt + lib_data_cnt + hi_cnt + f_oc_cnt + ids_cnt;

        static_assert(string_cnt <= 1, "Duplicate file_name");
        static_assert(lib_cnt <= 1, "Duplicate parse_lib_provider");
        static_assert(ao_cnt <= 1, "Duplicate asm_option");
        static_assert(ac_cnt <= 1, "Duplicate analyzing_context");
        static_assert(lib_data_cnt <= 1, "Duplicate library_data");
        static_assert(hi_cnt <= 1, "Duplicate collect_highlighting_info");
        static_assert(f_oc_cnt <= 1, "Duplicate file_is_opencode");
        static_assert(ids_cnt <= 1, "Duplicate id_storage");
        static_assert(
            !(ac_cnt && (ao_cnt || ids_cnt)), "Do not specify both analyzing_context and asm_option or id_storage");
        static_assert(cnt == sizeof...(Args), "Unrecognized argument provided");

        (set(std::forward<Args>(args)), ...);
    }
};

// this class analyzes provided text and produces diagnostics and highlighting info with respect to provided context
class analyzer : public diagnosable_ctx
{
    analyzing_context ctx_;

    semantics::source_info_processor src_proc_;

    processing::statement_fields_parser field_parser_;

    processing::processing_manager mngr_;

public:
    analyzer(const std::string& text, analyzer_options opts = {});

    analyzing_context context() const;
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
