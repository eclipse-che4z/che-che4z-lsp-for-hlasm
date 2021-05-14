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
    std::string file_name, const file_manager& file_mngr, std::atomic<bool>* cancel)
    : file_impl(std::move(file_name))
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

parse_result processor_file_impl::parse(parse_lib_provider& lib_provider)
{
    if (opencode_analyzer_)
        opencode_analyzer_ = std::make_unique<analyzer>(
            get_text(), get_file_name(), lib_provider, get_lsp_editing(), std::move(opencode_analyzer_->hlasm_ctx().ids()));
    else
        opencode_analyzer_ = std::make_unique<analyzer>(get_text(), get_file_name(), lib_provider, get_lsp_editing());

    auto old_dep = dependencies_;

    auto res = parse_inner(*opencode_analyzer_);

    if (!cancel_ || !*cancel_)
    {
        dependencies_.clear();
        for (auto& file : opencode_analyzer_->hlasm_ctx().get_visited_files())
            if (file != get_file_name())
                dependencies_.insert(file);
    }

    files_to_close_.clear();
    // files that used to be dependencies but are not anymore should be closed internally
    for (auto& file : old_dep)
    {
        if (dependencies_.find(file) == dependencies_.end())
            files_to_close_.insert(file);
    }

    last_analyzer_ = opencode_analyzer_.get();
    return res;
}


parse_result processor_file_impl::parse_macro(
    parse_lib_provider& lib_provider, analyzing_context ctx, const library_data data)
{
    auto cache_key = macro_cache_key::create_from_context(*ctx.hlasm_ctx, data);

    if (macro_cache_.load_from_cache(cache_key, ctx))
        return true;

    auto a = std::make_unique<analyzer>(get_text(), get_file_name(), ctx, lib_provider, data, get_lsp_editing());

    auto ret = parse_inner(*a);

    if (!ret) // Parsing was interrupted by cancellation token, do not save the result into cache
        return false;

    last_analyzer_ = a.get();
    macro_cache_.save_analyzer(cache_key, std::move(a));
    return ret;
}

parse_result processor_file_impl::parse_no_lsp_update(
    parse_lib_provider& lib_provider, analyzing_context ctx, const library_data data)
{
    auto no_update_analyzer_ =
        std::make_unique<analyzer>(get_text(), get_file_name(), std::move(ctx), lib_provider, data, get_lsp_editing());
    no_update_analyzer_->analyze();
    return true;
}

const std::set<std::string>& processor_file_impl::dependencies() { return dependencies_; }

const semantics::lines_info& processor_file_impl::get_hl_info()
{
    return last_analyzer_->source_processor().semantic_tokens();
}

const lsp::feature_provider& processor_file_impl::get_lsp_feature_provider()
{
    return *last_analyzer_->context().lsp_ctx;
}

const std::set<std::string>& processor_file_impl::files_to_close() { return files_to_close_; }

const performance_metrics& processor_file_impl::get_metrics() { return last_analyzer_->get_metrics(); }

void processor_file_impl::erase_cache_of_opencode(const std::string& opencode_file_name)
{
    macro_cache_.erase_cache_of_opencode(opencode_file_name);
}

bool processor_file_impl::parse_inner(analyzer& new_analyzer)
{
    diags().clear();

    new_analyzer.analyze(cancel_);

    collect_diags_from_child(new_analyzer);

    if (cancel_ && *cancel_)
        return false;
    return true;
}

} // namespace hlasm_plugin::parser_library::workspaces