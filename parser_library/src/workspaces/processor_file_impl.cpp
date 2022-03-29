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

parse_result processor_file_impl::parse(
    parse_lib_provider& lib_provider, asm_option asm_opts, preprocessor_options pp, virtual_file_monitor* vfm)
{
    if (!last_analyzer_opencode_)
        last_opencode_id_storage_ = std::make_shared<context::id_storage>();

    last_analyzer_ = std::make_unique<analyzer>(get_text(),
        analyzer_options {
            get_file_name(),
            &lib_provider,
            std::move(asm_opts),
            get_lsp_editing() ? collect_highlighting_info::yes : collect_highlighting_info::no,
            file_is_opencode::yes,
            last_opencode_id_storage_,
            std::move(pp),
            vfm,
        });
    // If parsed as opencode previously, use id_index from the last parsing

    last_analyzer_opencode_ = true;

    auto old_dep = dependencies_;

    auto res = parse_inner(*last_analyzer_);

    if (!cancel_ || !*cancel_)
    {
        dependencies_.clear();
        for (auto& file : last_analyzer_->hlasm_ctx().get_visited_files())
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
    auto cache_key = macro_cache_key::create_from_context(*ctx.hlasm_ctx, data);

    if (macro_cache_.load_from_cache(cache_key, ctx))
        return true;

    auto a = std::make_unique<analyzer>(get_text(),
        analyzer_options {
            get_file_name(),
            &lib_provider,
            std::move(ctx),
            data,
            get_lsp_editing() ? collect_highlighting_info::yes : collect_highlighting_info::no,
        });

    auto ret = parse_inner(*a);

    if (!ret) // Parsing was interrupted by cancellation token, do not save the result into cache
        return false;

    macro_cache_.save_macro(cache_key, *a);
    last_analyzer_ = std::move(a);
    last_analyzer_opencode_ = false;

    return ret;
}

parse_result processor_file_impl::parse_no_lsp_update(
    parse_lib_provider& lib_provider, analyzing_context ctx, library_data data)
{
    auto no_update_analyzer_ = std::make_unique<analyzer>(get_text(),
        analyzer_options {
            get_file_name(),
            &lib_provider,
            std::move(ctx),
            data,
            get_lsp_editing() ? collect_highlighting_info::yes : collect_highlighting_info::no,
        });
    no_update_analyzer_->analyze();
    return true;
}

const std::set<std::string>& processor_file_impl::dependencies() { return dependencies_; }

const semantics::lines_info& processor_file_impl::get_hl_info()
{
    if (last_analyzer_)
        return last_analyzer_->source_processor().semantic_tokens();

    const static semantics::lines_info empty_lines;
    return empty_lines;
}

namespace {
class empty_feature_provider final : public lsp::feature_provider
{
    // Inherited via feature_provider
    location definition(const std::string& document_uri, position pos) const override
    {
        return location(pos, document_uri);
    }
    location_list references(const std::string&, position) const override { return location_list(); }
    lsp::hover_result hover(const std::string&, position) const override { return lsp::hover_result(); }
    lsp::completion_list_s completion(const std::string&, position, char, completion_trigger_kind) const override
    {
        return {};
    }
    lsp::document_symbol_list_s document_symbol(const std::string&, long long) const override { return {}; }
};
} // namespace

const lsp::feature_provider& processor_file_impl::get_lsp_feature_provider()
{
    if (last_analyzer_)
        return *last_analyzer_->context().lsp_ctx;

    const static empty_feature_provider empty_res;
    return empty_res;
}

const std::set<std::string>& processor_file_impl::files_to_close() { return files_to_close_; }

const performance_metrics& processor_file_impl::get_metrics()
{
    if (last_analyzer_)
        return last_analyzer_->get_metrics();
    const static performance_metrics metrics;
    return metrics;
}

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