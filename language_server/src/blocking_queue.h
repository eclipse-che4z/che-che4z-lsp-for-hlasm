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

enum class blocking_queue_termination_policy : bool
{
    drop_elements,
    process_elements,
};

template<typename T,
    blocking_queue_termination_policy termination_policy = blocking_queue_termination_policy::drop_elements,
    bool one_reader = true>
class blocking_queue
{
    static constexpr unsigned char terminated_flag = 0x01;
    static constexpr unsigned char has_elements_flag = 0x02;

    std::mutex mutex;
    std::condition_variable cond_var;
    std::deque<T> queue;
    std::atomic<unsigned char> state = 0;

public:
    bool push(T&& t)
    {
        std::unique_lock g(mutex);

        if (terminated())
            return false;

        const bool notify = queue.size() == 0;
        queue.push_back(std::move(t));
        state.fetch_or(has_elements_flag, std::memory_order_relaxed);

        g.unlock();

        if (notify)
            cond_var.notify_one();

        return true;
    }

    bool push(const T& t)
    {
        std::unique_lock g(mutex);

        if (terminated())
            return false;

        const bool notify = queue.size() == 0;
        queue.push_back(t);
        state.fetch_or(has_elements_flag, std::memory_order_relaxed);

        g.unlock();

        if (notify)
            cond_var.notify_one();

        return true;
    }

    std::optional<T> pop()
    {
        constexpr auto drop = blocking_queue_termination_policy::drop_elements;
        constexpr auto process = blocking_queue_termination_policy::process_elements;

        std::unique_lock g(mutex);
        cond_var.wait(g, [this] { return queue.size() || terminated(); });

        if ((termination_policy == drop && terminated()) || (termination_policy == process && queue.size() == 0))
            return std::nullopt;

        std::optional<T> result = std::move(queue.front());
        queue.pop_front();
        if (queue.empty())
            state.fetch_and(static_cast<unsigned char>(~has_elements_flag), std::memory_order_relaxed);

        return result;
    }

    void terminate()
    {
        std::unique_lock g(mutex);
        state.fetch_or(terminated_flag, std::memory_order_relaxed);
        g.unlock();

        cond_var.notify_one();
    }

    bool terminated() const { return state.load(std::memory_order_relaxed) & terminated_flag; }
    bool empty() const { return !(state.load(std::memory_order_relaxed) & has_elements_flag); }

    bool will_block() const requires one_reader { return state.load(std::memory_order_relaxed) == 0; }

    const std::atomic<unsigned char>* state_preview() const { return &state; }
};
} // namespace hlasm_plugin::language_server

#endif // HLASMPLUGIN_HLASMLANGUAGESERVER_BLOCKING_QUEUE_H
