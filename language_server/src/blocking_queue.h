/*
 * Copyright (c) 2021 Broadcom.
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

#ifndef HLASMPLUGIN_HLASMLANGUAGESERVER_BLOCKING_QUEUE_H
#define HLASMPLUGIN_HLASMLANGUAGESERVER_BLOCKING_QUEUE_H

#include <atomic>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <optional>

namespace hlasm_plugin::language_server {
template<typename T>
class blocking_queue
{
    std::mutex mutex;
    std::condition_variable cond_var;
    std::deque<T> queue;
    std::atomic<bool> terminated = false;

public:
    void push(T&& t)
    {
        std::unique_lock g(mutex);
        if (terminated)
            return;

        const bool notify = queue.size() == 0;
        queue.push_back(std::move(t));

        g.unlock();

        if (notify)
            cond_var.notify_one();
    }
    void push(const T& t)
    {
        std::unique_lock g(mutex);
        if (terminated)
            return;

        const bool notify = queue.size() == 0;
        queue.push_back(t);

        g.unlock();

        if (notify)
            cond_var.notify_one();
    }

    std::optional<T> pop()
    {
        std::unique_lock g(mutex);
        cond_var.wait(g, [this] { return queue.size() || terminated; });
        if (terminated)
            return std::nullopt;
        std::optional<T> result = std::move(queue.front());
        queue.pop_front();

        return result;
    }

    void terminate()
    {
        terminated = true;
        cond_var.notify_one();
    }
};
} // namespace hlasm_plugin::language_server

#endif // HLASMPLUGIN_HLASMLANGUAGESERVER_BLOCKING_QUEUE_H
