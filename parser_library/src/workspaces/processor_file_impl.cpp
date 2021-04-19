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
    , file_manager_(&file_mngr)
    , cancel_(cancel)
    , macro_cache_(file_mngr, *this)
{}

processor_file_impl::processor_file_impl(file_impl&& f_impl, const file_manager& file_mngr, std::atomic<bool>* cancel)
    : file_impl(std::move(f_impl))
    , file_manager_(&file_mngr)
    , cancel_(cancel)
    , macro_cache_(file_mngr, *this)
{}

processor_file_impl::processor_file_impl(
    const file_impl& file, const file_manager& file_mngr, std::atomic<bool>* cancel)
    : file_impl(file)
    , file_manager_(&file_mngr)
    , cancel_(cancel)
    , macro_cache_(file_mngr, *this)
{}

void processor_file_impl::collect_diags() const { file_impl::collect_diags(); }

bool processor_file_impl::is_once_only() const { return false; }

parse_result processor_file_impl::parse(parse_lib_provider& lib_provider)
{
    if (analyzer_)
        analyzer_ = std::make_unique<analyzer>(
            get_text(), get_file_name(), lib_provider, get_lsp_editing(), analyzer_->hlasm_ctx().move_ids());
    else
        analyzer_ = std::make_unique<analyzer>(get_text(), get_file_name(), lib_provider, get_lsp_editing());

    auto old_dep = dependencies_;

    auto res = parse_inner(*analyzer_);

    if (!cancel_ || !*cancel_)
    {
        dependencies_.clear();
        for (auto& file : analyzer_->hlasm_ctx().get_visited_files())
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

    return res;
}


parse_result processor_file_impl::parse_macro(
    parse_lib_provider& lib_provider, analyzing_context ctx, const library_data data)
{
    analyzer_ =
        std::make_unique<analyzer>(get_text(), get_file_name(), std::move(ctx), lib_provider, data, get_lsp_editing());

    if (macro_cache_.load_from_cache(ctx, data))
        return true;

    auto ret = parse_inner(*analyzer_);

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
    return analyzer_->source_processor().semantic_tokens();
}

const lsp::feature_provider& processor_file_impl::get_lsp_feature_provider() { return *analyzer_->context().lsp_ctx; }

const std::set<std::string>& processor_file_impl::files_to_close() { return files_to_close_; }

const performance_metrics& processor_file_impl::get_metrics() { return analyzer_->get_metrics(); }

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