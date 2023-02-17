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

#ifndef HLASMPLUGIN_UTILS_TASK_H
#define HLASMPLUGIN_UTILS_TASK_H

#include <cassert>
#include <coroutine>
#include <exception>
#include <utility>

namespace hlasm_plugin::utils {

struct task
{
    class awaiter;
    struct promise_type
    {
        task get_return_object() { return task(std::coroutine_handle<promise_type>::from_promise(*this)); }
        std::suspend_always initial_suspend() const noexcept { return {}; }
        std::suspend_always final_suspend() noexcept
        {
            detach();
            return {};
        }
        void return_void() const noexcept {}
        void unhandled_exception()
        {
            if (!active || pending_exception)
                throw;
            pending_exception = std::current_exception();
        }

        std::coroutine_handle<promise_type> next_step = std::coroutine_handle<promise_type>::from_promise(*this);
        awaiter* active = nullptr;
        std::coroutine_handle<promise_type> top_waiter = std::coroutine_handle<promise_type>::from_promise(*this);
        std::exception_ptr pending_exception;

        void attach(std::coroutine_handle<promise_type> current_top_waiter, awaiter* a)
        {
            current_top_waiter.promise().next_step = std::coroutine_handle<promise_type>::from_promise(*this);
            active = a;
            top_waiter = std::move(current_top_waiter);
        }

        void detach() noexcept
        {
            if (active)
            {
                active->to_resume.promise().pending_exception = std::exchange(pending_exception, {});
                top_waiter.promise().next_step = active->to_resume;

                next_step = std::coroutine_handle<promise_type>::from_promise(*this);
                active = nullptr;
                top_waiter = std::coroutine_handle<promise_type>::from_promise(*this);
            }
        }
    };

    class awaiter
    {
        promise_type& self;
        std::coroutine_handle<promise_type> to_resume {};

        friend struct promise_type;

    public:
        constexpr bool await_ready() const noexcept { return false; }
        bool await_suspend(std::coroutine_handle<promise_type> h) noexcept
        {
            self.attach(h.promise().top_waiter, this);
            to_resume = std::move(h);
            return true;
        }
        void await_resume() const
        {
            if (to_resume.promise().pending_exception)
                std::rethrow_exception(std::exchange(to_resume.promise().pending_exception, {}));
        }

        explicit awaiter(promise_type& self) noexcept
            : self(self)
        {}
    };

    explicit task(std::coroutine_handle<promise_type> handle)
        : m_handle(std::move(handle))
    {}
    task(task&& t) noexcept
        : m_handle(std::exchange(t.m_handle, {}))
    {}
    task& operator=(task&& t) noexcept
    {
        task tmp(std::move(t));
        std::swap(m_handle, tmp.m_handle);

        return *this;
    }
    ~task()
    {
        if (m_handle)
        {
            // pending exception will be dropped - should we do something about it?
            m_handle.promise().detach();
            m_handle.destroy();
        }
    }

    bool done() const noexcept
    {
        assert(m_handle);
        return m_handle.done();
    }

    void operator()() const
    {
        assert(m_handle);
        m_handle.promise().next_step();
    }

    auto operator co_await() const&& { return awaiter(m_handle.promise()); }

    static std::suspend_always suspend() { return {}; }

    std::exception_ptr pending_exception(bool clear = false) const
    {
        assert(m_handle);
        auto& excp = m_handle.promise().next_step.promise().pending_exception;
        return clear ? std::exchange(excp, {}) : excp;
    }

private:
    std::coroutine_handle<promise_type> m_handle;
};

} // namespace hlasm_plugin::utils

#endif
