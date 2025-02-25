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

// Pimpl implementations of workspace manager. Also converts C-like parameters
// into C++ ones.

#include "workspace_manager.h"

#include <algorithm>
#include <atomic>
#include <charconv>
#include <chrono>
#include <deque>
#include <functional>
#include <limits>
#include <optional>
#include <ranges>
#include <set>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "completion_item.h"
#include "debugging/debugger_configuration.h"
#include "document_symbol_item.h"
#include "external_configuration_requests.h"
#include "fade_messages.h"
#include "folding_range.h"
#include "nlohmann/json.hpp"
#include "protocol.h"
#include "utils/async_busy_wait.h"
#include "utils/content_loader.h"
#include "utils/encoding.h"
#include "utils/error_codes.h"
#include "utils/path_conversions.h"
#include "utils/platform.h"
#include "utils/projectors.h"
#include "utils/resource_location.h"
#include "utils/scope_exit.h"
#include "utils/task.h"
#include "workspace_manager.h"
#include "workspace_manager_external_file_requests.h"
#include "workspace_manager_response.h"
#include "workspaces/configuration_provider.h"
#include "workspaces/file_manager_impl.h"
#include "workspaces/workspace.h"
#include "workspaces/workspace_configuration.h"

namespace hlasm_plugin::parser_library {

// Implementation of workspace manager (Implementation part of the pimpl idiom)
// Holds workspaces, file manager and macro tracer and handles LSP and DAP
// notifications and requests.

class workspace_manager_impl final : public workspace_manager,
                                     debugger_configuration_provider,
                                     workspaces::external_file_reader,
                                     external_configuration_requests,
                                     workspaces::configuration_provider
{
    static constexpr lib_config supress_all { 0 };
    using resource_location = utils::resource::resource_location;

    bool m_include_advisory_cfg_diags = false;

    struct opened_workspace
    {
        opened_workspace(const resource_location& location,
            std::string_view,
            workspaces::file_manager& file_manager,
            const lib_config& global_config,
            external_configuration_requests* ecr)
            : config(file_manager, location, settings, global_config, ecr)
        {}
        opened_workspace(workspaces::file_manager& file_manager,
            const lib_config& global_config,
            external_configuration_requests* ecr)
            : config(file_manager, utils::resource::resource_location(), settings, global_config, ecr)
        {}
        workspaces::shared_json settings = std::make_shared<const nlohmann::json>(nlohmann::json::object());
        workspaces::workspace_configuration config;
        bool logically_deleted = false;
    };

    static constexpr std::string_view hlasm_external_scheme = "hlasm-external:";
    static constexpr std::string_view hlasm_internal_scheme = "hlasm:";
    static constexpr std::string_view default_allowed_schemes[] = {
        // e4e integration
        "e4e-change-lvl:",
        "e4e-element:",
        "e4e-listing:",
        "e4e-read-only-cached-element:",
        "e4e-readonly-file:",
        "e4e-readonly-generic-report:",
        "e4e-readonly-report:",

        "file:",
        hlasm_external_scheme,
        hlasm_internal_scheme,

        // e4e integration
        "ndvr:",

        "untitled:",
        "vscode-test-web:",
        "vscode-vfs:",

        "zowe-ds:",
        "zowe-uss:",
    };
    static_assert(std::ranges::is_sorted(default_allowed_schemes));
    std::vector<std::string> allowed_schemes = {
        std::begin(default_allowed_schemes),
        std::end(default_allowed_schemes),
    };
    bool allowed_scheme(std::string_view uri) const noexcept
    {
        return std::ranges::binary_search(allowed_schemes, extract_scheme(uri));
    }
    bool allowed_scheme(const resource_location& uri) const noexcept { return allowed_scheme(uri.get_uri()); }

    static resource_location normalized_uri(std::string_view uri) { return resource_location(uri).lexically_normal(); }
    auto ws_path_match(const resource_location& normalized_uri)
    {
        size_t max = 0;
        opened_workspace* max_ows = &m_implicit_workspace;
        for (auto& [ws_uri, ows] : m_workspaces)
        {
            if (ows.logically_deleted)
                continue;
            size_t match = prefix_match(normalized_uri.get_uri(), ws_uri.get_uri());
            if (match > max && match >= ws_uri.get_uri().size())
            {
                max = match;
                max_ows = &ows;
            }
        }

        return max_ows;
    }

    enum class work_item_type
    {
        workspace_open,
        workspace_close,
        settings_change,
        file_change,
        query,
        dc_request,
    };

    struct work_item
    {
        unsigned long long id;

        std::variant<std::function<void()>, std::function<void(bool)>, utils::task, std::function<utils::task()>>
            action;
        std::function<bool()> validator; // maybe empty

        work_item_type request_type;

        std::vector<std::pair<unsigned long long, std::function<void()>>> pending_requests;

        bool workspace_removed = false;

        bool is_valid() const { return !validator || validator(); }
        bool remove_pending_request(unsigned long long rid)
        {
            auto it = std::ranges::find(pending_requests, rid, utils::first_element);

            if (it == pending_requests.end())
                return false;

            pending_requests.erase(it);
            return true;
        }
        void cancel_pending_requests() noexcept
        {
            for (const auto& [_, h] : pending_requests)
                h();
            pending_requests.clear();
        }

        bool is_task() const { return action.index() == 2 || action.index() == 3; }

        bool perform_action()
        {
            switch (action.index())
            {
                case 0:
                    if (!workspace_removed)
                        std::get<0>(action)();
                    return true;

                case 1:
                    std::get<1>(action)(workspace_removed);
                    return true;

                case 3:
                    if (workspace_removed)
                        return true;

                    if (auto t = std::get<3>(action)(); !t.valid())
                        return true;
                    else
                        action = std::move(t);
                    [[fallthrough]];

                case 2: {
                    if (workspace_removed)
                        return true;

                    const auto& task = std::get<2>(action);
                    if (!task.done())
                        task.resume(nullptr);

                    return task.done();
                }
                default:
                    return true;
            }
        }
    };

    work_item* find_work_item(unsigned long long id)
    {
        for (auto& wi : m_work_queue)
            if (wi.id == id)
                return &wi;
        return nullptr;
    }

    bool attach_configuration_request(std::string_view uri, work_item& wi, opened_workspace* ows)
    {
        if (!m_requests)
            return false;

        auto configuration_request = next_unique_id();

        struct open_workspace_t
        {
            unsigned long long work_item_id;
            unsigned long long request_id;
            workspace_manager_impl* self;
            opened_workspace* ows;

            void resolve_wi() const
            {
                if (auto* wi = self->find_work_item(work_item_id))
                    wi->remove_pending_request(request_id);
            }

            void provide(std::string_view json_text) const
            {
                resolve_wi();
                ows->settings = std::make_shared<const nlohmann::json>(nlohmann::json::parse(json_text));
            }

            void error(int, const char*) const noexcept
            {
                static const auto empty = std::make_shared<const nlohmann::json>();
                resolve_wi();
                ows->settings = empty;
            }
        };

        auto [resp, _] = make_workspace_manager_response(open_workspace_t { wi.id, configuration_request, this, ows });

        wi.pending_requests.emplace_back(configuration_request, [resp = resp]() noexcept { resp.invalidate(); });

        m_requests->request_workspace_configuration(uri, std::move(resp));

        return true;
    }

    bool run_active_task(const std::atomic<unsigned char>* yield_indicator)
    {
        const auto& [task, start] = m_active_task;
        task.resume(yield_indicator);
        if (!task.done())
            return false;

        std::chrono::duration<double> duration = std::chrono::steady_clock::now() - start;

        const auto& [url, metadata, perf_metrics, errors, warnings, outputs_changed] = task.value();

        if (perf_metrics)
        {
            parsing_metadata data { perf_metrics.value(), metadata, errors, warnings };
            for (auto consumer : m_parsing_metadata_consumers)
                consumer->consume_parsing_metadata(url.get_uri(), duration.count(), data);
        }
        if (outputs_changed)
        {
            for (auto consumer : m_parsing_metadata_consumers)
                consumer->outputs_changed(url.get_uri());
        }

        m_active_task = {};

        return true;
    }

    std::pair<bool, bool> run_parse_loop(const std::atomic<unsigned char>* yield_indicator)
    {
        auto result = std::pair<bool, bool>(false, true);
        while (true)
        {
            auto task = m_ws.parse_file();
            if (!task.valid())
                break;

            m_active_task = { std::move(task), std::chrono::steady_clock::now() };

            if (!run_active_task(yield_indicator))
                return result;

            result.first = true;
        }
        result.second = false;
        return result;
    }

    bool run_parse_loop(const std::atomic<unsigned char>* yield_indicator, bool previous_progress)
    {
        const auto& [progress, stuff_to_do] = run_parse_loop(yield_indicator);

        if (progress || previous_progress)
            notify_diagnostics_consumers();

        return stuff_to_do;
    }

    static constexpr bool parsing_must_be_done(const work_item& item)
    {
        return item.request_type == work_item_type::query;
    }

    void idle_handler(const std::atomic<unsigned char>* yield_indicator) override
    {
        bool parsing_done = false;
        bool finished_inflight_task = false;
        while (true)
        {
            if (!m_work_queue.empty())
            {
                auto& item = m_work_queue.front();
                if (!item.pending_requests.empty() && item.is_valid())
                    return;
                if (item.is_task() || item.workspace_removed || !item.is_valid() || parsing_done
                    || !parsing_must_be_done(item))
                {
                    bool done = true;
                    utils::scope_exit pop_front([this, &done]() noexcept {
                        if (done)
                        {
                            m_work_queue.front().cancel_pending_requests();
                            m_work_queue.pop_front();
                        }
                    });

                    if (item.request_type == work_item_type::file_change)
                    {
                        parsing_done = false;
                        m_active_task = {};
                    }

                    done = item.perform_action();

                    if (!done)
                        return;

                    continue;
                }
            }
            else if (parsing_done)
                return;

            if (m_active_task.valid())
            {
                if (!run_active_task(yield_indicator))
                    return;
                finished_inflight_task = true;
            }

            if (run_parse_loop(yield_indicator, std::exchange(finished_inflight_task, false)))
                return;

            parsing_done = true;
        }
    }

    void did_open_file(std::string_view document_uri, version_t version, std::string_view text) override
    {
        auto uri = normalized_uri(document_uri);
        auto open_result = std::make_shared<workspaces::file_content_state>();
        m_work_queue.emplace_back(work_item {
            next_unique_id(),
            [this, document_loc = uri, version, text = std::string(text), open_result]() mutable {
                *open_result = m_file_manager.did_open_file(document_loc, version, std::move(text));
            },
            {},
            work_item_type::file_change,
        });
        m_work_queue.emplace_back(work_item {
            next_unique_id(),
            std::function<utils::task()>([this, document_loc = std::move(uri), open_result]() mutable {
                auto ows = ws_path_match(document_loc);
                if (!ows->config.is_configuration_file(document_loc))
                    return m_ws.did_open_file(std::move(document_loc), *open_result);

                return ows->config.parse_configuration_file(std::move(document_loc)).then([this](auto r) {
                    if (r == workspaces::parse_config_file_result::parsed)
                        m_ws.mark_all_opened_files();
                });
            }),
            {},
            work_item_type::file_change,
        });
    }

    void did_change_file(
        std::string_view document_uri, version_t version, std::span<const document_change> changes) override
    {
        auto uri = normalized_uri(document_uri);

        struct captured_change
        {
            bool whole;
            range change_range;
            std::string text;
        };

        std::vector<captured_change> captured_changes;
        captured_changes.reserve(changes.size());
        std::ranges::transform(changes, std::back_inserter(captured_changes), [](const document_change& change) {
            return captured_change {
                .whole = change.whole,
                .change_range = change.change_range,
                .text = std::string(change.text),
            };
        });

        m_work_queue.emplace_back(work_item {
            next_unique_id(),
            [this, document_loc = uri, version, captured_changes = std::move(captured_changes)]() {
                std::vector<document_change> list;
                list.reserve(captured_changes.size());
                std::ranges::transform(captured_changes, std::back_inserter(list), [](const captured_change& cc) {
                    return cc.whole ? document_change(cc.text) : document_change(cc.change_range, cc.text);
                });
                m_file_manager.did_change_file(document_loc, version, list);
            },
            {},
            work_item_type::file_change,
        });

        m_work_queue.emplace_back(work_item {
            next_unique_id(),
            std::function<utils::task()>(
                [this,
                    document_loc = std::move(uri),
                    file_content_status = !changes.empty() ? workspaces::file_content_state::changed_content
                                                           : workspaces::file_content_state::identical]() mutable {
                    auto ows = ws_path_match(document_loc);
                    if (!ows->config.is_configuration_file(document_loc))
                        return m_ws.mark_file_for_parsing(document_loc, file_content_status);

                    return ows->config.parse_configuration_file(std::move(document_loc)).then([this](auto result) {
                        if (result == workspaces::parse_config_file_result::parsed)
                            m_ws.mark_all_opened_files();
                    });
                }),
            {},
            work_item_type::file_change,
        });
    }

    void did_close_file(std::string_view document_uri) override
    {
        auto uri = normalized_uri(document_uri);
        m_work_queue.emplace_back(work_item {
            next_unique_id(),
            [this, document_loc = uri]() { m_file_manager.did_close_file(document_loc); },
            {},
            work_item_type::file_change,
        });
        m_work_queue.emplace_back(work_item {
            next_unique_id(),
            std::function<utils::task()>([this, document_loc = std::move(uri)]() mutable {
                ws_path_match(document_loc)->config.prune_external_processor_groups(document_loc);
                return m_ws.did_close_file(std::move(document_loc)).then([this]() { notify_diagnostics_consumers(); });
            }),
            {},
            work_item_type::file_change,
        });
    }

    utils::value_task<std::optional<std::vector<index_t<workspaces::processor_group, unsigned long long>>>>
    handle_library_refresh(
        std::shared_ptr<std::pair<std::vector<resource_location>, std::vector<workspaces::file_content_state>>>
            paths_for_ws)
    {
        std::optional<std::vector<index_t<workspaces::processor_group, unsigned long long>>> proc_grps;
        const auto updater = [&proc_grps](auto r) {
            if (!r)
                return;
            if (!proc_grps)
            {
                proc_grps = std::move(r);
                return;
            }
            proc_grps->insert(proc_grps->end(), r->begin(), r->end());
        };
        const auto& [paths, changes] = *paths_for_ws;
        std::vector<utils::task> tasks;
        tasks.reserve(1 + m_workspaces.size());
        tasks.emplace_back(m_implicit_workspace.config.refresh_libraries(paths).then(updater));
        for (auto& [_, ows] : m_workspaces)
            tasks.emplace_back(ows.config.refresh_libraries(paths).then(updater));

        co_await utils::task::wait_all(std::move(tasks));

        co_return proc_grps;
    }

    void did_change_watched_files(std::span<const fs_change> fs_changes) override
    {
        auto paths_for_ws =
            std::make_shared<std::pair<std::vector<resource_location>, std::vector<workspaces::file_content_state>>>();
        for (const auto& change : fs_changes)
            paths_for_ws->first.emplace_back(normalized_uri(change.uri));

        m_work_queue.emplace_back(work_item {
            next_unique_id(),
            std::function<utils::task()>([this, paths_for_ws]() -> utils::task {
                std::vector<utils::task> pending_updates;
                auto& [paths, changes] = *paths_for_ws;

                paths_for_ws->second.reserve(paths_for_ws->first.size());
                for (const auto& path : paths)
                {
                    auto update = m_file_manager.update_file(path);
                    auto& change = changes.emplace_back();
                    if (!update.valid())
                        change = workspaces::file_content_state::identical;
                    else if (update.done())
                        change = update.value();
                    else
                        pending_updates.emplace_back(std::move(update).then([&change](auto c) { change = c; }));
                }
                if (pending_updates.empty())
                    return {};
                return utils::task::wait_all(std::move(pending_updates));
            }),
            {},
            work_item_type::file_change,
        });

        m_work_queue.emplace_back(work_item {
            next_unique_id(),
            std::function<utils::task()>([this, paths = std::move(paths_for_ws)]() {
                return handle_library_refresh(paths).then([this, paths](auto r) {
                    auto& [f, c] = *paths;
                    return m_ws.did_change_watched_files(std::move(f), std::move(c), std::move(r));
                });
            }),
            {},
            work_item_type::file_change,
        });
    }

    void register_diagnostics_consumer(diagnostics_consumer* consumer) override
    {
        m_diag_consumers.push_back(consumer);
    }

    void unregister_diagnostics_consumer(diagnostics_consumer* consumer) override
    {
        std::erase(m_diag_consumers, consumer);
    }

    void register_parsing_metadata_consumer(parsing_metadata_consumer* consumer) override
    {
        m_parsing_metadata_consumers.push_back(consumer);
    }

    void unregister_parsing_metadata_consumer(parsing_metadata_consumer* consumer) override
    {
        std::erase(m_parsing_metadata_consumers, consumer);
    }

    void set_message_consumer(message_consumer* consumer) override
    {
        m_message_consumer = consumer;
        m_ws.set_message_consumer(consumer);
    }

    void set_request_interface(workspace_manager_requests* requests) override { m_requests = requests; }

    static auto response_handle(auto r, auto f)
    {
        return [r = std::move(r), f = std::move(f)](bool workspace_removed) {
            if (!r.valid())
                r.error(utils::error::lsp::request_canceled);
            else if (workspace_removed)
                r.error(utils::error::lsp::removing_workspace);
            else
                f(r);
        };
    }

    template<typename R,
        std::invocable<workspace_manager_response<R>, workspaces::workspace&, const resource_location&> A>
    void handle_request(std::string_view document_uri, workspace_manager_response<R> r, A a)
    {
        m_work_queue.emplace_back(work_item {
            next_unique_id(),
            response_handle(r,
                [this, doc_loc = normalized_uri(document_uri), a = std::move(a)](
                    const workspace_manager_response<R>& resp) { std::invoke(a, resp, m_ws, doc_loc); }),
            [r]() { return r.valid(); },
            work_item_type::query,
        });
    }

    void definition(std::string_view document_uri, position pos, workspace_manager_response<const location&> r) override
    {
        handle_request(document_uri, std::move(r), [pos](const auto& resp, auto& ws, const auto& doc_loc) {
            resp.provide(ws.definition(doc_loc, pos));
        });
    }

    void references(
        std::string_view document_uri, position pos, workspace_manager_response<std::span<const location>> r) override
    {
        handle_request(document_uri, std::move(r), [pos](const auto& resp, auto& ws, const auto& doc_loc) {
            resp.provide(ws.references(doc_loc, pos));
        });
    }

    void hover(std::string_view document_uri, position pos, workspace_manager_response<std::string_view> r) override
    {
        handle_request(document_uri, std::move(r), [pos](const auto& resp, auto& ws, const auto& doc_loc) {
            resp.provide(ws.hover(doc_loc, pos));
        });
    }


    void completion(std::string_view document_uri,
        position pos,
        const char trigger_char,
        completion_trigger_kind trigger_kind,
        workspace_manager_response<std::span<const completion_item>> r) override
    {
        handle_request(document_uri,
            std::move(r),
            [pos, trigger_char, trigger_kind](const auto& resp, auto& ws, const auto& doc_loc) {
                resp.provide(ws.completion(doc_loc, pos, trigger_char, trigger_kind));
            });
    }

    void document_symbol(
        std::string_view document_uri, workspace_manager_response<std::span<const document_symbol_item>> r) override
    {
        handle_request(document_uri, std::move(r), [](const auto& resp, auto& ws, const auto& doc_loc) {
            resp.provide(ws.document_symbol(doc_loc));
        });
    }

    utils::task handle_config_update(opened_workspace& ows)
    {
        if (!ows.config.settings_updated())
            co_return;
        const auto res = co_await ows.config.parse_configuration_file();
        if (res == workspaces::parse_config_file_result::parsed)
            m_ws.mark_all_opened_files();
        notify_diagnostics_consumers();
    }

    void change_implicit_group_base(std::string_view uri) override
    {
        m_implicit_workspace.config.change_processor_group_base(normalized_uri(uri));
    }

    void configuration_changed(const lib_config& new_config, std::string_view full_cfg) override
    {
        // TODO: should this action be also performed IN ORDER?

        m_global_config = new_config;

        auto cfg = std::make_shared<const nlohmann::json>(
            full_cfg.empty() ? nlohmann::json() : nlohmann::json::parse(full_cfg));
        m_work_queue.emplace_back(work_item {
            next_unique_id(),
            std::function<utils::task()>([this, cfg = std::move(cfg)]() -> utils::task {
                m_implicit_workspace.settings = cfg;
                return handle_config_update(m_implicit_workspace);
            }),
            {},
            work_item_type::settings_change,
        });

        for (auto& [uri, ows] : m_workspaces)
        {
            auto& refersh_settings = m_work_queue.emplace_back(work_item {
                next_unique_id(),
                std::function<utils::task()>([this, &ows]() -> utils::task { return handle_config_update(ows); }),
                {},
                work_item_type::settings_change,
            });

            attach_configuration_request(uri.get_uri(), refersh_settings, &ows);
        }
    }

    void semantic_tokens(
        std::string_view document_uri, workspace_manager_response<std::span<const token_info>> r) override
    {
        handle_request(document_uri, std::move(r), [](const auto& resp, auto& ws, const auto& doc_loc) {
            resp.provide(ws.semantic_tokens(doc_loc));
        });
    }

    void branch_information(
        std::string_view document_uri, workspace_manager_response<std::span<const branch_info>> r) override
    {
        handle_request(document_uri, std::move(r), [](const auto& resp, auto& ws, const auto& doc_loc) {
            resp.provide(ws.branch_information(doc_loc));
        });
    }

    void folding(std::string_view document_uri, workspace_manager_response<std::span<const folding_range>> r) override
    {
        handle_request(document_uri, std::move(r), [](const auto& resp, auto& ws, const auto& doc_loc) {
            resp.provide(ws.folding(doc_loc));
        });
    }

    std::string get_virtual_file_content(unsigned long long id) const override
    {
        return m_file_manager.get_virtual_file(id);
    }

    void toggle_advisory_configuration_diagnostics() override
    {
        m_include_advisory_cfg_diags ^= true;

        notify_diagnostics_consumers();
    }

    void make_opcode_suggestion(std::string_view document_uri,
        std::string_view opcode,
        bool extended,
        workspace_manager_response<std::span<const opcode_suggestion>> r) override
    {
        // performed out of order
        auto suggestions = m_ws.make_opcode_suggestion(resource_location(document_uri), opcode, extended);

        std::vector<opcode_suggestion> res;

        for (auto&& [suggestion, distance] : suggestions)
            res.emplace_back(opcode_suggestion { std::move(suggestion), distance });

        r.provide(res);
    }

    void collect_diags()
    {
        std::unordered_set<std::string> suppress_files;

        m_diagnostics.clear();
        m_ws.produce_diagnostics(m_diagnostics);
        std::erase_if(m_diagnostics, [this, &suppress_files](const auto& d) {
            const auto& origin = d.related.empty() ? d.file_uri : d.related.back().location.uri;
            return !allowed_scheme(origin) && (suppress_files.emplace(d.file_uri), true);
        });
        for (auto it = suppress_files.begin(); it != suppress_files.end();)
        {
            auto node = suppress_files.extract(it++);
            m_diagnostics.emplace_back(info_SUP(std::move(node.value())));
        }

        const auto usage = m_ws.report_used_configuration_files();

        m_implicit_workspace.config.produce_diagnostics(m_diagnostics, usage, m_include_advisory_cfg_diags);
        for (const auto& [_, ows] : m_workspaces)
            ows.config.produce_diagnostics(m_diagnostics, usage, m_include_advisory_cfg_diags);
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
        collect_diags();

        m_fade_messages.clear();
        m_ws.retrieve_fade_messages(m_fade_messages);

        for (auto consumer : m_diag_consumers)
            consumer->consume_diagnostics(m_diagnostics, m_fade_messages);
    }

    static size_t prefix_match(std::string_view first, std::string_view second)
    {
        auto [f, s] = std::ranges::mismatch(first, second);
        return static_cast<size_t>(std::min(f - first.begin(), s - second.begin()));
    }

    unsigned long long next_unique_id() { return ++m_unique_id_sequence; }

    [[nodiscard]] utils::value_task<std::optional<std::string>> load_text_external(
        const utils::resource::resource_location& document_loc) const
    {
        struct content_t
        {
            std::optional<std::string> result;

            void provide(std::string_view c) { result = std::string(c); }
            void error(int, const char*) noexcept { result.reset(); }
        };
        auto [channel, data] = make_workspace_manager_response(std::in_place_type<content_t>);
        m_external_file_requests->read_external_file(document_loc.get_uri(), channel);

        return utils::async_busy_wait(std::move(channel), &data->result);
    }

    [[nodiscard]] utils::value_task<std::optional<std::string>> load_text(
        const utils::resource::resource_location& document_loc) const override
    {
        if (document_loc.is_local() && !utils::platform::is_web())
            return utils::value_task<std::optional<std::string>>::from_value(utils::resource::load_text(document_loc));

        if (!m_external_file_requests || !m_vscode_extensions || !allowed_scheme(document_loc))
            return utils::value_task<std::optional<std::string>>::from_value(std::nullopt);

        return load_text_external(document_loc);
    }

    [[nodiscard]] utils::value_task<std::pair<std::vector<std::pair<std::string, utils::resource::resource_location>>,
        utils::path::list_directory_rc>>
    list_directory_files_external(const utils::resource::resource_location& directory, bool subdir) const
    {
        using enum utils::path::list_directory_rc;
        struct content_t
        {
            explicit content_t(utils::resource::resource_location dir, bool subdir)
                : dir(std::move(dir))
                , subdir(subdir)
            {}
            utils::resource::resource_location dir;
            bool subdir;
            std::pair<std::vector<std::pair<std::string, utils::resource::resource_location>>,
                utils::path::list_directory_rc>
                result;

            void provide(workspace_manager_external_directory_result c)
            {
                try
                {
                    auto& dirs = result.first;
                    for (auto s : c.member_urls)
                    {
                        auto url = utils::resource::resource_location(s);
                        auto filename = url.filename();
                        if (subdir)
                            url.join("");
                        filename = utils::encoding::percent_decode(filename);
                        if (filename.empty())
                        {
                            result = { {}, other_failure };
                            break;
                        }
                        dirs.emplace_back(std::move(filename), std::move(url).lexically_normal());
                    }
                }
                catch (...)
                {
                    result = { {}, other_failure };
                }
            }
            void error(int err, const char*) noexcept
            {
                if (err > 0)
                    result.second = not_a_directory;
                else if (err == 0)
                    result.second = not_exists;
                else
                    result.second = other_failure;
            }
        };
        auto [channel, data] = make_workspace_manager_response(std::in_place_type<content_t>, directory, subdir);
        m_external_file_requests->read_external_directory(data->dir.get_uri(), channel, subdir);

        return utils::async_busy_wait(std::move(channel), &data->result);
    }

    [[nodiscard]] utils::value_task<std::pair<std::vector<std::pair<std::string, utils::resource::resource_location>>,
        utils::path::list_directory_rc>>
    list_directory_files(const utils::resource::resource_location& directory) const override
    {
        if (directory.is_local() && !utils::platform::is_web())
            return utils::value_task<std::pair<std::vector<std::pair<std::string, utils::resource::resource_location>>,
                utils::path::list_directory_rc>>::from_value(utils::resource::list_directory_files(directory));

        if (!m_external_file_requests || !m_vscode_extensions || !allowed_scheme(directory))
            return utils::value_task<std::pair<std::vector<std::pair<std::string, utils::resource::resource_location>>,
                utils::path::list_directory_rc>>::from_value({ {}, utils::path::list_directory_rc::not_exists });

        return list_directory_files_external(directory, false);
    }

    [[nodiscard]] utils::value_task<std::pair<std::vector<std::pair<std::string, utils::resource::resource_location>>,
        utils::path::list_directory_rc>>
    list_directory_subdirs_and_symlinks(const utils::resource::resource_location& directory) const override
    {
        if (directory.is_local() && !utils::platform::is_web())
            return utils::value_task<std::pair<std::vector<std::pair<std::string, utils::resource::resource_location>>,
                utils::path::list_directory_rc>>::
                from_value(utils::resource::list_directory_subdirs_and_symlinks(directory));

        if (!m_external_file_requests || !m_vscode_extensions || !allowed_scheme(directory))
            return utils::value_task<std::pair<std::vector<std::pair<std::string, utils::resource::resource_location>>,
                utils::path::list_directory_rc>>::from_value({ {}, utils::path::list_directory_rc::not_exists });

        return list_directory_files_external(directory, true);
    }

    std::deque<work_item> m_work_queue;

    struct
    {
        utils::value_task<workspaces::parse_file_result> task;
        std::chrono::steady_clock::time_point start_time;

        bool valid() const noexcept { return task.valid(); }
    } m_active_task;

    lib_config m_global_config;

    workspace_manager_external_file_requests* m_external_file_requests = nullptr;
    workspaces::file_manager_impl m_file_manager;

    std::unordered_map<resource_location, opened_workspace> m_workspaces;
    opened_workspace m_implicit_workspace;
    workspaces::workspace m_ws;
    bool m_vscode_extensions;

    std::vector<diagnostics_consumer*> m_diag_consumers;
    std::vector<parsing_metadata_consumer*> m_parsing_metadata_consumers;
    message_consumer* m_message_consumer = nullptr;
    workspace_manager_requests* m_requests = nullptr;
    std::vector<diagnostic> m_diagnostics;
    std::vector<fade_message> m_fade_messages;
    unsigned long long m_unique_id_sequence = 0;

    static std::string_view extract_scheme(std::string_view uri) { return uri.substr(0, uri.find(':') + 1); }
    void recompute_allow_list()
    {
        allowed_schemes.assign(std::begin(default_allowed_schemes), std::end(default_allowed_schemes));
        for (const auto& [uri, _] : m_workspaces)
        {
            const auto scheme = extract_scheme(uri.get_uri());
            if (scheme.empty())
                continue;
            allowed_schemes.emplace_back(scheme);
        }
        std::ranges::sort(allowed_schemes);
        auto [new_end, _] = std::ranges::unique(allowed_schemes);
        allowed_schemes.erase(new_end, allowed_schemes.end());
    }

    void add_workspace(std::string_view name, std::string_view uri) override
    {
        auto ws_uri = normalized_uri(uri);
        auto& ows = m_workspaces
                        .try_emplace(ws_uri,
                            ws_uri,
                            name,
                            m_file_manager,
                            m_global_config,
                            static_cast<external_configuration_requests*>(this))
                        .first->second;
        m_ws.set_message_consumer(m_message_consumer);

        recompute_allow_list();

        auto& new_workspace = m_work_queue.emplace_back(work_item {
            next_unique_id(),
            std::function<utils::task()>([this, &ows]() -> utils::task {
                return ows.config.parse_configuration_file().then([this](auto) {
                    notify_diagnostics_consumers();
                    invalidate_external_configuration({});
                    m_ws.mark_all_opened_files();
                });
            }),
            {},
            work_item_type::workspace_open,
        });

        attach_configuration_request(ws_uri.get_uri(), new_workspace, &ows);
    }

    void remove_workspace(std::string_view uri) override
    {
        auto it = m_workspaces.find(normalized_uri(uri));
        if (it == m_workspaces.end())
            return; // erase does no action, if the key does not exist

        auto* ows = &it->second;
        ows->logically_deleted = true;
        m_work_queue.emplace_back(work_item {
            next_unique_id(),
            std::function<void()>([this, it]() {
                m_workspaces.erase(it);

                recompute_allow_list();

                invalidate_external_configuration({});
                notify_diagnostics_consumers();

                m_ws.mark_all_opened_files();
            }),
            {},
            work_item_type::workspace_close,
        });
    }

    debugger_configuration_provider& get_debugger_configuration_provider() override { return *this; }

    void provide_debugger_configuration(
        std::string_view document_uri, workspace_manager_response<debugging::debugger_configuration> conf) override
    {
        using enum work_item_type;

        auto uri = normalized_uri(document_uri);
        // insert as a priority request, but after workspace_open requests if present
        m_work_queue.insert(
            std::ranges::find(std::views::reverse(m_work_queue), workspace_open, &work_item::request_type).base(),
            {
                next_unique_id(),
                std::function<utils::task()>([this, uri = std::move(uri), conf = std::move(conf)]() mutable {
                    return get_analyzer_configuration(std::move(uri)).then([this, conf = std::move(conf)](auto r) {
                        auto& [c, _] = r;
                        conf.provide({
                            .fm = &m_file_manager,
                            .libraries = std::move(c.libraries),
                            .opts = std::move(c.opts),
                            .pp_opts = std::move(c.pp_opts),
                        });
                    });
                }),
                {},
                dc_request,
            });
    }


    void read_external_configuration(
        std::string_view uri, workspace_manager_response<std::string_view> content) override
    {
        if (!m_requests || !m_vscode_extensions)
        {
            content.error(utils::error::not_found);
            return;
        }

        m_requests->request_file_configuration(uri, content);
    }

    void invalidate_external_configuration(std::string_view uri) override
    {
        if (uri.size() == 0)
        {
            resource_location res;
            m_implicit_workspace.config.prune_external_processor_groups(res);
            for (auto& [_, ows] : m_workspaces)
            {
                ows.config.prune_external_processor_groups(res);
            }
            m_ws.external_configuration_invalidated(res);
        }
        else
        {
            auto conf_uri = normalized_uri(uri);
            auto ows = ws_path_match(conf_uri);
            ows->config.prune_external_processor_groups(conf_uri);
            m_ws.external_configuration_invalidated(conf_uri);
        }
    }

    void retrieve_output(
        std::string_view document_uri, workspace_manager_response<std::span<const output_line>> r) override
    {
        handle_request(document_uri, std::move(r), [](const auto& resp, auto& ws, const auto& doc_loc) {
            resp.provide(ws.retrieve_output(doc_loc));
        });
    }

    [[nodiscard]] utils::value_task<
        std::pair<workspaces::analyzer_configuration, index_t<workspaces::processor_group, unsigned long long>>>
    get_analyzer_configuration(utils::resource::resource_location url) override
    {
        auto ows = ws_path_match(url);
        return ows->config.get_analyzer_configuration(std::move(url));
    }
    [[nodiscard]] workspaces::opcode_suggestion_data get_opcode_suggestion_data(
        const utils::resource::resource_location& url) override
    {
        auto ows = ws_path_match(url);
        return ows->config.get_opcode_suggestion_data(url);
    }

public:
    explicit workspace_manager_impl(
        workspace_manager_external_file_requests* external_file_requests, bool vscode_extensions)
        : m_external_file_requests(external_file_requests)
        , m_file_manager(*this)
        , m_implicit_workspace(m_file_manager, m_global_config, this)
        , m_ws(m_file_manager, *this)
        , m_vscode_extensions(vscode_extensions)
    {
        m_work_queue.emplace_back(work_item {
            next_unique_id(),
            std::function<utils::task()>([this]() -> utils::task {
                return m_implicit_workspace.config.parse_configuration_file().then(
                    [this](auto) { notify_diagnostics_consumers(); });
            }),
            {},
            work_item_type::workspace_open,
        });
    }
    workspace_manager_impl(const workspace_manager_impl&) = delete;
    workspace_manager_impl& operator=(const workspace_manager_impl&) = delete;

    workspace_manager_impl(workspace_manager_impl&&) = delete;
    workspace_manager_impl& operator=(workspace_manager_impl&&) = delete;
};

workspace_manager* create_workspace_manager_impl(
    workspace_manager_external_file_requests* external_requests, bool vscode_extensions)
{
    return new workspace_manager_impl(external_requests, vscode_extensions);
}

} // namespace hlasm_plugin::parser_library
