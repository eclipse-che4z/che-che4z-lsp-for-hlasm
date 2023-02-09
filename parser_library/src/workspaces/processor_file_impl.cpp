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

#include <cassert>
#include <memory>
#include <string>
#include <utility>

#include "file.h"
#include "file_manager.h"

namespace hlasm_plugin::parser_library::workspaces {

processor_file_impl::processor_file_impl(std::shared_ptr<file> file, file_manager& file_mngr, std::atomic<bool>* cancel)
    : file_mngr_(file_mngr)
    , file_(std::move(file))
    , cancel_(cancel)
    , macro_cache_(file_mngr, file_)
{}

void processor_file_impl::collect_diags() const {}
bool processor_file_impl::is_once_only() const { return false; }

parse_result processor_file_impl::parse(parse_lib_provider& lib_provider,
    asm_option asm_opts,
    std::vector<preprocessor_options> pp,
    virtual_file_monitor* vfm)
{
    if (!last_opencode_id_storage_)
        last_opencode_id_storage_ = std::make_shared<context::id_storage>();

    const bool collect_hl = should_collect_hl();
    auto fms = std::make_shared<std::vector<fade_message_s>>();
    auto new_analyzer = std::make_unique<analyzer>(file_->get_text(),
        analyzer_options {
            file_->get_location(),
            &lib_provider,
            std::move(asm_opts),
            collect_hl ? collect_highlighting_info::yes : collect_highlighting_info::no,
            file_is_opencode::yes,
            last_opencode_id_storage_,
            std::move(pp),
            vfm,
            fms,
        });
    // If parsed as opencode previously, use id_index from the last parsing

    auto old_dep = dependencies_;

    new_analyzer->analyze(cancel_);

    if (cancel_ && *cancel_)
        return false;

    diags().clear();
    collect_diags_from_child(*new_analyzer);

    last_analyzer_ = std::move(new_analyzer);
    last_analyzer_with_lsp = collect_hl;
    last_hl_info_ = last_analyzer_->source_processor().semantic_tokens();

    dependencies_.clear();
    for (auto& file : last_analyzer_->hlasm_ctx().get_visited_files())
        if (file != file_->get_location())
            dependencies_.insert(file);

    files_to_close_.clear();
    // files that used to be dependencies but are not anymore should be closed internally
    for (const auto& file : old_dep)
    {
        if (dependencies_.find(file) == dependencies_.end())
            files_to_close_.insert(file);
    }

    fade_messages_ = std::move(fms);

    return true;
}

parse_result processor_file_impl::parse_macro(
    parse_lib_provider& lib_provider, analyzing_context ctx, library_data data)
{
    auto cache_key = macro_cache_key::create_from_context(*ctx.hlasm_ctx, data);

    if (macro_cache_.load_from_cache(cache_key, ctx))
        return true;

    const bool collect_hl = should_collect_hl(ctx.hlasm_ctx.get());
    auto fms = std::make_shared<std::vector<fade_message_s>>();
    analyzer a(file_->get_text(),
        analyzer_options {
            file_->get_location(),
            &lib_provider,
            std::move(ctx),
            data,
            collect_hl ? collect_highlighting_info::yes : collect_highlighting_info::no,
            fms,
        });

    a.analyze(cancel_);

    if (cancel_ && *cancel_)
        return false;

    diags().clear();
    collect_diags_from_child(a);

    macro_cache_.save_macro(cache_key, a);
    last_analyzer_with_lsp = collect_hl;
    if (collect_hl)
        last_hl_info_ = a.source_processor().semantic_tokens();

    fade_messages_ = std::move(fms);

    return true;
}

const std::set<utils::resource::resource_location>& processor_file_impl::dependencies() { return dependencies_; }

const semantics::lines_info& processor_file_impl::get_hl_info() { return last_hl_info_; }

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

void processor_file_impl::erase_unused_cache_entries() { macro_cache_.erase_unused(); }

bool processor_file_impl::should_collect_hl(context::hlasm_context* ctx) const
{
    // collect highlighting information in any of the following cases:
    // 1) The file is opened in the editor
    // 2) HL information was previously requested
    // 3) this macro is a top-level macro
    return file_->get_lsp_editing() || last_analyzer_with_lsp || ctx && ctx->processing_stack().parent().empty();
}

bool processor_file_impl::has_lsp_info() const { return last_analyzer_ && last_analyzer_with_lsp; }

void processor_file_impl::retrieve_fade_messages(std::vector<fade_message_s>& fms) const
{
    fms.insert(std::end(fms), std::begin(*fade_messages_), std::end(*fade_messages_));
}

const file_location& processor_file_impl::get_location() const { return file_->get_location(); }

bool processor_file_impl::current_version() const
{
    auto f = file_mngr_.find(get_location());
    return f == file_;
}

void processor_file_impl::update_source()
{
    last_analyzer_.reset();
    last_hl_info_.clear();
    used_files.clear();
    file_ = file_mngr_.add_file(get_location());
    macro_cache_ = macro_cache(file_mngr_, file_);
    diags().clear();
}

void processor_file_impl::store_used_files(std::unordered_map<utils::resource::resource_location,
    std::shared_ptr<file>,
    utils::resource::resource_location_hasher> uf)
{
    used_files = std::move(uf);
}

} // namespace hlasm_plugin::parser_library::workspaces
