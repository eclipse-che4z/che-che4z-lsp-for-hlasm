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

#include "processor_file_impl.h"

#include <memory>
#include <string>

#include "file.h"

namespace hlasm_plugin::parser_library::workspaces {

processor_file_impl::processor_file_impl(
    utils::resource::resource_location file_loc, const file_manager& file_mngr, std::atomic<bool>* cancel)
    : file_impl(std::move(file_loc))
    , cancel_(cancel)
    , macro_cache_(file_mngr, *this)
{}

processor_file_impl::processor_file_impl(file_impl&& f_impl, const file_manager& file_mngr, std::atomic<bool>* cancel)
    : file_impl(std::move(f_impl))
    , cancel_(cancel)
    , macro_cache_(file_mngr, *this)
{}

processor_file_impl::processor_file_impl(
    const file_impl& file, const file_manager& file_mngr, std::atomic<bool>* cancel)
    : file_impl(file)
    , cancel_(cancel)
    , macro_cache_(file_mngr, *this)
{}

void processor_file_impl::collect_diags() const { file_impl::collect_diags(); }

bool processor_file_impl::is_once_only() const { return false; }

parse_result processor_file_impl::parse(parse_lib_provider& lib_provider,
    asm_option asm_opts,
    std::vector<preprocessor_options> pp,
    virtual_file_monitor* vfm)
{
    if (!last_analyzer_opencode_)
        last_opencode_id_storage_ = std::make_shared<context::id_storage>();

    const bool collect_hl = should_collect_hl();
    auto new_analyzer = std::make_unique<analyzer>(get_text(),
        analyzer_options {
            get_location(),
            &lib_provider,
            std::move(asm_opts),
            collect_hl ? collect_highlighting_info::yes : collect_highlighting_info::no,
            file_is_opencode::yes,
            last_opencode_id_storage_,
            std::move(pp),
            vfm,
        });
    // If parsed as opencode previously, use id_index from the last parsing

    auto old_dep = dependencies_;

    auto res = parse_inner(*new_analyzer);

    if (!cancel_ || !*cancel_)
    {
        last_analyzer_ = std::move(new_analyzer);
        last_analyzer_opencode_ = true;
        last_analyzer_with_lsp = collect_hl;

        dependencies_.clear();
        for (auto& file : last_analyzer_->hlasm_ctx().get_visited_files())
            if (file != get_location())
                dependencies_.insert(file);

        files_to_close_.clear();
        // files that used to be dependencies but are not anymore should be closed internally
        for (const auto& file : old_dep)
        {
            if (dependencies_.find(file) == dependencies_.end())
                files_to_close_.insert(file);
        }
    }

    return res;
}

parse_result processor_file_impl::parse_macro(
    parse_lib_provider& lib_provider, analyzing_context ctx, library_data data)
{
    auto cache_key = macro_cache_key::create_from_context(*ctx.hlasm_ctx, data);

    if (macro_cache_.load_from_cache(cache_key, ctx))
        return true;

    const bool collect_hl = should_collect_hl(ctx.hlasm_ctx.get());
    auto a = std::make_unique<analyzer>(get_text(),
        analyzer_options {
            get_location(),
            &lib_provider,
            std::move(ctx),
            data,
            collect_hl ? collect_highlighting_info::yes : collect_highlighting_info::no,
        });

    auto ret = parse_inner(*a);

    if (!ret) // Parsing was interrupted by cancellation token, do not save the result into cache
        return false;

    macro_cache_.save_macro(cache_key, *a);
    last_analyzer_ = std::move(a);
    last_analyzer_opencode_ = false;
    last_analyzer_with_lsp = collect_hl;

    return ret;
}

parse_result processor_file_impl::parse_no_lsp_update(
    parse_lib_provider& lib_provider, analyzing_context ctx, library_data data)
{
    auto no_update_analyzer_ = std::make_unique<analyzer>(get_text(),
        analyzer_options {
            get_location(),
            &lib_provider,
            std::move(ctx),
            data,
            should_collect_hl(ctx.hlasm_ctx.get()) ? collect_highlighting_info::yes : collect_highlighting_info::no,
        });
    no_update_analyzer_->analyze();
    return true;
}

const std::set<utils::resource::resource_location>& processor_file_impl::dependencies() { return dependencies_; }

const semantics::lines_info& processor_file_impl::get_hl_info()
{
    if (last_analyzer_)
        return last_analyzer_->source_processor().semantic_tokens();

    const static semantics::lines_info empty_lines;
    return empty_lines;
}

const lsp::lsp_context* processor_file_impl::get_lsp_context()
{
    if (last_analyzer_)
        return last_analyzer_->context().lsp_ctx.get();

    return nullptr;
}

const std::set<utils::resource::resource_location>& processor_file_impl::files_to_close() { return files_to_close_; }

const performance_metrics& processor_file_impl::get_metrics()
{
    if (last_analyzer_)
        return last_analyzer_->get_metrics();
    const static performance_metrics metrics;
    return metrics;
}

void processor_file_impl::erase_cache_of_opencode(const utils::resource::resource_location& opencode_file_location)
{
    macro_cache_.erase_cache_of_opencode(opencode_file_location);
}

bool processor_file_impl::parse_inner(analyzer& new_analyzer)
{
    new_analyzer.analyze(cancel_);

    if (cancel_ && *cancel_)
        return false;

    diags().clear();
    collect_diags_from_child(new_analyzer);
    return true;
}

bool processor_file_impl::should_collect_hl(context::hlasm_context* ctx) const
{
    // collect highlighting information in any of the following cases:
    // 1) The file is opened in the editor
    // 2) HL information was previously requested
    // 3) this macro is a top-level macro
    return get_lsp_editing() || last_analyzer_with_lsp || ctx && ctx->processing_stack().parent().empty();
}

bool processor_file_impl::has_lsp_info() const { return last_analyzer_with_lsp; }

} // namespace hlasm_plugin::parser_library::workspaces