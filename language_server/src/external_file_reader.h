/*
 * Copyright (c) 2023 Broadcom.
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

#ifndef HLASMPLUGIN_LANGUAGESERVER_EXTERNAL_FILE_READER_H
#define HLASMPLUGIN_LANGUAGESERVER_EXTERNAL_FILE_READER_H

#include <atomic>
#include <functional>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <utility>

#include "json_channel.h"

#include "message_router.h"
#include "nlohmann/json_fwd.hpp"
#include "sequence.h"
#include "workspace_manager_external_file_requests.h"

namespace hlasm_plugin::language_server {

class external_file_reader final : public parser_library::workspace_manager_external_file_requests, public json_sink
{
    std::mutex m_mutex;
    json_sink& m_output;

    std::atomic<size_t> m_next_id = 1;
    std::unordered_map<size_t, std::pair<std::thread::id, std::function<void(bool error, const nlohmann::json&)>>>
        m_pending_requests;

    std::unordered_map<std::thread::id, std::function<void()>> m_registrations;

    bool enqueue_message(
        size_t next_id, nlohmann::json msg, std::function<void(bool error, const nlohmann::json&)> handler);

    void wakeup_thread(std::thread::id id);

public:
    explicit external_file_reader(json_sink& output)
        : m_output(output)
    {}

    // Inherited via workspace_manager_external_file_requests
    void read_external_file(
        const char* url, parser_library::workspace_manager_response<parser_library::sequence<char>> content) override;
    void read_external_directory(const char* url,
        parser_library::workspace_manager_response<parser_library::workspace_manager_external_directory_result> members,
        bool subdir = false) override;

    class thread_registration
    {
        external_file_reader* m_self = nullptr;

    public:
        thread_registration() = default;
        explicit thread_registration(external_file_reader& self)
            : m_self(&self)
        {}
        thread_registration(const thread_registration&) = delete;
        thread_registration(thread_registration&&) = delete;
        thread_registration& operator=(const thread_registration&) = delete;
        thread_registration& operator=(thread_registration&&) = delete;
        ~thread_registration();
    };

    thread_registration register_thread(std::function<void()> wakeup_rtn);

    // Inherited via json_sink
    void write(const nlohmann::json&) override;
    void write(nlohmann::json&&) override;

    message_router::message_predicate get_filtering_predicate() const;
};

} // namespace hlasm_plugin::language_server

#endif // !HLASMPLUGIN_LANGUAGESERVER_EXTERNAL_FILE_READER_H
