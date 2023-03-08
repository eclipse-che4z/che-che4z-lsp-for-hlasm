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

#include "request_manager.h"

#include <chrono>
#include <string_view>

#include "nlohmann/json.hpp"

using namespace hlasm_plugin::language_server;

request::request(nlohmann::json message, server* executing_server)
    : message(std::move(message))
    , valid(true)
    , executing_server(executing_server)
{}

request_manager::request_manager(std::atomic<bool>* cancel, async_policy async_pol)
    : end_worker_(false)
    , cancel_(cancel)
    , worker_(&request_manager::handle_request_, this, &end_worker_)
    , async_policy_(async_pol)
{}

void request_manager::add_request(server* server, nlohmann::json message)
{
    if (async_policy_ == async_policy::SYNC)
    {
        server->message_received(message);
        return;
    }
    // add request to q
    {
        std::unique_lock<std::mutex> lock(q_mtx_);
        // get new file
        auto [file, is_parsing_required] = get_request_file_(message);
        // if the new file is the same as the currently running one, cancel the old one
        if (currently_running_file_ == file && currently_running_file_ != ""
            && (is_parsing_required == request_parsing_implication::parsing_required
                || is_parsing_required == request_parsing_implication::stop_parsing))
        {
            *cancel_ = true;
            // mark redundant requests as non valid
            for (auto& req : requests_)
            {
                if (req.valid && get_request_file_(req.message).first == file)
                    req.valid = false;
            }
        }

        // finally add it to the q
        requests_.push_back(request(std::move(message), server));
    }
    // wake up the worker thread
    cond_.notify_one();
}

void request_manager::end_worker()
{
    {
        std::lock_guard<std::mutex> lock(q_mtx_);
        end_worker_ = true;
    }

    cond_.notify_one();
    worker_.join();
}

bool request_manager::is_running() const
{
    std::unique_lock<std::mutex> lock(q_mtx_);
    return !requests_.empty();
}

void request_manager::handle_request_(const std::atomic<bool>* end_loop)
{
    // endless cycle in separate thread, pick up work if there is some, otherwise wait for work
    while (true)
    {
        std::unique_lock<std::mutex> lock(q_mtx_);
        // wait for work to come
        cond_.wait(lock, [&] { return !requests_.empty() || *end_loop; });
        if (*end_loop)
            return;

        // get first request
        auto to_run = std::move(requests_.front());
        requests_.pop_front();
        // remember file name that is about to be parsed
        currently_running_file_ = get_request_file_(to_run.message).first;
        // if the request is valid, do not cancel
        // if not, cancel the parsing right away, only the file manager should update the data
        if (cancel_)
            *cancel_ = !to_run.valid;

        // remember server, that is currently parsing
        currently_running_server_ = to_run.executing_server;
        // unlock the mutex, main thread may add new requests
        lock.unlock();
        // handle the request
        to_run.executing_server->message_received(to_run.message);

        currently_running_server_ = nullptr;
    }
}

void request_manager::finish_server_requests(server* to_finish)
{
    std::lock_guard guard(q_mtx_);

    if (requests_.empty())
        return;

    if (cancel_)
        *cancel_ = true;

    // if currently running request runs on the server we are about to finish, wait for that request to finish.
    while (currently_running_server_ == to_finish)
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // executes all remaining requests for a server
    for (auto& req : requests_)
    {
        if (req.executing_server != to_finish)
            continue;

        req.executing_server->message_received(req.message);
    }
    // remove the executed requests
    requests_.erase(
        std::remove_if(
            requests_.begin(), requests_.end(), [&to_finish](auto r) { return r.executing_server == to_finish; }),
        requests_.end());
}

std::pair<std::string, request_manager::request_parsing_implication> request_manager::get_request_file_(
    const nlohmann::json& r) const
{
    constexpr auto parsing_implication = [](std::string_view method) {
        if (method == "textDocument/didOpen" || method == "textDocument/didChange")
            return request_parsing_implication::parsing_required;
        if (method == "textDocument/didClose")
            return request_parsing_implication::stop_parsing;
        return request_parsing_implication::parsing_not_required;
    };

    auto found = r.find("method");
    if (found == r.end())
        return {};
    auto method = found->get<std::string>();
    if (!method.starts_with("textDocument"))
        return {};
    const auto params = r.find("params");
    if (params == r.end())
        return {};
    const auto textDocument = params->find("textDocument");
    if (textDocument == params->end())
        return {};
    const auto uri = textDocument->find("uri");
    if (uri == textDocument->end())
        return {};
    return { uri->get<std::string>(), parsing_implication(method) };
}
