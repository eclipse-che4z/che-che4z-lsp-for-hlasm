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

#ifndef HLASMPLUGIN_PARSERLIBRARY_WORKSPACE_MANAGER_RESPONSE_H
#define HLASMPLUGIN_PARSERLIBRARY_WORKSPACE_MANAGER_RESPONSE_H

#include <atomic>
#include <utility>

namespace hlasm_plugin::parser_library {

namespace detail {
template<typename T>
struct is_in_place_type_t
{
    static constexpr bool value = false;
};
template<typename T>
struct is_in_place_type_t<std::in_place_type_t<T>>
{
    static constexpr bool value = true;
};

template<typename U, typename T>
concept workspace_manager_response_compatible = requires(U& u, T t)
{
    u.provide(std::move(t));
    {
        u.error(0, "")
    }
    noexcept;
};

} // namespace detail

class workspace_manager_response_base
{
protected:
    struct impl_actions
    {
        void (*addref)(void*) noexcept = nullptr;
        void (*release)(void*) noexcept = nullptr;
        void (*error)(void*, int, const char*) noexcept = nullptr;
        void (*provide)(void*, void*) noexcept = nullptr;
        void (*invalidate)(void*) noexcept = nullptr;
        bool (*valid)(void*) noexcept = nullptr;
        bool (*resolved)(void*) noexcept = nullptr;
    };
    struct shared_data_base
    {
        std::atomic<unsigned long long> counter = 1;
        std::atomic<bool> valid = true;
        std::atomic<bool> resolved = false;

        shared_data_base() = default;
    };

    workspace_manager_response_base() = default;
    workspace_manager_response_base(void* impl, const impl_actions* actions) noexcept
        : impl(impl)
        , actions(actions)
    {}

    workspace_manager_response_base(const workspace_manager_response_base& o) noexcept
        : impl(o.impl)
        , actions(o.actions)
    {
        if (impl)
            actions->addref(impl);
    }
    workspace_manager_response_base(workspace_manager_response_base&& o) noexcept
        : impl(std::exchange(o.impl, nullptr))
        , actions(std::exchange(o.actions, nullptr))
    {}
    workspace_manager_response_base& operator=(const workspace_manager_response_base& o) noexcept
    {
        workspace_manager_response_base tmp(o);
        std::swap(impl, tmp.impl);
        std::swap(actions, tmp.actions);
        return *this;
    }
    workspace_manager_response_base& operator=(workspace_manager_response_base&& o) noexcept
    {
        workspace_manager_response_base tmp(std::move(o));
        std::swap(impl, tmp.impl);
        std::swap(actions, tmp.actions);
        return *this;
    }
    ~workspace_manager_response_base()
    {
        if (impl)
            actions->release(impl);
    }

    bool valid() const noexcept { return actions->valid(impl); }
    bool resolved() const noexcept { return actions->resolved(impl); }
    void error(int ec, const char* error) const noexcept { actions->error(impl, ec, error); }
    template<typename E>
    void error(const E& e) const noexcept
    {
        error(e.code, e.msg);
    }
    void invalidate() const noexcept { actions->invalidate(impl); }
    void provide(void* t) const noexcept { actions->provide(impl, t); }

    void* get_impl() const noexcept { return impl; }

private:
    void* impl = nullptr;
    const impl_actions* actions = nullptr;
};

template<typename T>
class workspace_manager_response;

class
{
    template<typename U, typename T>
    T provide_type(void (U::*)(T)) const;
    template<typename U, typename T>
    T provide_type(void (U::*)(T) const) const;
    template<typename U, typename T>
    T provide_type(void (U::*)(T&&)) const;
    template<typename U, typename T>
    T provide_type(void (U::*)(T&&) const) const;

public:
    template<typename U>
    auto operator()(U u) const requires(!detail::is_in_place_type_t<U>::value)
    {
        auto result = workspace_manager_response<decltype(provide_type(&U::provide))>(std::move(u));

        return std::make_pair(std::move(result), result.template get_impl<U>());
    }
    template<typename U, typename... Args>
    auto operator()(std::in_place_type_t<U> u, Args&&... args) const
    {
        auto result =
            workspace_manager_response<decltype(provide_type(&U::provide))>(std::move(u), std::forward<Args>(args)...);
        return std::make_pair(std::move(result), result.template get_impl<U>());
    }

} inline constexpr make_workspace_manager_response;

template<typename T>
class workspace_manager_response : workspace_manager_response_base
{
    template<detail::workspace_manager_response_compatible<T> U>
    struct shared_data : shared_data_base
    {
        U data;

        explicit shared_data(U data) requires(!detail::is_in_place_type_t<U>::value)
            : data(std::move(data))
        {}
        template<typename... Args>
        explicit shared_data(std::in_place_type_t<U>, Args&&... args)
            : data(std::forward<Args>(args)...)
        {}
    };
    template<detail::workspace_manager_response_compatible<T> U>
    static constexpr impl_actions get_actions = {
        .addref =
            +[](void* p) noexcept { static_cast<shared_data<U>*>(p)->counter.fetch_add(1, std::memory_order_relaxed); },
        .release =
            +[](void* p) noexcept {
                if (auto* ptr = static_cast<shared_data<U>*>(p);
                    ptr->counter.fetch_sub(1, std::memory_order_release) == 1)
                {
                    std::atomic_thread_fence(std::memory_order_acquire);
                    delete ptr;
                }
            },
        .error =
            +[](void* p, int ec, const char* error) noexcept {
                auto* ptr = static_cast<shared_data<U>*>(p);
                ptr->data.error(ec, error);
                ptr->resolved.store(true, std::memory_order_release);
            },
        .provide =
            +[](void* p, void* t) noexcept {
                auto* ptr = static_cast<shared_data<U>*>(p);
                try
                {
                    ptr->data.provide(std::move(*static_cast<T*>(t)));
                }
                catch (...)
                {
                    ptr->data.error(-2, "Exception thrown while providing result");
                }
                ptr->resolved.store(true, std::memory_order_release);
            },
        .invalidate =
            +[](void* p) noexcept { static_cast<shared_data<U>*>(p)->valid.store(false, std::memory_order_relaxed); },
        .valid =
            +[](void* p) noexcept { return static_cast<shared_data<U>*>(p)->valid.load(std::memory_order_relaxed); },
        .resolved =
            +[](void* p) noexcept { return static_cast<shared_data<U>*>(p)->resolved.load(std::memory_order_acquire); },
    };

    template<typename U>
    U* get_impl() const
    {
        return &static_cast<shared_data<U>*>(workspace_manager_response_base::get_impl())->data;
    }

public:
    workspace_manager_response() = default;
    template<detail::workspace_manager_response_compatible<T> U>
    explicit workspace_manager_response(U u) requires(!detail::is_in_place_type_t<U>::value)
        : workspace_manager_response_base(new shared_data<U>(std::move(u)), &get_actions<U>)
    {}
    template<detail::workspace_manager_response_compatible<T> U, typename... Args>
    explicit workspace_manager_response(std::in_place_type_t<U> u, Args&&... args)
        : workspace_manager_response_base(
            new shared_data<U>(std::move(u), std::forward<Args>(args)...), &get_actions<U>)
    {}

    using workspace_manager_response_base::error;
    using workspace_manager_response_base::invalidate;
    using workspace_manager_response_base::resolved;
    using workspace_manager_response_base::valid;

    void provide(T&& t) const noexcept { workspace_manager_response_base::provide(&t); }

    friend decltype(make_workspace_manager_response);
};

} // namespace hlasm_plugin::parser_library

#endif
