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

#ifndef HLASMPLUGIN_PARSERLIBRARY_WORKSPACE_MANAGER_IMPL_H
#define HLASMPLUGIN_PARSERLIBRARY_WORKSPACE_MANAGER_IMPL_H

#include <algorithm>
#include <atomic>

#include "debugging/debug_lib_provider.h"
#include "workspace_manager.h"
#include "workspaces/file_manager_impl.h"
#include "workspaces/workspace.h"

namespace hlasm_plugin::parser_library {

// Implementation of workspace manager (Implementation part of the pimpl idiom)
// Holds workspaces, file manager and macro tracer and handles LSP and DAP
// notifications and requests.
class workspace_manager::impl final : public diagnosable_impl
{
    static constexpr lib_config supress_all { 0 };

public:
    impl(std::atomic<bool>* cancel = nullptr)
        : cancel_(cancel)
        , file_manager_(cancel)
        , implicit_workspace_(file_manager_, global_config_, m_global_settings, cancel)
        , quiet_implicit_workspace_(file_manager_, supress_all, m_global_settings, cancel)
    {}
    impl(const impl&) = delete;
    impl& operator=(const impl&) = delete;

    impl(impl&&) = delete;
    impl& operator=(impl&&) = delete;

    size_t get_workspaces(ws_id* workspaces, size_t max_size)
    {
        size_t size = 0;

        for (auto it = workspaces_.begin(); size < max_size && it != workspaces_.end(); ++size, ++it)
        {
            workspaces[size] = &it->second;
        }
        return size;
    }

    size_t get_workspaces_count() const { return workspaces_.size(); }

    void add_workspace(std::string name, std::string uri)
    {
        auto ws = workspaces_.emplace(name,
            workspaces::workspace(utils::resource::resource_location(std::move(uri)),
                name,
                file_manager_,
                global_config_,
                m_global_settings,
                cancel_));
        ws.first->second.set_message_consumer(message_consumer_);
        ws.first->second.open();

        notify_diagnostics_consumers();
    }
    ws_id find_workspace(const std::string& document_uri) { return &ws_path_match(document_uri); }
    void remove_workspace(std::string uri)
    {
        auto it = workspaces_.find(uri);
        if (it == workspaces_.end())
            return; // erase does no action, if the key does not exist
        workspaces_.erase(uri);
        notify_diagnostics_consumers();
    }

    void did_open_file(const utils::resource::resource_location& document_loc, version_t version, std::string text)
    {
        file_manager_.did_open_file(document_loc, version, std::move(text));
        if (cancel_ && *cancel_)
            return;

        workspaces::workspace& ws = ws_path_match(document_loc.get_uri());
        auto metadata = ws.did_open_file(document_loc);
        if (cancel_ && *cancel_)
            return;

        notify_diagnostics_consumers();
        // only on open
        notify_performance_consumers(document_loc, metadata);
    }
    void did_change_file(const utils::resource::resource_location& document_loc,
        version_t version,
        const document_change* changes,
        size_t ch_size)
    {
        file_manager_.did_change_file(document_loc, version, changes, ch_size);
        if (cancel_ && *cancel_)
            return;

        workspaces::workspace& ws = ws_path_match(document_loc.get_uri());
        ws.did_change_file(document_loc, changes, ch_size);
        if (cancel_ && *cancel_)
            return;

        notify_diagnostics_consumers();
    }

    void did_close_file(const utils::resource::resource_location& document_loc)
    {
        workspaces::workspace& ws = ws_path_match(document_loc.get_uri());
        ws.did_close_file(document_loc);
        notify_diagnostics_consumers();
    }

    void did_change_watched_files(const std::vector<utils::resource::resource_location>& paths)
    {
        for (const auto& path : paths)
        {
            workspaces::workspace& ws = ws_path_match(path.get_uri());
            ws.did_change_watched_files(path);
        }
        notify_diagnostics_consumers();
    }

    void register_diagnostics_consumer(diagnostics_consumer* consumer) { diag_consumers_.push_back(consumer); }
    void unregister_diagnostics_consumer(diagnostics_consumer* consumer)
    {
        diag_consumers_.erase(
            std::remove(diag_consumers_.begin(), diag_consumers_.end(), consumer), diag_consumers_.end());
    }

    void register_parsing_metadata_consumer(parsing_metadata_consumer* consumer)
    {
        parsing_metadata_consumers_.push_back(consumer);
    }

    void unregister_parsing_metadata_consumer(parsing_metadata_consumer* consumer)
    {
        auto& pmc = parsing_metadata_consumers_;
        pmc.erase(std::remove(pmc.begin(), pmc.end(), consumer), pmc.end());
    }

    void set_message_consumer(message_consumer* consumer)
    {
        message_consumer_ = consumer;
        implicit_workspace_.set_message_consumer(consumer);
        for (auto& wks : workspaces_)
            wks.second.set_message_consumer(consumer);
    }

    location definition_result;
    position_uri definition(const std::string& document_uri, const position pos)
    {
        auto doc_loc = utils::resource::resource_location(document_uri);

        if (cancel_ && *cancel_)
        {
            definition_result = { pos, doc_loc };
            return position_uri(definition_result);
        }

        definition_result = ws_path_match(document_uri).definition(doc_loc, pos);

        return position_uri(definition_result);
    }

    location_list references_result;
    position_uri_list references(const std::string& document_uri, const position pos)
    {
        if (cancel_ && *cancel_)
            return {};

        references_result =
            ws_path_match(document_uri).references(utils::resource::resource_location(document_uri), pos);

        return { references_result.data(), references_result.size() };
    }

    std::string hover_result;
    std::string_view hover(const std::string& document_uri, const position pos)
    {
        if (cancel_ && *cancel_)
            return "";

        hover_result = ws_path_match(document_uri).hover(utils::resource::resource_location(document_uri), pos);

        return hover_result;
    }


    lsp::completion_list_s completion_result;
    completion_list completion(const std::string& document_uri,
        const position pos,
        const char trigger_char,
        completion_trigger_kind trigger_kind)
    {
        if (cancel_ && *cancel_)
            return completion_list { nullptr, 0 };

        completion_result =
            ws_path_match(document_uri)
                .completion(utils::resource::resource_location(document_uri), pos, trigger_char, trigger_kind);

        return completion_list(completion_result.data(), completion_result.size());
    }

    lsp::document_symbol_list_s document_symbol_result;
    document_symbol_list document_symbol(const std::string& document_uri, long long limit)
    {
        if (cancel_ && *cancel_)
            return document_symbol_list { nullptr, 0 };

        document_symbol_result =
            ws_path_match(document_uri).document_symbol(utils::resource::resource_location(document_uri), limit);

        return document_symbol_list(document_symbol_result.data(), document_symbol_result.size());
    }

    lib_config global_config_;
    workspaces::workspace::shared_json m_global_settings =
        std::make_shared<const nlohmann::json>(nlohmann::json::object());
    void configuration_changed(const lib_config& new_config, std::shared_ptr<const nlohmann::json> global_settings)
    {
        global_config_ = new_config;
        m_global_settings.store(std::move(global_settings));

        bool updated = false;
        for (auto& [_, ws] : workspaces_)
            updated |= ws.settings_updated();

        if (updated)
            notify_diagnostics_consumers();
    }

    std::vector<token_info> empty_tokens;
    const std::vector<token_info>& semantic_tokens(const char* document_uri)
    {
        if (cancel_ && *cancel_)
            return empty_tokens;

        utils::resource::resource_location doc(document_uri);

        auto file = file_manager_.find(doc);
        if (dynamic_cast<workspaces::processor_file*>(file.get()) != nullptr)
            return file_manager_.find_processor_file(doc)->get_hl_info();

        return empty_tokens;
    }

    continuous_sequence<char> get_virtual_file_content(unsigned long long id) const
    {
        return make_continuous_sequence(file_manager_.get_virtual_file(id));
    }

private:
    void collect_diags() const override
    {
        collect_diags_from_child(file_manager_);

        for (auto& it : workspaces_)
            collect_diags_from_child(it.second);
    }

    // returns implicit workspace, if the file does not belong to any workspace
    workspaces::workspace& ws_path_match(const std::string& document_uri)
    {
        size_t max = 0;
        workspaces::workspace* max_ws = nullptr;
        for (auto& ws : workspaces_)
        {
            size_t match = prefix_match(document_uri, ws.second.uri());
            if (match > max && match >= ws.first.size())
            {
                max = match;
                max_ws = &ws.second;
            }
        }
        if (max_ws != nullptr)
            return *max_ws;
        else if (document_uri.starts_with("file:") || document_uri.starts_with("untitled:"))
            return implicit_workspace_;
        else
            return quiet_implicit_workspace_;
    }

    void notify_diagnostics_consumers() const
    {
        diags().clear();
        collect_diags();
        diagnostic_list l(diags().data(), diags().size());
        for (auto consumer : diag_consumers_)
        {
            consumer->consume_diagnostics(l);
        }
    }

    void notify_performance_consumers(
        const utils::resource::resource_location& document_uri, workspace_file_info ws_file_info) const
    {
        auto file = file_manager_.find(document_uri);
        auto proc_file = dynamic_cast<workspaces::processor_file*>(file.get());
        if (proc_file)
        {
            const auto& metrics = proc_file->get_metrics();
            for (auto consumer : parsing_metadata_consumers_)
            {
                consumer->consume_parsing_metadata({ metrics, ws_file_info });
            }
        }
    }

    static size_t prefix_match(const std::string& first, const std::string& second)
    {
        auto [f, s] = std::mismatch(first.begin(), first.end(), second.begin(), second.end());
        return static_cast<size_t>(std::min(f - first.begin(), s - second.begin()));
    }

    std::atomic<bool>* cancel_;
    workspaces::file_manager_impl file_manager_;
    std::unordered_map<std::string, workspaces::workspace> workspaces_;
    workspaces::workspace implicit_workspace_;
    workspaces::workspace quiet_implicit_workspace_;

    std::vector<diagnostics_consumer*> diag_consumers_;
    std::vector<parsing_metadata_consumer*> parsing_metadata_consumers_;
    message_consumer* message_consumer_ = nullptr;
};
} // namespace hlasm_plugin::parser_library

#endif // !HLASMPLUGIN_PARSERLIBRARY_WORKSPACE_MANAGER_IMPL_H
