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
#include <charconv>
#include <chrono>
#include <deque>
#include <functional>
#include <limits>
#include <optional>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "lsp/completion_item.h"
#include "lsp/document_symbol_item.h"
#include "nlohmann/json.hpp"
#include "protocol.h"
#include "utils/scope_exit.h"
#include "workspace_manager.h"
#include "workspace_manager_response.h"
#include "workspaces/file_manager_impl.h"
#include "workspaces/workspace.h"

namespace hlasm_plugin::parser_library {

// Implementation of workspace manager (Implementation part of the pimpl idiom)
// Holds workspaces, file manager and macro tracer and handles LSP and DAP
// notifications and requests.

class workspace_manager::impl final : public diagnosable_impl
{
    static constexpr lib_config supress_all { 0 };
    using resource_location = utils::resource::resource_location;

    struct opened_workspace
    {
        opened_workspace(const resource_location& location,
            const std::string& name,
            workspaces::file_manager& file_manager,
            const lib_config& global_config)
            : ws(location, name, file_manager, global_config, settings)
        {}
        opened_workspace(workspaces::file_manager& file_manager, const lib_config& global_config)
            : ws(file_manager, global_config, settings)
        {}
        workspaces::shared_json settings = std::make_shared<const nlohmann::json>(nlohmann::json::object());
        workspaces::workspace ws;
    };

public:
    impl()
        : implicit_workspace_(file_manager_, global_config_)
        , quiet_implicit_workspace_(file_manager_, supress_all)
    {}
    impl(const impl&) = delete;
    impl& operator=(const impl&) = delete;

    impl(impl&&) = delete;
    impl& operator=(impl&&) = delete;

    static auto& ws_path_match(auto& self, std::string_view document_uri)
    {
        if (auto hlasm_id = extract_hlasm_id(document_uri); hlasm_id.has_value())
        {
            if (auto related_ws = self.file_manager_.get_virtual_file_workspace(hlasm_id.value()); !related_ws.empty())
                for (auto& [_, ows] : self.workspaces_)
                    if (ows.ws.uri() == related_ws.get_uri())
                        return ows;
        }

        size_t max = 0;
        decltype(&self.workspaces_.begin()->second) max_ows = nullptr;
        for (auto& [name, ows] : self.workspaces_)
        {
            size_t match = prefix_match(document_uri, ows.ws.uri());
            if (match > max && match >= name.size())
            {
                max = match;
                max_ows = &ows;
            }
        }
        if (max_ows != nullptr)
            return *max_ows;
        else if (document_uri.starts_with("file:") || document_uri.starts_with("untitled:"))
            return self.implicit_workspace_;
        else
            return self.quiet_implicit_workspace_;
    }

    // returns implicit workspace, if the file does not belong to any workspace
    auto& ws_path_match(std::string_view document_uri) { return ws_path_match(*this, document_uri); }
    auto& ws_path_match(std::string_view document_uri) const { return ws_path_match(*this, document_uri); }

    size_t get_workspaces(ws_id* workspaces, size_t max_size)
    {
        size_t size = 0;

        for (auto it = workspaces_.begin(); size < max_size && it != workspaces_.end(); ++size, ++it)
        {
            workspaces[size] = &it->second.ws;
        }
        return size;
    }

    size_t get_workspaces_count() const { return workspaces_.size(); }

    enum class work_item_type
    {
        workspace_open,
        settings_change,
        file_change,
        query,
    };

    struct work_item
    {
        unsigned long long id;

        opened_workspace* ows;

        std::function<void(bool)> action; // true on workspace removal
        std::function<bool()> validator; // maybe empty

        work_item_type request_type;

        std::vector<std::pair<unsigned long long, std::function<void()>>> pending_requests;

        bool workspace_removed = false;

        bool is_valid() const { return !validator || validator(); }
        bool remove_pending_request(unsigned long long rid)
        {
            auto it = std::find_if(
                pending_requests.begin(), pending_requests.end(), [rid](const auto& e) { return e.first == rid; });

            if (it == pending_requests.end())
                return false;

            pending_requests.erase(it);
            return true;
        }
        void cancel_pending_requests()
        {
            for (const auto& [_, h] : pending_requests)
                h();
            pending_requests.clear();
        }
    };
    std::deque<work_item> m_work_queue;

    work_item* find_work_item(unsigned long long id)
    {
        for (auto& wi : m_work_queue)
            if (wi.id == id)
                return &wi;
        return nullptr;
    }

    void add_workspace(std::string name, std::string uri)
    {
        auto& ows =
            workspaces_.try_emplace(name, resource_location(std::move(uri)), name, file_manager_, global_config_)
                .first->second;
        ows.ws.set_message_consumer(message_consumer_);

        auto& new_workspace = m_work_queue.emplace_back(work_item {
            next_unique_id(),
            &ows,
            [this, &ws = ows.ws](bool workspace_removed) {
                if (workspace_removed)
                    return;
                ws.open();

                notify_diagnostics_consumers();
            },
            {},
            work_item_type::workspace_open,
        });

        attach_configuration_request(new_workspace);
    }

    bool attach_configuration_request(work_item& wi)
    {
        if (!requests_)
            return false;

        auto configuration_request = next_unique_id();

        struct open_workspace_t
        {
            unsigned long long work_item_id;
            unsigned long long request_id;
            impl* self;

            auto* get_wi() const
            {
                auto* wi = self->find_work_item(work_item_id);
                return wi && wi->remove_pending_request(request_id) ? wi : nullptr;
            }

            void provide(sequence<char> json_text) const
            {
                if (auto* wi = get_wi())
                    wi->ows->settings =
                        std::make_shared<const nlohmann::json>(nlohmann::json::parse(std::string_view(json_text)));
            }

            void error(int, const char*) const
            {
                if (auto* wi = get_wi())
                    wi->ows->settings = std::make_shared<const nlohmann::json>();
            }
        };

        auto [resp, _] = make_workspace_manager_response(open_workspace_t { wi.id, configuration_request, this });

        wi.pending_requests.emplace_back(configuration_request, [resp = resp]() { resp.invalidate(); });

        requests_->request_workspace_configuration(wi.ows->ws.uri().c_str(), std::move(resp));

        return true;
    }

    ws_id find_workspace(const std::string& document_uri) { return &ws_path_match(document_uri).ws; }
    void remove_workspace(std::string uri)
    {
        auto it = workspaces_.find(uri);
        if (it == workspaces_.end())
            return; // erase does no action, if the key does not exist

        auto* ows = &it->second;

        for (auto& e : m_work_queue)
        {
            if (e.ows != ows)
                continue;

            e.workspace_removed = true;
            e.cancel_pending_requests();
        }
        if (m_active_task.ows == ows)
            m_active_task = {};

        workspaces_.erase(it);
        notify_diagnostics_consumers();
    }

    struct
    {
        utils::value_task<workspaces::parse_file_result> task;
        opened_workspace* ows = nullptr;
        std::chrono::steady_clock::time_point start_time;

        bool valid() const noexcept { return task.valid(); }
    } m_active_task;

    bool run_active_task(const std::atomic<unsigned char>* yield_indicator)
    {
        const auto& [task, ows, start] = m_active_task;
        task.resume(yield_indicator);
        if (!task.done())
            return false;

        std::chrono::duration<double> duration = std::chrono::steady_clock::now() - start;

        const auto& [url, metadata, perf_metrics, errors, warnings] = task.value();

        if (perf_metrics)
        {
            parsing_metadata data { perf_metrics.value(), metadata, errors, warnings };
            for (auto consumer : parsing_metadata_consumers_)
                consumer->consume_parsing_metadata(sequence<char>(url.get_uri()), duration.count(), data);
        }

        m_active_task = {};

        return true;
    }

    std::pair<bool, bool> run_parse_loop(opened_workspace& ows, const std::atomic<unsigned char>* yield_indicator)
    {
        auto result = std::pair<bool, bool>(false, true);
        while (true)
        {
            auto task = ows.ws.parse_file();
            if (!task.valid())
                break;

            m_active_task = { std::move(task), &ows, std::chrono::steady_clock::now() };

            if (!run_active_task(yield_indicator))
                return result;

            result.first = true;
        }
        result.second = false;
        return result;
    }

    bool run_parse_loop(const std::atomic<unsigned char>* yield_indicator, bool previous_progress)
    {
        constexpr auto combine = [](std::pair<bool, bool>& r, std::pair<bool, bool> n) {
            r.first |= n.first;
            r.second |= n.second;
        };
        auto result = run_parse_loop(implicit_workspace_, yield_indicator);
        combine(result, run_parse_loop(quiet_implicit_workspace_, yield_indicator));
        for (auto& [_, ows] : workspaces_)
            combine(result, run_parse_loop(ows, yield_indicator));

        const auto& [progress, stuff_to_do] = result;

        if (progress || previous_progress)
            notify_diagnostics_consumers();

        return stuff_to_do;
    }

    static constexpr bool parsing_must_be_done(const work_item& item)
    {
        return item.request_type == work_item_type::query;
    }

    bool idle_handler(const std::atomic<unsigned char>* yield_indicator)
    {
        bool parsing_done = false;
        bool finished_inflight_task = false;
        while (true)
        {
            if (!m_work_queue.empty())
            {
                auto& item = m_work_queue.front();
                if (!item.pending_requests.empty() && item.is_valid())
                    return false;
                if (item.workspace_removed || !item.is_valid() || parsing_done || !parsing_must_be_done(item))
                {
                    utils::scope_exit pop_front([this]() noexcept { m_work_queue.pop_front(); });

                    item.cancel_pending_requests();

                    if (item.request_type == work_item_type::file_change)
                    {
                        parsing_done = false;
                        m_active_task = {};
                    }

                    item.action(item.workspace_removed);

                    continue;
                }
            }
            else if (parsing_done)
                return false;

            if (m_active_task.valid())
            {
                if (!run_active_task(yield_indicator))
                    return true;
                finished_inflight_task = true;
            }

            if (run_parse_loop(yield_indicator, std::exchange(finished_inflight_task, false)))
                return true;

            parsing_done = true;
        }
    }

    void did_open_file(const utils::resource::resource_location& document_loc, version_t version, std::string text)
    {
        auto& ows = ws_path_match(document_loc.get_uri());
        m_work_queue.emplace_back(work_item {
            next_unique_id(),
            &ows,
            [this, document_loc, version, text = std::move(text), &ws = ows.ws](bool workspace_removed) mutable {
                auto file_changed = file_manager_.did_open_file(document_loc, version, std::move(text));
                if (!workspace_removed)
                    ws.did_open_file(document_loc, file_changed);
            },
            {},
            work_item_type::file_change,
        });
    }

    void did_change_file(const utils::resource::resource_location& document_loc,
        version_t version,
        const document_change* changes,
        size_t ch_size)
    {
        auto& ows = ws_path_match(document_loc.get_uri());

        struct captured_change
        {
            bool whole;
            range change_range;
            std::string text;
        };

        std::vector<captured_change> captured_changes;
        captured_changes.reserve(ch_size);
        std::transform(
            changes, changes + ch_size, std::back_inserter(captured_changes), [](const document_change& change) {
                return captured_change {
                    .whole = change.whole,
                    .change_range = change.change_range,
                    .text = std::string(change.text, change.text_length),
                };
            });

        m_work_queue.emplace_back(work_item {
            next_unique_id(),
            &ows,
            [this, document_loc, version, captured_changes = std::move(captured_changes), &ws = ows.ws](
                bool workspace_removed) {
                std::vector<document_change> list;
                list.reserve(captured_changes.size());
                std::transform(captured_changes.begin(),
                    captured_changes.end(),
                    std::back_inserter(list),
                    [](const captured_change& cc) {
                        return cc.whole ? document_change(cc.text.data(), cc.text.size())
                                        : document_change(cc.change_range, cc.text.data(), cc.text.size());
                    });
                file_manager_.did_change_file(document_loc, version, list.data(), list.size());
                if (!workspace_removed)
                    ws.did_change_file(document_loc, list.data(), list.size());
            },
            {},
            work_item_type::file_change,
        });
    }

    void did_close_file(const utils::resource::resource_location& document_loc)
    {
        auto& ows = ws_path_match(document_loc.get_uri());

        m_work_queue.emplace_back(work_item {
            next_unique_id(),
            &ows,
            [this, document_loc, &ws = ows.ws](bool workspace_removed) {
                file_manager_.did_close_file(document_loc);
                if (!workspace_removed)
                    ws.did_close_file(document_loc);
            },
            {},
            work_item_type::file_change,
        });
    }

    void did_change_watched_files(std::vector<utils::resource::resource_location> paths)
    {
        std::unordered_map<opened_workspace*, std::vector<resource_location>> paths_for_ws;
        for (auto& path : paths)
            paths_for_ws[&ws_path_match(path.get_uri())].push_back(std::move(path));

        for (auto& [ows, path_list] : paths_for_ws)
            m_work_queue.emplace_back(work_item {
                next_unique_id(),
                ows,
                [path_list = std::move(path_list), &ws = ows->ws](bool workspace_removed) {
                    if (!workspace_removed)
                        ws.did_change_watched_files(path_list);
                },
                {},
                work_item_type::file_change,
            });
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
        implicit_workspace_.ws.set_message_consumer(consumer);
        for (auto& wks : workspaces_)
            wks.second.ws.set_message_consumer(consumer);
    }

    void set_request_interface(workspace_manager_requests* requests) { requests_ = requests; }

    struct
    {
        int code;
        const char* msg;
    } static constexpr request_cancelled { -32800, "Canceled" }, request_failed { -32803, "Unknown reason" },
        removing_workspace { -32803, "Workspace removal in progress" };

    static auto response_handle(auto r, auto f)
    {
        return [r = std::move(r), f = std::move(f)](bool workspace_removed) {
            if (!r.valid())
                r.error(request_cancelled.code, request_cancelled.msg);
            else if (workspace_removed)
                r.error(removing_workspace.code, removing_workspace.msg);
            else
                f(r);
        };
    }

    void definition(const std::string& document_uri, position pos, workspace_manager_response<position_uri> r)
    {
        auto& ows = ws_path_match(document_uri);

        m_work_queue.emplace_back(work_item {
            next_unique_id(),
            &ows,
            response_handle(r,
                [&ws = ows.ws, doc_loc = resource_location(document_uri), pos](
                    const workspace_manager_response<position_uri>& resp) {
                    resp.provide(position_uri(ws.definition(doc_loc, pos)));
                }),
            [r]() { return r.valid(); },
            work_item_type::query,
        });
    }

    void references(const std::string& document_uri, position pos, workspace_manager_response<position_uri_list> r)
    {
        auto& ows = ws_path_match(document_uri);

        m_work_queue.emplace_back(work_item {
            next_unique_id(),
            &ows,
            response_handle(r,
                [&ws = ows.ws, doc_loc = resource_location(document_uri), pos](
                    const workspace_manager_response<position_uri_list>& resp) {
                    auto references_result = ws.references(doc_loc, pos);
                    resp.provide({ references_result.data(), references_result.size() });
                }),
            [r]() { return r.valid(); },
            work_item_type::query,
        });
    }

    void hover(const std::string& document_uri, position pos, workspace_manager_response<sequence<char>> r)
    {
        auto& ows = ws_path_match(document_uri);

        m_work_queue.emplace_back(work_item {
            next_unique_id(),
            &ows,
            response_handle(r,
                [&ws = ows.ws, doc_loc = resource_location(document_uri), pos](
                    const workspace_manager_response<sequence<char>>& resp) {
                    auto hover_result = ws.hover(doc_loc, pos);
                    resp.provide(sequence<char>(hover_result));
                }),
            [r]() { return r.valid(); },
            work_item_type::query,
        });
    }


    void completion(const std::string& document_uri,
        position pos,
        const char trigger_char,
        completion_trigger_kind trigger_kind,
        workspace_manager_response<completion_list> r)
    {
        auto& ows = ws_path_match(document_uri);

        m_work_queue.emplace_back(work_item {
            next_unique_id(),
            &ows,
            response_handle(r,
                [&ws = ows.ws, doc_loc = resource_location(document_uri), pos, trigger_char, trigger_kind](
                    const workspace_manager_response<completion_list>& resp) {
                    auto completion_result = ws.completion(doc_loc, pos, trigger_char, trigger_kind);
                    resp.provide(completion_list(completion_result.data(), completion_result.size()));
                }),
            [r]() { return r.valid(); },
            work_item_type::query,
        });
    }

    void document_symbol(
        const std::string& document_uri, long long limit, workspace_manager_response<document_symbol_list> r)
    {
        auto& ows = ws_path_match(document_uri);

        m_work_queue.emplace_back(work_item {
            next_unique_id(),
            &ows,
            response_handle(r,
                [&ws = ows.ws, doc_loc = resource_location(document_uri), limit](
                    const workspace_manager_response<document_symbol_list>& resp) {
                    auto document_symbol_result = ws.document_symbol(doc_loc, limit);
                    resp.provide(document_symbol_list(document_symbol_result.data(), document_symbol_result.size()));
                }),
            [r]() { return r.valid(); },
            work_item_type::query,
        });
    }

    lib_config global_config_;
    void configuration_changed(const lib_config& new_config)
    {
        // TODO: should this action be also performed IN ORDER?

        global_config_ = new_config;
        if (implicit_workspace_.ws.settings_updated())
            notify_diagnostics_consumers();

        if (!requests_)
            return;

        for (auto& [_, ows] : workspaces_)
        {
            auto& refersh_settings = m_work_queue.emplace_back(work_item {
                next_unique_id(),
                &ows,
                [this, &ws = ows.ws](bool workspace_removed) {
                    if (workspace_removed)
                        return;
                    if (ws.settings_updated())
                        notify_diagnostics_consumers();
                },
                {},
                work_item_type::settings_change,
            });

            attach_configuration_request(refersh_settings);
        }
    }

    void semantic_tokens(const char* document_uri, workspace_manager_response<continuous_sequence<token_info>> r)
    {
        auto& ows = ws_path_match(document_uri);

        m_work_queue.emplace_back(work_item {
            next_unique_id(),
            &ows,
            response_handle(r,
                [&ws = ows.ws, doc_loc = resource_location(document_uri)](
                    const workspace_manager_response<continuous_sequence<token_info>>& resp) {
                    resp.provide(make_continuous_sequence(ws.semantic_tokens(doc_loc)));
                }),
            [r]() { return r.valid(); },
            work_item_type::query,
        });
    }

    continuous_sequence<char> get_virtual_file_content(unsigned long long id) const
    {
        return make_continuous_sequence(file_manager_.get_virtual_file(id));
    }

    void make_opcode_suggestion(const std::string& document_uri,
        const std::string& opcode,
        bool extended,
        workspace_manager_response<continuous_sequence<opcode_suggestion>> r)
    {
        // performed out of order
        auto suggestions =
            ws_path_match(document_uri)
                .ws.make_opcode_suggestion(utils::resource::resource_location(document_uri), opcode, extended);

        std::vector<opcode_suggestion> res;

        for (auto&& [suggestion, distance] : suggestions)
            res.emplace_back(opcode_suggestion { make_continuous_sequence(std::move(suggestion)), distance });

        r.provide(make_continuous_sequence(std::move(res)));
    }

private:
    void collect_diags() const override
    {
        collect_diags_from_child(implicit_workspace_.ws);
        collect_diags_from_child(quiet_implicit_workspace_.ws);
        for (auto& it : workspaces_)
            collect_diags_from_child(it.second.ws);
    }

    static std::optional<unsigned long long> extract_hlasm_id(std::string_view uri)
    {
        static constexpr std::string_view prefix = "hlasm://";
        if (!uri.starts_with(prefix))
            return std::nullopt;
        uri.remove_prefix(prefix.size());
        if (auto slash = uri.find('/'); slash == std::string_view::npos)
            return std::nullopt;
        else
            uri = uri.substr(0, slash);

        unsigned long long result = 0;

        auto [p, err] = std::from_chars(uri.data(), uri.data() + uri.size(), result);
        if (err != std::errc() || p != uri.data() + uri.size())
            return std::nullopt;
        else
            return result;
    }

    void notify_diagnostics_consumers()
    {
        diags().clear();
        collect_diags();

        fade_messages_.clear();
        implicit_workspace_.ws.retrieve_fade_messages(fade_messages_);
        quiet_implicit_workspace_.ws.retrieve_fade_messages(fade_messages_);
        for (const auto& [_, ows] : workspaces_)
            ows.ws.retrieve_fade_messages(fade_messages_);

        for (auto consumer : diag_consumers_)
            consumer->consume_diagnostics(diagnostic_list(diags().data(), diags().size()),
                fade_message_list(fade_messages_.data(), fade_messages_.size()));
    }

    static size_t prefix_match(std::string_view first, std::string_view second)
    {
        auto [f, s] = std::mismatch(first.begin(), first.end(), second.begin(), second.end());
        return static_cast<size_t>(std::min(f - first.begin(), s - second.begin()));
    }

    unsigned long long next_unique_id() { return ++unique_id_sequence; }

    workspaces::file_manager_impl file_manager_;

    std::unordered_map<std::string, opened_workspace> workspaces_;
    opened_workspace implicit_workspace_;
    opened_workspace quiet_implicit_workspace_;

    std::vector<diagnostics_consumer*> diag_consumers_;
    std::vector<parsing_metadata_consumer*> parsing_metadata_consumers_;
    message_consumer* message_consumer_ = nullptr;
    workspace_manager_requests* requests_ = nullptr;
    std::vector<fade_message_s> fade_messages_;
    unsigned long long unique_id_sequence = 0;
};
} // namespace hlasm_plugin::parser_library

#endif // !HLASMPLUGIN_PARSERLIBRARY_WORKSPACE_MANAGER_IMPL_H
