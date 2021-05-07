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

processor_file_impl::processor_file_impl(std::string file_name, std::atomic<bool>* cancel)
    : file_impl(std::move(file_name))
    , cancel_(cancel)
{}

processor_file_impl::processor_file_impl(file_impl&& f_impl, std::atomic<bool>* cancel)
    : file_impl(std::move(f_impl))
    , cancel_(cancel)
{}

processor_file_impl::processor_file_impl(const file_impl& file, std::atomic<bool>* cancel)
    : file_impl(file)
    , cancel_(cancel)
{}

void processor_file_impl::collect_diags() const { file_impl::collect_diags(); }

bool processor_file_impl::is_once_only() const { return false; }

parse_result processor_file_impl::parse(parse_lib_provider& lib_provider, asm_option asm_opts)
{
    analyzer_options opts = { get_file_name(), &lib_provider, std::move(asm_opts) };
    opts.collect_hl_info = get_lsp_editing();
    analyzer_ = std::make_unique<analyzer>(get_text(), std::move(opts));

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
    parse_lib_provider& lib_provider, analyzing_context ctx, library_data data)
{
    analyzer_ = std::make_unique<analyzer>(
        get_text(), analyzer_options { get_file_name(), &lib_provider, std::move(ctx), data, get_lsp_editing() });

    return parse_inner(*analyzer_);
}

parse_result processor_file_impl::parse_no_lsp_update(
    parse_lib_provider& lib_provider, analyzing_context ctx, library_data data)
{
    auto no_update_analyzer_ = std::make_unique<analyzer>(
        get_text(), analyzer_options { get_file_name(), &lib_provider, std::move(ctx), data, get_lsp_editing() });
    no_update_analyzer_->analyze();
    return true;
}

bool processor_file_impl::parse_info_updated()
{
    bool ret = parse_info_updated_;
    parse_info_updated_ = false;
    return ret;
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

    // collect semantic info if the file is open in IDE
    if (get_lsp_editing())
        parse_info_updated_ = true;

    if (cancel_ && *cancel_)
        return false;
    return true;
}

} // namespace hlasm_plugin::parser_library::workspaces