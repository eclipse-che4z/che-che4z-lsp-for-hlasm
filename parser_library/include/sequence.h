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

#ifndef HLASMPLUGIN_PARSERLIBRARY_SEQUENCE_H
#define HLASMPLUGIN_PARSERLIBRARY_SEQUENCE_H

#include <iterator>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#pragma warning(push)
#pragma warning(disable : 4661)

namespace hlasm_plugin::parser_library {

template<typename T>
class sequence_iterator
{
    const T* m_ptr;
    size_t m_idx;

public:
    sequence_iterator(const T* ptr, size_t index) noexcept
        : m_ptr(ptr)
        , m_idx(index)
    {}

    auto operator*() const { return m_ptr->item(m_idx); }
    sequence_iterator& operator++() noexcept
    {
        m_idx++;
        return *this;
    }
    sequence_iterator operator++(int) noexcept
    {
        auto tmp = *this;
        ++(*this);
        return tmp;
    }
    friend bool operator==(const sequence_iterator& a, const sequence_iterator& b) noexcept
    {
        return a.m_ptr == b.m_ptr && a.m_idx == b.m_idx;
    }
    friend bool operator!=(const sequence_iterator& a, const sequence_iterator& b) noexcept
    {
        return a.m_ptr != b.m_ptr || a.m_idx != b.m_idx;
    }

    using iterator_category = std::input_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = decltype(m_ptr->item(0));
    using pointer = void;
    using reference = value_type;
};

template<typename T>
concept trivial_storage_sequence = requires(const T& t)
{
    typename T::sequence_trivial;
    {
        t.data()
    }
    noexcept;
    requires std::is_pointer_v<decltype(t.data())>;
    requires std::is_standard_layout_v<std::remove_pointer_t<decltype(t.data())>>;
};

template<typename T>
class continuous_storage
{
    void* to_delete = nullptr;
    void (*deleter)(void*) = nullptr;
    const T* m_data = nullptr;

public:
    continuous_storage() = default;
    template<typename U>
    explicit continuous_storage(U&& v)
        : to_delete(new std::decay_t<U>(std::forward<U>(v)))
        , deleter(+[](void* ptr) { delete (std::decay_t<U>*)ptr; })
        , m_data(((const std::decay_t<U>*)to_delete)->data())
    {}
    continuous_storage(const continuous_storage& o) = delete;
    continuous_storage(continuous_storage&& o) noexcept
        : to_delete(std::exchange(o.to_delete, nullptr))
        , deleter(o.deleter)
        , m_data(std::exchange(o.m_data, nullptr))
    {}
    continuous_storage& operator=(const continuous_storage&) = delete;
    continuous_storage& operator=(continuous_storage&& o) noexcept
    {
        continuous_storage tmp(std::move(o));
        swap(tmp);
        return *this;
    }
    ~continuous_storage()
    {
        if (to_delete)
            deleter(to_delete);
    }

    void swap(continuous_storage& o) noexcept
    {
        using std::swap;
        swap(to_delete, o.to_delete);
        swap(deleter, o.deleter);
        swap(m_data, o.m_data);
    }

    auto data() const noexcept { return m_data; }

    using sequence_trivial = void;
};

// Provides pimpl for arrays. The returned item can be
// converted to its exported representation in implementation
// of item. c_type is the exported type, storage is its
// implementation.
template<typename c_type, typename storage = void>
class sequence
{
    friend PARSER_LIBRARY_EXPORT c_type sequence_item_get(const sequence*, size_t);

    class counter
    {
        size_t size = 0;

    public:
        counter() = default;
        explicit counter(size_t s) noexcept
            : size(s)
        {}
        counter(const counter&) = default;
        counter(counter&& r) noexcept
            : size(std::exchange(r.size, 0))
        {}
        counter& operator=(const counter&) = default;
        counter& operator=(counter&& o) noexcept
        {
            size = std::exchange(o.size, 0);
            return *this;
        }
        operator size_t() const { return size; }
    };
    static constexpr bool owns_resources = !std::is_trivially_destructible_v<storage>;
    using counter_t = std::conditional_t<owns_resources, counter, size_t>;

public:
    sequence() = default;
    sequence(storage stor, size_t size) noexcept(std::is_nothrow_move_constructible_v<storage>)
        : stor_(std::move(stor))
        , size_(size)
    {}
    explicit operator std::vector<c_type>() const { return std::vector<c_type>(begin(), end()); }

    [[nodiscard]] c_type item(size_t index) const noexcept(trivial_storage_sequence<storage>)
    {
        if constexpr (trivial_storage_sequence<storage>)
            return data()[index];
        else
            return sequence_item_get(this, index);
    }

    [[nodiscard]] auto data() const noexcept requires(trivial_storage_sequence<storage>)
    {
        if constexpr (trivial_storage_sequence<storage>) // clang-12 :(
            return stor_.data();
    }
    [[nodiscard]] size_t size() const noexcept { return size_; }

    [[nodiscard]] auto begin() const noexcept
    {
        if constexpr (trivial_storage_sequence<storage>)
            return stor_.data();
        else
            return sequence_iterator(this, 0);
    }
    [[nodiscard]] auto end() const noexcept
    {
        if constexpr (trivial_storage_sequence<storage>)
            return stor_.data() + size();
        else
            return sequence_iterator(this, size());
    }

private:
    storage stor_ = storage();
    counter_t size_ = counter_t();
};

template<typename c_type>
using continuous_sequence = sequence<c_type, continuous_storage<c_type>>;

template<typename C>
auto make_continuous_sequence(C c)
{
    using c_type = std::decay_t<decltype(*c.data())>;

    if (const auto s = c.size(); s != 0)
        return continuous_sequence<c_type>(continuous_storage<c_type>(std::move(c)), s);
    else
        return continuous_sequence<c_type>();
}

// Specialization for simple arrays
// not all string_view implementations are abi-compatible.
template<typename c_type>
class sequence<c_type, void>
{
public:
    sequence() = default;
    sequence(const c_type* data, size_t size) noexcept
        : data_(data)
        , size_(size)
    {}
    template<typename U, typename = std::void_t<decltype(std::declval<U>().data()), decltype(std::declval<U>().size())>>
    explicit sequence(U&& sv) noexcept
        : data_(sv.data())
        , size_(sv.size())
    {}

    template<typename U,
        std::enable_if_t<
            std::is_same_v<std::basic_string_view<c_type>, U> || std::is_same_v<std::basic_string<c_type>, U>,
            int> = 0>
    explicit operator U() const
    {
        return U(data_, size_);
    }
    explicit operator std::vector<c_type>() const { return std::vector<c_type>(begin(), end()); }


    [[nodiscard]] c_type item(size_t index) const { return data_[index]; }
    [[nodiscard]] size_t size() const noexcept { return size_; }
    [[nodiscard]] const c_type* data() const noexcept { return data_; }

    [[nodiscard]] const c_type* begin() const noexcept { return data_; }
    [[nodiscard]] const c_type* end() const noexcept { return data_ + size_; }

private:
    const c_type* data_ = nullptr;
    size_t size_ = 0;
};
template<class T>
explicit sequence(T&&) -> sequence<std::decay_t<decltype(*std::declval<T>().data())>, void>;

} // namespace hlasm_plugin::parser_library

#pragma warning(pop)

#endif // !HLASMPLUGIN_PARSERLIBRARY_SEQUENCE_H
