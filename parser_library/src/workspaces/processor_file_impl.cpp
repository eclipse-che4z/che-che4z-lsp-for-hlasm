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

// Contains all the context that affects parsing an external file (macro or copy member)
struct macro_cache_key
{
    library_data data;
    std::string opencode_name;
    std::vector<context::opcode_t> opsyn_state;
};



using version_stamp = std::vector<std::pair<std::string, version_t>>;


parse_result processor_file_impl::parse_macro(
    parse_lib_provider& lib_provider, analyzing_context ctx, const library_data data)
{
    analyzer_ =
        std::make_unique<analyzer>(get_text(), get_file_name(), std::move(ctx), lib_provider, data, get_lsp_editing());

    /*std::variant<lsp::macro_info_ptr, context::copy_member_ptr> external_dep;
    assert(data.proc_kind == processing::processing_kind::MACRO || data.proc_kind == processing::processing_kind::COPY);
    if (data.proc_kind == processing::processing_kind::MACRO)
        external_dep = ctx.lsp_ctx->get_macro_info(data.library_member);
    else if (data.proc_kind == processing::processing_kind::COPY)
        external_dep = ctx.hlasm_ctx->get_copy_member(data.library_member);
        */
    //std::vector<std::pair<macro_cache_key, version_t>>
    

    return parse_inner(*analyzer_);
}

parse_result processor_file_impl::parse_no_lsp_update(
    parse_lib_provider& lib_provider, analyzing_context ctx, const library_data data)
{
    auto no_update_analyzer_ =
        std::make_unique<analyzer>(get_text(), get_file_name(), std::move(ctx), lib_provider, data, get_lsp_editing());
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