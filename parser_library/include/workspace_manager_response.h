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
} // namespace detail

class workspace_manager_response_base
{
protected:
    struct impl_actions
    {
        void (*deleter)(void*) noexcept = nullptr;
        void (*error)(void*, int, const char*) = nullptr;
        void (*provide)(void*, void*) = nullptr;
    };
    struct invalidator_t
    {
        void* impl = nullptr;
        void (*deleter)(void*) noexcept = nullptr;
        union
        {
            void (*complex)(void*) = nullptr;
            void (*simple)();
        };
    };
    struct shared_data_base
    {
        unsigned long long counter = 1;
        invalidator_t invalidator;
        bool valid = true;
        bool resolved = false;

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
            ++static_cast<shared_data_base*>(impl)->counter;
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
        if (impl && --static_cast<shared_data_base*>(impl)->counter == 0)
        {
            change_invalidation_callback({});
            actions->deleter(impl);
        }
    }

    bool valid() const noexcept { return static_cast<shared_data_base*>(impl)->valid; }
    bool resolved() const noexcept { return static_cast<shared_data_base*>(impl)->resolved; }
    void error(int ec, const char* error) const
    {
        static_cast<shared_data_base*>(impl)->resolved = true;
        return actions->error(impl, ec, error);
    }
    void invalidate() const
    {
        auto* base = static_cast<shared_data_base*>(impl);
        if (!base->valid)
            return;

        base->valid = false;
        auto& i = base->invalidator;
        if (i.impl == impl)
            i.simple();
        else if (i.impl)
            i.complex(i.impl);
    }

    template<typename C>
    void set_invalidation_callback(C t)
    {
        change_invalidation_callback(invalidator_t {
            new C(std::move(t)),
            +[](void* p) noexcept { delete static_cast<C*>(p); },
            +[](void* p) { (*static_cast<C*>(p))(); },
        });
    }

    template<typename C>
    void set_invalidation_callback(C t) noexcept requires std::is_function_v<C>
    {
        change_invalidation_callback(invalidator_t { impl, nullptr, t });
    }

    void remove_invalidation_handler() noexcept { change_invalidation_callback({}); }

    void provide(void* t) const
    {
        static_cast<shared_data_base*>(impl)->resolved = true;
        actions->provide(impl, t);
    }

    void* get_impl() const noexcept { return impl; }

private:
    void* impl = nullptr;
    const impl_actions* actions = nullptr;

    void change_invalidation_callback(invalidator_t next_i) noexcept
    {
        auto& i = static_cast<shared_data_base*>(impl)->invalidator;

        std::swap(i, next_i);

        if (next_i.deleter)
            next_i.deleter(next_i.impl);
    }
};

template<typename T>
class workspace_manager_response;

class
{
    template<typename U, typename T>
    T provide_type(void (U::*)(T)) const;
    template<typename U, typename T>
    T provide_type(void (U::*)(T) const) const;

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
    template<typename U>
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
    template<typename U>
    static constexpr impl_actions get_actions = {
        .deleter = +[](void* p) noexcept { delete static_cast<shared_data<U>*>(p); },
        .error = +[](void* p, int ec, const char* error) { static_cast<shared_data<U>*>(p)->data.error(ec, error); },
        .provide =
            +[](void* p, void* t) { static_cast<shared_data<U>*>(p)->data.provide(std::move(*static_cast<T*>(t))); },
    };

    template<typename U>
    U* get_impl() const
    {
        return &static_cast<shared_data<U>*>(workspace_manager_response_base::get_impl())->data;
    }

public:
    workspace_manager_response() = default;
    template<typename U>
    explicit workspace_manager_response(U u) requires(!detail::is_in_place_type_t<U>::value)
        : workspace_manager_response_base(new shared_data<U>(std::move(u)), &get_actions<U>)
    {}
    template<typename U, typename... Args>
    explicit workspace_manager_response(std::in_place_type_t<U> u, Args&&... args)
        : workspace_manager_response_base(
            new shared_data<U>(std::move(u), std::forward<Args>(args)...), &get_actions<U>)
    {}

    using workspace_manager_response_base::error;
    using workspace_manager_response_base::invalidate;
    using workspace_manager_response_base::remove_invalidation_handler;
    using workspace_manager_response_base::resolved;
    using workspace_manager_response_base::set_invalidation_callback;
    using workspace_manager_response_base::valid;

    void provide(T t) const { workspace_manager_response_base::provide(&t); }

    friend decltype(make_workspace_manager_response);
};

} // namespace hlasm_plugin::parser_library

#endif
