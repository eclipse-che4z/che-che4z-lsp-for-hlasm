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
#include "lsp/lsp_context.h"

namespace hlasm_plugin::parser_library::workspaces {

processor_file_impl::processor_file_impl(std::shared_ptr<file> file, file_manager& file_mngr, std::atomic<bool>* cancel)
    : m_file_mngr(file_mngr)
    , m_file(std::move(file))
    , m_cancel(cancel)
    , m_macro_cache(file_mngr, m_file)
{}

void processor_file_impl::collect_diags() const {}
bool processor_file_impl::is_once_only() const { return false; }

bool processor_file_impl::parse(parse_lib_provider& lib_provider,
    asm_option asm_opts,
    std::vector<preprocessor_options> pp,
    virtual_file_monitor* vfm)
{
    if (!m_last_opencode_id_storage)
        m_last_opencode_id_storage = std::make_shared<context::id_storage>();

    const bool collect_hl = should_collect_hl();
    auto fms = std::make_shared<std::vector<fade_message_s>>();
    auto new_analyzer = std::make_unique<analyzer>(m_file->get_text(),
        analyzer_options {
            m_file->get_location(),
            &lib_provider,
            std::move(asm_opts),
            collect_hl ? collect_highlighting_info::yes : collect_highlighting_info::no,
            file_is_opencode::yes,
            m_last_opencode_id_storage,
            std::move(pp),
            vfm,
            fms,
        });

    auto old_dep = m_dependencies;

    processing::hit_count_analyzer hc_analyzer(new_analyzer->hlasm_ctx());
    new_analyzer->register_stmt_analyzer(&hc_analyzer); // TODO dangling reference to hc_analyzer

    for (auto a = new_analyzer->co_analyze(); !a.done(); a())
    {
        if (m_cancel && m_cancel->load(std::memory_order_relaxed))
            return false;
    }

    diags().clear();
    collect_diags_from_child(*new_analyzer);

    m_last_analyzer = std::move(new_analyzer);
    m_last_analyzer_with_lsp = collect_hl;
    m_last_hl_info = m_last_analyzer->source_processor().semantic_tokens();

    m_dependencies.clear();
    for (auto& file : m_last_analyzer->hlasm_ctx().get_visited_files())
        if (file != m_file->get_location())
            m_dependencies.insert(file);

    m_files_to_close.clear();
    // files that used to be dependencies but are not anymore should be closed internally
    for (const auto& file : old_dep)
    {
        if (!m_dependencies.contains(file))
            m_files_to_close.insert(file);
    }

    m_hc_map = hc_analyzer.take_hit_count_map();
    m_fade_messages = std::move(fms);

    return true;
}

bool processor_file_impl::parse_macro(parse_lib_provider& lib_provider, analyzing_context ctx, library_data data)
{
    auto cache_key = macro_cache_key::create_from_context(*ctx.hlasm_ctx, data);

    if (m_macro_cache.load_from_cache(cache_key, ctx))
        return true;

    const bool collect_hl = should_collect_hl(ctx.hlasm_ctx.get());
    analyzer a(m_file->get_text(),
        analyzer_options {
            m_file->get_location(),
            &lib_provider,
            std::move(ctx),
            data,
            collect_hl ? collect_highlighting_info::yes : collect_highlighting_info::no,
        });

    processing::hit_count_analyzer hc_analyzer(a.hlasm_ctx());
    a.register_stmt_analyzer(&hc_analyzer);

    for (auto co_a = a.co_analyze(); !co_a.done(); co_a())
    {
        if (m_cancel && m_cancel->load(std::memory_order_relaxed))
            return false;
    }

    diags().clear();
    collect_diags_from_child(a);

    m_macro_cache.save_macro(cache_key, a);
    m_last_analyzer_with_lsp = collect_hl;
    if (collect_hl)
        m_last_hl_info = a.source_processor().semantic_tokens();

    m_hc_map = hc_analyzer.take_hit_count_map();

    return true;
}

const std::set<utils::resource::resource_location>& processor_file_impl::dependencies() { return m_dependencies; }

const semantics::lines_info& processor_file_impl::get_hl_info() { return m_last_hl_info; }

const lsp::lsp_context* processor_file_impl::get_lsp_context() const
{
    if (m_last_analyzer)
        return m_last_analyzer->context().lsp_ctx.get();

    return nullptr;
}

const std::set<utils::resource::resource_location>& processor_file_impl::files_to_close() { return m_files_to_close; }

const performance_metrics& processor_file_impl::get_metrics()
{
    if (m_last_analyzer)
        return m_last_analyzer->get_metrics();
    const static performance_metrics metrics;
    return metrics;
}

void processor_file_impl::erase_unused_cache_entries() { m_macro_cache.erase_unused(); }

bool processor_file_impl::should_collect_hl(context::hlasm_context* ctx) const
{
    // collect highlighting information in any of the following cases:
    // 1) The file is opened in the editor
    // 2) HL information was previously requested
    // 3) this macro is a top-level macro
    return m_file->get_lsp_editing() || m_last_analyzer_with_lsp || ctx && ctx->processing_stack().parent().empty();
}

bool processor_file_impl::has_lsp_info() const { return m_last_analyzer_with_lsp; }

const std::vector<fade_message_s>& processor_file_impl::fade_messages() const { return *m_fade_messages; }
const processing::hit_count_map& processor_file_impl::hit_count_map() const { return m_hc_map; }

const file_location& processor_file_impl::get_location() const { return m_file->get_location(); }

bool processor_file_impl::current_version() const
{
    auto f = m_file_mngr.find(get_location());
    return f == m_file;
}

void processor_file_impl::update_source()
{
    m_last_analyzer.reset();
    m_last_hl_info.clear();
    used_files.clear();
    m_file = m_file_mngr.add_file(get_location());
    m_macro_cache = macro_cache(m_file_mngr, m_file);
    m_hc_map.clear();
    diags().clear();
}

void processor_file_impl::store_used_files(std::unordered_map<utils::resource::resource_location,
    std::shared_ptr<file>,
    utils::resource::resource_location_hasher> uf)
{
    used_files = std::move(uf);
}

} // namespace hlasm_plugin::parser_library::workspaces
