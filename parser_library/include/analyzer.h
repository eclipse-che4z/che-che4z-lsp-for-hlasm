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

#include <concepts>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include "analyzing_context.h"
#include "compiler_options.h"
#include "preprocessor_options.h"
#include "processing_format.h"
#include "protocol.h"
#include "utils/resource_location.h"

namespace hlasm_plugin::utils {
class task;
template<std::move_constructible T>
class value_task;
} // namespace hlasm_plugin::utils

namespace hlasm_plugin::parser_library::context {
class hlasm_context;
class id_storage;
} // namespace hlasm_plugin::parser_library::context

namespace hlasm_plugin::parser_library::parsing {
class parser_holder;
} // namespace hlasm_plugin::parser_library::parsing

namespace hlasm_plugin::parser_library::processing {
class preprocessor;
class statement_analyzer;
} // namespace hlasm_plugin::parser_library::processing

namespace hlasm_plugin::parser_library::semantics {
class source_info_processor;
} // namespace hlasm_plugin::parser_library::semantics

namespace hlasm_plugin::parser_library {
struct fade_message;
class output_handler;
class parse_lib_provider;
class virtual_file_monitor;
class virtual_file_handle;

template<typename T>
class diagnostic_consumer_t;
struct diagnostic;
struct diagnostic_op;

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

struct diagnostic_limit
{
    size_t limit = static_cast<size_t>(-1);
};

class analyzer_options
{
    class dependency_data
    {
        std::string name;
        processing::processing_kind kind = processing::processing_kind::ORDINARY;

        dependency_data() = default;
        explicit dependency_data(std::string_view name, processing::processing_kind kind)
            : name(name)
            , kind(kind)
        {}

        friend class analyzer_options;
    };
    utils::resource::resource_location file_loc = utils::resource::resource_location("");
    parse_lib_provider* lib_provider = nullptr;
    std::variant<asm_option, analyzing_context> ctx_source;
    collect_highlighting_info collect_hl_info = collect_highlighting_info::no;
    file_is_opencode parsing_opencode = file_is_opencode::no;
    std::shared_ptr<context::id_storage> ids_init;
    std::vector<preprocessor_options> preprocessor_args;
    virtual_file_monitor* vf_monitor = nullptr;
    std::shared_ptr<std::vector<fade_message>> fade_messages = nullptr;
    output_handler* output = nullptr;
    std::string dep_name;
    processing::processing_kind dep_kind = processing::processing_kind::ORDINARY;
    diagnostic_limit diag_limit;

    void set(utils::resource::resource_location rl) { file_loc = std::move(rl); }
    void set(parse_lib_provider* lp) { lib_provider = lp; }
    void set(asm_option ao) { ctx_source = std::move(ao); }
    void set(analyzing_context ac) { ctx_source = std::move(ac); }
    void set(collect_highlighting_info hi) { collect_hl_info = hi; }
    void set(file_is_opencode f_oc) { parsing_opencode = f_oc; }
    void set(std::shared_ptr<context::id_storage> ids) { ids_init = std::move(ids); }
    void set(preprocessor_options pp) { preprocessor_args.push_back(std::move(pp)); }
    void set(std::vector<preprocessor_options> pp) { preprocessor_args = std::move(pp); }
    void set(virtual_file_monitor* vfm) { vf_monitor = vfm; }
    void set(std::shared_ptr<std::vector<fade_message>> fmc) { fade_messages = fmc; };
    void set(output_handler* o) { output = o; }
    void set(dependency_data d)
    {
        dep_kind = d.kind;
        dep_name = std::move(d.name);
    }
    void set(diagnostic_limit dl) { diag_limit = dl; }

    context::hlasm_context& get_hlasm_context();
    analyzing_context& get_context();
    parse_lib_provider& get_lib_provider() const;
    std::unique_ptr<processing::preprocessor> get_preprocessor(
        std::function<utils::value_task<std::optional<std::pair<std::string, utils::resource::resource_location>>>(
            std::string)>,
        diagnostic_consumer_t<diagnostic_op>&,
        semantics::source_info_processor&) const;

    friend class analyzer;

public:
    analyzer_options() = default;
    analyzer_options(analyzer_options&&) = default;

    template<typename... Args>
    explicit analyzer_options(Args&&... args)
    {
        constexpr auto rl_cnt =
            (0 + ... + std::is_convertible_v<std::decay_t<Args>, utils::resource::resource_location>);
        constexpr auto lib_cnt = (0 + ... + std::is_convertible_v<std::decay_t<Args>, parse_lib_provider*>);
        constexpr auto ao_cnt = (0 + ... + std::is_same_v<std::decay_t<Args>, asm_option>);
        constexpr auto ac_cnt = (0 + ... + std::is_same_v<std::decay_t<Args>, analyzing_context>);
        constexpr auto hi_cnt = (0 + ... + std::is_same_v<std::decay_t<Args>, collect_highlighting_info>);
        constexpr auto f_oc_cnt = (0 + ... + std::is_same_v<std::decay_t<Args>, file_is_opencode>);
        constexpr auto ids_cnt = (0 + ... + std::is_same_v<std::decay_t<Args>, std::shared_ptr<context::id_storage>>);
        constexpr auto pp_cnt = (0 + ... + std::is_convertible_v<std::decay_t<Args>, preprocessor_options>)+(
            0 + ... + std::is_same_v<std::decay_t<Args>, std::vector<preprocessor_options>>);
        constexpr auto vfm_cnt = (0 + ... + std::is_convertible_v<std::decay_t<Args>, virtual_file_monitor*>);
        constexpr auto fmc_cnt =
            (0 + ... + std::is_same_v<std::decay_t<Args>, std::shared_ptr<std::vector<fade_message>>>);
        constexpr auto o_cnt = (0 + ... + std::is_convertible_v<std::decay_t<Args>, output_handler*>);
        constexpr auto dep_data_cnt = (0 + ... + std::is_convertible_v<std::decay_t<Args>, dependency_data>);
        constexpr auto diag_limit_cnt = (0 + ... + std::is_convertible_v<std::decay_t<Args>, diagnostic_limit>);
        constexpr auto cnt = rl_cnt + lib_cnt + ao_cnt + ac_cnt + hi_cnt + f_oc_cnt + ids_cnt + pp_cnt + vfm_cnt
            + fmc_cnt + o_cnt + dep_data_cnt + diag_limit_cnt;

        static_assert(rl_cnt <= 1, "Duplicate resource_location");
        static_assert(lib_cnt <= 1, "Duplicate parse_lib_provider");
        static_assert(ao_cnt <= 1, "Duplicate asm_option");
        static_assert(ac_cnt <= 1, "Duplicate analyzing_context");
        static_assert(hi_cnt <= 1, "Duplicate collect_highlighting_info");
        static_assert(f_oc_cnt <= 1, "Duplicate file_is_opencode");
        static_assert(ids_cnt <= 1, "Duplicate id_storage");
        static_assert(pp_cnt <= 1, "Duplicate preprocessor_args");
        static_assert(vfm_cnt <= 1, "Duplicate virtual_file_monitor");
        static_assert(fmc_cnt <= 1, "Duplicate fade message container");
        static_assert(!(ac_cnt && (ao_cnt || ids_cnt || pp_cnt)),
            "Do not specify both analyzing_context and asm_option, id_storage or preprocessor_args");
        static_assert(o_cnt <= 1, "Duplicate output_handler");
        static_assert(dep_data_cnt <= 1, "Duplicate dependency_data");
        static_assert(diag_limit_cnt <= 1, "Duplicate diagnostic_limit");
        static_assert(cnt == sizeof...(Args), "Unrecognized argument provided");

        (set(std::forward<Args>(args)), ...);
    }

    static dependency_data dependency(std::string n, processing::processing_kind k)
    {
        return dependency_data(std::move(n), k);
    }
};

// this class analyzes provided text and produces diagnostics and highlighting info with respect to provided context
class analyzer
{
    struct impl;

    std::unique_ptr<impl> m_impl;

public:
    analyzer(std::string_view text, analyzer_options opts = {});
    ~analyzer();

    std::vector<std::pair<virtual_file_handle, utils::resource::resource_location>> take_vf_handles();
    analyzing_context context() const;

    context::hlasm_context& hlasm_ctx();
    std::vector<token_info> take_semantic_tokens();

    void analyze();
    [[nodiscard]] utils::task co_analyze() &;

    const performance_metrics& get_metrics() const;

    std::span<diagnostic> diags() const noexcept;

    void register_stmt_analyzer(processing::statement_analyzer* stmt_analyzer);

    parsing::parser_holder& parser(); // for testing only
};

} // namespace hlasm_plugin::parser_library
#endif
