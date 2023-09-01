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

#include "external_file_reader.h"

#include <cassert>
#include <string_view>

#include "nlohmann/json.hpp"
#include "utils/error_codes.h"

using namespace hlasm_plugin;
using namespace hlasm_plugin::parser_library;

namespace hlasm_plugin::language_server {

constexpr std::string_view request_type_message = "external_file_request";
constexpr std::string_view response_type_message = "external_file_response";

namespace {
constexpr std::pair<int, const char*> unknown_error { -1, "Unknown error" };
std::pair<int, const char*> extract_error(const nlohmann::json& errmsg) noexcept
{
    if (!errmsg.is_object())
        return unknown_error;

    auto code = errmsg.find("code");
    auto msg = errmsg.find("msg");

    if (code == errmsg.end() || msg == errmsg.end() || !code->is_number_integer() || !msg->is_string())
        return unknown_error;

    return { code->get<int>(), msg->get<const std::string*>()->c_str() };
}
} // namespace

bool external_file_reader::enqueue_message(
    size_t next_id, nlohmann::json msg, std::function<void(bool error, const nlohmann::json&)> handler)
{
    auto tid = std::this_thread::get_id();

    if (std::lock_guard g(m_mutex); m_registrations.contains(tid))
        m_pending_requests.try_emplace(next_id, std::move(tid), std::move(handler));
    else
        return false;

    m_output.write({ { "jsonrpc", "2.0" }, { "method", request_type_message }, { "params", std::move(msg) } });

    return true;
}

void external_file_reader::read_external_file(const char* url, workspace_manager_response<sequence<char>> content)
{
    auto next_id = m_next_id.fetch_add(1, std::memory_order_relaxed);
    nlohmann::json msg = {
        { "id", next_id },
        { "op", "read_file" },
        { "url", url },
    };

    std::function handler = [content](bool error, const nlohmann::json& result) noexcept {
        if (error)
        {
            auto [err, errmsg] = extract_error(result);
            content.error(err, errmsg);
        }
        else if (!result.is_string())
            content.error(utils::error::invalid_json);
        else
            content.provide(sequence<char>(result.get<std::string_view>()));
    };

    if (!enqueue_message(next_id, std::move(msg), std::move(handler)))
        content.error(utils::error::message_send);
}

void external_file_reader::read_external_directory(
    const char* url, workspace_manager_response<workspace_manager_external_directory_result> members)
{
    auto next_id = m_next_id.fetch_add(1, std::memory_order_relaxed);
    nlohmann::json msg = {
        { "id", next_id },
        { "op", "list_directory" },
        { "url", url },
    };

    std::function handler = [members](bool error, const nlohmann::json& result) noexcept {
        if (error)
        {
            auto [err, errmsg] = extract_error(result);
            members.error(err, errmsg);
            return;
        }
        auto member_urls = result.find("member_urls");
        if (member_urls == result.end() || !member_urls->is_array())
        {
            members.error(utils::error::invalid_json);
            return;
        }
        std::vector<sequence<char>> tmp;
        try
        {
            tmp.reserve(result.size());
        }
        catch (const std::bad_alloc&)
        {
            members.error(utils::error::allocation);
            return;
        }
        for (const auto& item : *member_urls)
        {
            if (!item.is_string())
            {
                members.error(utils::error::invalid_json);
                return;
            }
            tmp.emplace_back(sequence<char>(item.get<std::string_view>()));
        }

        members.provide({
            .member_urls = sequence<sequence<char>>(tmp),
        });
    };

    if (!enqueue_message(next_id, std::move(msg), std::move(handler)))
        members.error(utils::error::message_send);
}


void external_file_reader::wakeup_thread(std::thread::id id)
{
    std::lock_guard g(m_mutex);

    if (auto it = m_registrations.find(id); it != m_registrations.end())
        it->second();
}

external_file_reader::thread_registration external_file_reader::register_thread(std::function<void()> wakeup_rtn)
{
    assert(wakeup_rtn);

    auto tid = std::this_thread::get_id();

    std::lock_guard g(m_mutex);

    auto [it, inserted] = m_registrations.try_emplace(tid, std::move(wakeup_rtn));

    assert(inserted);

    return thread_registration(*this);
}

namespace {
std::string_view extract_method(const nlohmann::json& msg)
{
    auto method_ptr = msg.find("method");
    if (method_ptr != msg.end() && method_ptr->is_string())
        return method_ptr->get<std::string_view>();
    else
        return {};
}
} // namespace

void external_file_reader::write(const nlohmann::json& msg)
{
    const auto params = msg.find("params");
    if (params == msg.end() || !params->is_object())
        return;

    const auto id = params->value("id", (size_t)0);
    const auto data = params->find("data");
    const auto error = params->find("error");

    if (const auto node = ((void)std::lock_guard(m_mutex), m_pending_requests.extract(id)))
    {
        const auto& [tid, handler] = node.mapped();
        if (error != params->end())
            handler(true, *error);
        else if (data == params->end())
            handler(true, {});
        else
            handler(false, *data);
        wakeup_thread(tid);
    }

    return;
}

void external_file_reader::write(nlohmann::json&& msg) { write(msg); }

message_router::message_predicate external_file_reader::get_filtering_predicate() const
{
    return [](const nlohmann::json& msg) { return extract_method(msg) == response_type_message; };
}

external_file_reader::thread_registration::~thread_registration()
{
    if (!m_self)
        return;
    auto tid = std::this_thread::get_id();

    std::lock_guard g(m_self->m_mutex);

    std::erase_if(m_self->m_pending_requests, [&tid](const auto& v) { return v.second.first == tid; });

    m_self->m_registrations.erase(tid);
}

} // namespace hlasm_plugin::language_server
