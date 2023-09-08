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
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "debugging/debugger_configuration.h"
#include "external_configuration_requests.h"
#include "lsp/completion_item.h"
#include "lsp/document_symbol_item.h"
#include "nlohmann/json.hpp"
#include "protocol.h"
#include "utils/async_busy_wait.h"
#include "utils/content_loader.h"
#include "utils/encoding.h"
#include "utils/error_codes.h"
#include "utils/path_conversions.h"
#include "utils/platform.h"
#include "utils/resource_location.h"
#include "utils/scope_exit.h"
#include "utils/task.h"
#include "workspace_manager.h"
#include "workspace_manager_external_file_requests.h"
#include "workspace_manager_response.h"
#include "workspaces/file_manager_impl.h"
#include "workspaces/workspace.h"

namespace hlasm_plugin::parser_library {

// Implementation of workspace manager (Implementation part of the pimpl idiom)
// Holds workspaces, file manager and macro tracer and handles LSP and DAP
// notifications and requests.

class workspace_manager_impl final : public workspace_manager,
                                     debugger_configuration_provider,
                                     public diagnosable_impl,
                                     workspaces::external_file_reader,
                                     external_configuration_requests
{
    static constexpr lib_config supress_all { 0 };
    using resource_location = utils::resource::resource_location;

    bool m_include_advisory_cfg_diags = false;

    struct opened_workspace
    {
        opened_workspace(const resource_location& location,
            const std::string&,
            workspaces::file_manager& file_manager,
            const lib_config& global_config,
            external_configuration_requests* ecr)
            : ws(location, file_manager, global_config, settings, ecr)
        {}
        opened_workspace(workspaces::file_manager& file_manager,
            const lib_config& global_config,
            external_configuration_requests* ecr)
            : ws(file_manager, global_config, settings, nullptr, ecr)
        {}
        workspaces::shared_json settings = std::make_shared<const nlohmann::json>(nlohmann::json::object());
        workspaces::workspace ws;
    };

public:
    explicit workspace_manager_impl(
        workspace_manager_external_file_requests* external_file_requests, bool vscode_extensions)
        : m_external_file_requests(external_file_requests)
        , m_file_manager(*this)
        , m_implicit_workspace(m_file_manager, m_global_config, this)
        , m_quiet_implicit_workspace(m_file_manager, supress_all, this)
        , m_vscode_extensions(vscode_extensions)
    {}
    workspace_manager_impl(const workspace_manager_impl&) = delete;
    workspace_manager_impl& operator=(const workspace_manager_impl&) = delete;

    workspace_manager_impl(workspace_manager_impl&&) = delete;
    workspace_manager_impl& operator=(workspace_manager_impl&&) = delete;


    static constexpr std::string_view hlasm_external_scheme = "hlasm-external:";
    static constexpr std::string_view allowed_scheme_list[] = {
        "file:", "untitled:", hlasm_external_scheme, "vscode-vfs:", "vscode-test-web:"
    };
    static bool allowed_scheme(const resource_location& uri)
    {
        const auto matches_scheme = [u = uri.get_uri()](const auto& p) { return u.starts_with(p); };
        return std::any_of(std::begin(allowed_scheme_list), std::end(allowed_scheme_list), matches_scheme);
    }

    static auto ws_path_match(auto& self, std::string_view unnormalized_uri)
    {
        auto uri = resource_location(unnormalized_uri).lexically_normal();
        if (auto hlasm_id = extract_hlasm_id(uri.get_uri()); hlasm_id.has_value())
        {
            if (auto related_ws = self.m_file_manager.get_virtual_file_workspace(hlasm_id.value()); !related_ws.empty())
                for (auto& [_, ows] : self.m_workspaces)
                    if (ows.ws.uri() == related_ws.get_uri())
                        return std::pair(&ows, std::move(uri));
        }

        resource_location url_to_match = uri;
        if (uri.get_uri().starts_with(hlasm_external_scheme))
        {
            if (utils::path::dissected_uri comp = utils::path::dissect_uri(uri.get_uri()); comp.fragment)
            {
                if (auto fragment = utils::encoding::uri_friendly_base16_decode(*comp.fragment); !fragment.empty())
                    url_to_match = resource_location(std::move(fragment));
            }
        }

        size_t max = 0;
        decltype(&self.m_workspaces.begin()->second) max_ows = nullptr;
        for (auto& [_, ows] : self.m_workspaces)
        {
            size_t match = prefix_match(url_to_match.get_uri(), ows.ws.uri());
            if (match > max && match >= ows.ws.uri().size())
            {
                max = match;
                max_ows = &ows;
            }
        }

        if (max_ows != nullptr)
            return std::pair(max_ows, std::move(uri));
        else if (allowed_scheme(uri))
            return std::pair(&self.m_implicit_workspace, std::move(uri));
        else
            return std::pair(&self.m_quiet_implicit_workspace, std::move(uri));
    }

    // returns implicit workspace, if the file does not belong to any workspace
    auto ws_path_match(std::string_view document_uri) { return ws_path_match(*this, document_uri); }
    auto ws_path_match(std::string_view document_uri) const { return ws_path_match(*this, document_uri); }

    enum class work_item_type
    {
        workspace_open,
        settings_change,
        file_change,
        query,
        dc_request,
    };

    struct work_item
    {
        unsigned long long id;

        opened_workspace* ows;

        std::variant<std::function<void()>, std::function<void(bool)>, utils::task, std::function<utils::task()>>
            action;
        std::function<bool()> validator; // maybe empty

        work_item_type request_type;

        std::function<void(work_item&)> on_workspace_delete;

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

    bool attach_configuration_request(work_item& wi)
    {
        if (!m_requests)
            return false;

        auto configuration_request = next_unique_id();

        struct open_workspace_t
        {
            unsigned long long work_item_id;
            unsigned long long request_id;
            workspace_manager_impl* self;

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

            void error(int, const char*) const noexcept
            {
                static const auto empty = std::make_shared<const nlohmann::json>();
                if (auto* wi = get_wi())
                    wi->ows->settings = empty;
            }
        };

        auto [resp, _] = make_workspace_manager_response(open_workspace_t { wi.id, configuration_request, this });

        wi.pending_requests.emplace_back(configuration_request, [resp = resp]() noexcept { resp.invalidate(); });

        m_requests->request_workspace_configuration(wi.ows->ws.uri().c_str(), std::move(resp));

        return true;
    }

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
            for (auto consumer : m_parsing_metadata_consumers)
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
        auto result = run_parse_loop(m_implicit_workspace, yield_indicator);
        combine(result, run_parse_loop(m_quiet_implicit_workspace, yield_indicator));
        for (auto& [_, ows] : m_workspaces)
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

    void did_open_file(const char* document_uri, version_t version, const char* text_ptr, size_t text_size) override
    {
        auto [ows, uri] = ws_path_match(document_uri);
        auto open_result = std::make_shared<workspaces::file_content_state>();
        m_work_queue.emplace_back(work_item {
            next_unique_id(),
            nullptr,
            [this, document_loc = uri, version, text = std::string(text_ptr, text_size), open_result]() mutable {
                *open_result = m_file_manager.did_open_file(document_loc, version, std::move(text));
            },
            {},
            work_item_type::file_change,
        });
        m_work_queue.emplace_back(work_item {
            next_unique_id(),
            ows,
            std::function<utils::task()>([document_loc = std::move(uri), &ws = ows->ws, open_result]() mutable {
                return ws.did_open_file(std::move(document_loc), *open_result);
            }),
            {},
            work_item_type::file_change,
        });
    }

    void did_change_file(
        const char* document_uri, version_t version, const document_change* changes, size_t ch_size) override
    {
        auto [ows, uri] = ws_path_match(document_uri);

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
            nullptr,
            [this, document_loc = uri, version, captured_changes = std::move(captured_changes)]() {
                std::vector<document_change> list;
                list.reserve(captured_changes.size());
                std::transform(captured_changes.begin(),
                    captured_changes.end(),
                    std::back_inserter(list),
                    [](const captured_change& cc) {
                        return cc.whole ? document_change(cc.text.data(), cc.text.size())
                                        : document_change(cc.change_range, cc.text.data(), cc.text.size());
                    });
                m_file_manager.did_change_file(document_loc, version, list.data(), list.size());
            },
            {},
            work_item_type::file_change,
        });

        m_work_queue.emplace_back(work_item {
            next_unique_id(),
            ows,
            std::function<utils::task()>(
                [document_loc = std::move(uri),
                    &ws = ows->ws,
                    file_content_status = ch_size ? workspaces::file_content_state::changed_content
                                                  : workspaces::file_content_state::identical]() mutable {
                    return ws.did_change_file(std::move(document_loc), file_content_status);
                }),
            {},
            work_item_type::file_change,
        });
    }

    void did_close_file(const char* document_uri) override
    {
        auto [ows, uri] = ws_path_match(document_uri);
        m_work_queue.emplace_back(work_item {
            next_unique_id(),
            nullptr,
            [this, document_loc = uri]() { m_file_manager.did_close_file(document_loc); },
            {},
            work_item_type::file_change,
        });
        m_work_queue.emplace_back(work_item {
            next_unique_id(),
            ows,
            std::function<utils::task()>([this, document_loc = std::move(uri), &ws = ows->ws]() mutable {
                return ws.did_close_file(std::move(document_loc)).then([this]() { notify_diagnostics_consumers(); });
            }),
            {},
            work_item_type::file_change,
        });
    }

    void did_change_watched_files(sequence<fs_change> changes) override
    {
        auto paths_for_ws = std::make_shared<std::unordered_map<opened_workspace*,
            std::pair<std::vector<resource_location>, std::vector<workspaces::file_content_state>>>>();
        for (auto& change : changes)
        {
            auto [ows, uri] = ws_path_match(std::string_view(change.uri));
            auto& [path_list, _] = (*paths_for_ws)[ows];
            path_list.emplace_back(std::move(uri));
        }

        m_work_queue.emplace_back(work_item {
            next_unique_id(),
            nullptr,
            std::function<utils::task()>([this, paths_for_ws]() -> utils::task {
                std::vector<utils::task> pending_updates;
                for (auto& [_, path_change_list] : *paths_for_ws)
                {
                    auto& [paths, changes] = path_change_list;
                    changes.resize(paths.size());
                    auto cit = changes.begin();
                    for (const auto& path : paths)
                    {
                        auto update = m_file_manager.update_file(path);
                        auto& change = *cit++;
                        if (!update.valid())
                            change = workspaces::file_content_state::identical;
                        else if (update.done())
                            change = update.value();
                        else
                            pending_updates.emplace_back(std::move(update).then([&change](auto c) { change = c; }));
                    }
                }
                if (pending_updates.empty())
                    return {};
                return utils::task::wait_all(std::move(pending_updates));
            }),
            {},
            work_item_type::file_change,
        });

        for (auto& [ows, path_change_list] : *paths_for_ws)
            m_work_queue.emplace_back(work_item {
                next_unique_id(),
                ows,
                std::function<utils::task()>(
                    [path_change_list_p = std::shared_ptr<
                         std::pair<std::vector<resource_location>, std::vector<workspaces::file_content_state>>>(
                         paths_for_ws, &path_change_list),
                        &ws = ows->ws]() {
                        return ws.did_change_watched_files(
                            std::move(path_change_list_p->first), std::move(path_change_list_p->second));
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
        m_diag_consumers.erase(
            std::remove(m_diag_consumers.begin(), m_diag_consumers.end(), consumer), m_diag_consumers.end());
    }

    void register_parsing_metadata_consumer(parsing_metadata_consumer* consumer) override
    {
        m_parsing_metadata_consumers.push_back(consumer);
    }

    void unregister_parsing_metadata_consumer(parsing_metadata_consumer* consumer) override
    {
        auto& pmc = m_parsing_metadata_consumers;
        pmc.erase(std::remove(pmc.begin(), pmc.end(), consumer), pmc.end());
    }

    void set_message_consumer(message_consumer* consumer) override
    {
        m_message_consumer = consumer;
        m_implicit_workspace.ws.set_message_consumer(consumer);
        for (auto& wks : m_workspaces)
            wks.second.ws.set_message_consumer(consumer);
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

    void definition(const char* document_uri, position pos, workspace_manager_response<position_uri> r) override
    {
        auto [ows, uri] = ws_path_match(document_uri);

        m_work_queue.emplace_back(work_item {
            next_unique_id(),
            ows,
            response_handle(r,
                [&ws = ows->ws, doc_loc = std::move(uri), pos](const workspace_manager_response<position_uri>& resp) {
                    resp.provide(position_uri(ws.definition(doc_loc, pos)));
                }),
            [r]() { return r.valid(); },
            work_item_type::query,
        });
    }

    void references(const char* document_uri, position pos, workspace_manager_response<position_uri_list> r) override
    {
        auto [ows, uri] = ws_path_match(document_uri);

        m_work_queue.emplace_back(work_item {
            next_unique_id(),
            ows,
            response_handle(r,
                [&ws = ows->ws, doc_loc = std::move(uri), pos](
                    const workspace_manager_response<position_uri_list>& resp) {
                    auto references_result = ws.references(doc_loc, pos);
                    resp.provide({ references_result.data(), references_result.size() });
                }),
            [r]() { return r.valid(); },
            work_item_type::query,
        });
    }

    void hover(const char* document_uri, position pos, workspace_manager_response<sequence<char>> r) override
    {
        auto [ows, uri] = ws_path_match(document_uri);

        m_work_queue.emplace_back(work_item {
            next_unique_id(),
            ows,
            response_handle(r,
                [&ws = ows->ws, doc_loc = std::move(uri), pos](const workspace_manager_response<sequence<char>>& resp) {
                    auto hover_result = ws.hover(doc_loc, pos);
                    resp.provide(sequence<char>(hover_result));
                }),
            [r]() { return r.valid(); },
            work_item_type::query,
        });
    }


    void completion(const char* document_uri,
        position pos,
        const char trigger_char,
        completion_trigger_kind trigger_kind,
        workspace_manager_response<completion_list> r) override
    {
        auto [ows, uri] = ws_path_match(document_uri);

        m_work_queue.emplace_back(work_item {
            next_unique_id(),
            ows,
            response_handle(r,
                [&ws = ows->ws, doc_loc = std::move(uri), pos, trigger_char, trigger_kind](
                    const workspace_manager_response<completion_list>& resp) {
                    auto completion_result = ws.completion(doc_loc, pos, trigger_char, trigger_kind);
                    resp.provide(completion_list(completion_result.data(), completion_result.size()));
                }),
            [r]() { return r.valid(); },
            work_item_type::query,
        });
    }

    void document_symbol(
        const char* document_uri, long long limit, workspace_manager_response<document_symbol_list> r) override
    {
        auto [ows, uri] = ws_path_match(document_uri);

        m_work_queue.emplace_back(work_item {
            next_unique_id(),
            ows,
            response_handle(r,
                [&ws = ows->ws, doc_loc = std::move(uri), limit](
                    const workspace_manager_response<document_symbol_list>& resp) {
                    auto document_symbol_result = ws.document_symbol(doc_loc, limit);
                    resp.provide(document_symbol_list(document_symbol_result.data(), document_symbol_result.size()));
                }),
            [r]() { return r.valid(); },
            work_item_type::query,
        });
    }

    void configuration_changed(const lib_config& new_config) override
    {
        // TODO: should this action be also performed IN ORDER?

        m_global_config = new_config;

        m_work_queue.emplace_back(work_item {
            next_unique_id(),
            &m_implicit_workspace,
            std::function<utils::task()>([this, &ws = m_implicit_workspace.ws]() -> utils::task {
                return ws.settings_updated().then([this](bool u) {
                    if (u)
                        notify_diagnostics_consumers();
                });
            }),
            {},
            work_item_type::settings_change,
        });

        for (auto& [_, ows] : m_workspaces)
        {
            auto& refersh_settings = m_work_queue.emplace_back(work_item {
                next_unique_id(),
                &ows,
                std::function<utils::task()>([this, &ws = ows.ws]() -> utils::task {
                    return ws.settings_updated().then([this](bool u) {
                        if (u)
                            notify_diagnostics_consumers();
                    });
                }),
                {},
                work_item_type::settings_change,
            });

            attach_configuration_request(refersh_settings);
        }
    }

    void semantic_tokens(
        const char* document_uri, workspace_manager_response<continuous_sequence<token_info>> r) override
    {
        auto [ows, uri] = ws_path_match(document_uri);

        m_work_queue.emplace_back(work_item {
            next_unique_id(),
            ows,
            response_handle(r,
                [&ws = ows->ws, doc_loc = std::move(uri)](
                    const workspace_manager_response<continuous_sequence<token_info>>& resp) {
                    resp.provide(make_continuous_sequence(ws.semantic_tokens(doc_loc)));
                }),
            [r]() { return r.valid(); },
            work_item_type::query,
        });
    }

    continuous_sequence<char> get_virtual_file_content(unsigned long long id) const override
    {
        return make_continuous_sequence(m_file_manager.get_virtual_file(id));
    }

    void toggle_advisory_configuration_diagnostics() override
    {
        m_include_advisory_cfg_diags ^= true;

        // implicit workspaces are not affected
        for (auto& [_, opened_ws] : m_workspaces)
            opened_ws.ws.include_advisory_configuration_diagnostics(m_include_advisory_cfg_diags);

        notify_diagnostics_consumers();
    }

    void make_opcode_suggestion(const char* document_uri,
        const char* opcode,
        bool extended,
        workspace_manager_response<continuous_sequence<opcode_suggestion>> r) override
    {
        auto [ows, uri] = ws_path_match(document_uri);
        // performed out of order
        auto suggestions = ows->ws.make_opcode_suggestion(uri, opcode, extended);

        std::vector<opcode_suggestion> res;

        for (auto&& [suggestion, distance] : suggestions)
            res.emplace_back(opcode_suggestion { make_continuous_sequence(std::move(suggestion)), distance });

        r.provide(make_continuous_sequence(std::move(res)));
    }

private:
    void collect_diags() const override
    {
        collect_diags_from_child(m_implicit_workspace.ws);
        collect_diags_from_child(m_quiet_implicit_workspace.ws);
        for (auto& it : m_workspaces)
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

        m_fade_messages.clear();
        m_implicit_workspace.ws.retrieve_fade_messages(m_fade_messages);
        m_quiet_implicit_workspace.ws.retrieve_fade_messages(m_fade_messages);
        for (const auto& [_, ows] : m_workspaces)
            ows.ws.retrieve_fade_messages(m_fade_messages);

        for (auto consumer : m_diag_consumers)
            consumer->consume_diagnostics(diagnostic_list(diags().data(), diags().size()),
                fade_message_list(m_fade_messages.data(), m_fade_messages.size()));
    }

    static size_t prefix_match(std::string_view first, std::string_view second)
    {
        auto [f, s] = std::mismatch(first.begin(), first.end(), second.begin(), second.end());
        return static_cast<size_t>(std::min(f - first.begin(), s - second.begin()));
    }

    unsigned long long next_unique_id() { return ++m_unique_id_sequence; }

    [[nodiscard]] utils::value_task<std::optional<std::string>> load_text_external(
        const utils::resource::resource_location& document_loc) const
    {
        struct content_t
        {
            std::optional<std::string> result;

            void provide(sequence<char> c) { result = std::string(c); }
            void error(int, const char*) noexcept { result.reset(); }
        };
        auto [channel, data] = make_workspace_manager_response(std::in_place_type<content_t>);
        m_external_file_requests->read_external_file(document_loc.get_uri().c_str(), channel);

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
                        auto url = utils::resource::resource_location(std::string_view(s));
                        auto filename = url.filename();
                        if (subdir)
                            url.join("");
                        filename = utils::encoding::percent_decode(filename);
                        if (filename.empty())
                        {
                            result = { {}, other_failure };
                            break;
                        }
                        dirs.emplace_back(std::move(filename), std::move(url));
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
        m_external_file_requests->read_external_directory(data->dir.get_uri().c_str(), channel, subdir);

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
        opened_workspace* ows = nullptr;
        std::chrono::steady_clock::time_point start_time;

        bool valid() const noexcept { return task.valid(); }
    } m_active_task;

    lib_config m_global_config;

    workspace_manager_external_file_requests* m_external_file_requests = nullptr;
    workspaces::file_manager_impl m_file_manager;

    std::unordered_map<resource_location, opened_workspace, utils::resource::resource_location_hasher> m_workspaces;
    opened_workspace m_implicit_workspace;
    opened_workspace m_quiet_implicit_workspace;
    bool m_vscode_extensions;

    std::vector<diagnostics_consumer*> m_diag_consumers;
    std::vector<parsing_metadata_consumer*> m_parsing_metadata_consumers;
    message_consumer* m_message_consumer = nullptr;
    workspace_manager_requests* m_requests = nullptr;
    std::vector<fade_message_s> m_fade_messages;
    unsigned long long m_unique_id_sequence = 0;

    void add_workspace(const char* name, const char* uri) override
    {
        auto normalized_uri = resource_location(uri).lexically_normal();
        auto& ows = m_workspaces
                        .try_emplace(normalized_uri,
                            normalized_uri,
                            name,
                            m_file_manager,
                            m_global_config,
                            static_cast<external_configuration_requests*>(this))
                        .first->second;
        ows.ws.set_message_consumer(m_message_consumer);
        ows.ws.include_advisory_configuration_diagnostics(m_include_advisory_cfg_diags);

        auto& new_workspace = m_work_queue.emplace_back(work_item {
            next_unique_id(),
            &ows,
            std::function<utils::task()>([this, &ws = ows.ws]() -> utils::task {
                return ws.open().then([this]() { notify_diagnostics_consumers(); });
            }),
            {},
            work_item_type::workspace_open,
        });

        attach_configuration_request(new_workspace);
    }

    void remove_workspace(const char* uri) override
    {
        auto it = m_workspaces.find(resource_location(uri).lexically_normal());
        if (it == m_workspaces.end())
            return; // erase does no action, if the key does not exist

        auto* ows = &it->second;

        for (auto& e : m_work_queue)
        {
            if (e.ows != ows)
                continue;

            e.workspace_removed = true;
            e.cancel_pending_requests();
            if (e.on_workspace_delete)
                e.on_workspace_delete(e);
        }
        if (m_active_task.ows == ows)
            m_active_task = {};

        m_workspaces.erase(it);
        notify_diagnostics_consumers();
    }

    debugger_configuration_provider& get_debugger_configuration_provider() override { return *this; }

    void provide_debugger_configuration(
        sequence<char> document_uri, workspace_manager_response<debugging::debugger_configuration> conf) override
    {
        auto [ows, uri] = ws_path_match(std::string_view(document_uri));
        work_item wi {
            next_unique_id(),
            ows,
            ows->ws.get_debugger_configuration(std::move(uri)).then([conf](debugging::debugger_configuration dc) {
                conf.provide(std::move(dc));
            }),
            {},
            work_item_type::dc_request,
            [conf](work_item& me) {
                conf.error(utils::error::workspace_removed);
                me.action = []() {};
            },
        };
        const auto matching_open_request = [ows = ows](const auto& w) {
            return w.request_type == work_item_type::workspace_open && w.ows == ows;
        };
        // insert as a priority request, but after matching workspace_open request if present
        if (auto it = std::find_if(m_work_queue.begin(), m_work_queue.end(), matching_open_request);
            it != m_work_queue.end())
            m_work_queue.insert(++it, std::move(wi));
        else
            m_work_queue.push_front(std::move(wi));
    }


    void read_external_configuration(sequence<char> uri, workspace_manager_response<sequence<char>> content) override
    {
        if (!m_requests || !m_vscode_extensions)
        {
            content.error(utils::error::not_found);
            return;
        }

        m_requests->request_file_configuration(uri, content);
    }

    void invalidate_external_configuration(sequence<char> uri) override
    {
        if (uri.size() == 0)
        {
            resource_location res;
            for (auto& [_, ows] : m_workspaces)
                ows.ws.invalidate_external_configuration(res);
            m_implicit_workspace.ws.invalidate_external_configuration(res);
            m_quiet_implicit_workspace.ws.invalidate_external_configuration(res);
        }
        else
        {
            auto [ows, conf_uri] = ws_path_match(std::string_view(uri));
            ows->ws.invalidate_external_configuration(conf_uri);
        }
    }
};

workspace_manager* create_workspace_manager_impl(
    workspace_manager_external_file_requests* external_requests, bool vscode_extensions)
{
    return new workspace_manager_impl(external_requests, vscode_extensions);
}

} // namespace hlasm_plugin::parser_library
