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
    sequence_iterator(const T* ptr, size_t index)
        : m_ptr(ptr)
        , m_idx(index)
    {}

    auto operator*() const { return m_ptr->item(m_idx); }
    sequence_iterator& operator++()
    {
        m_idx++;
        return *this;
    }
    sequence_iterator operator++(int)
    {
        auto tmp = *this;
        ++(*this);
        return tmp;
    }
    friend bool operator==(const sequence_iterator& a, const sequence_iterator& b)
    {
        return a.m_ptr == b.m_ptr && a.m_idx == b.m_idx;
    };
    friend bool operator!=(const sequence_iterator& a, const sequence_iterator& b)
    {
        return a.m_ptr != b.m_ptr || a.m_idx != b.m_idx;
    };

    using iterator_category = std::input_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = decltype(m_ptr->item(0));
    using pointer = void;
    using reference = value_type;
};

// Provides pimpl for arrays. The returned item can be
// converted to its exported representation in implementation
// of item. c_type is the exported type, storage is its
// implementation.
template<typename c_type, typename storage = void>
class sequence
{
    class counter
    {
        size_t size = 0;

    public:
        counter() = default;
        counter(size_t s)
            : size(s)
        {}
        counter(const counter&) = default;
        counter(counter&& r) noexcept
            : size(std::exchange(r.size, 0))
        {}
        operator size_t() const { return size; }
    };
    static constexpr bool owns_resources = !std::is_trivially_destructible_v<storage>;
    using counter_t = std::conditional_t<owns_resources, counter, size_t>;

public:
    sequence() = default;
    sequence(const sequence&) = default;
    sequence(sequence&& s) = default;
    sequence(storage stor, size_t size)
        : stor_(std::move(stor))
        , size_(size)
    {}
    explicit operator std::vector<c_type>() const { return std::vector<c_type>(begin(), end()); }

    // needs to be specialized for every type
    [[nodiscard]] c_type item(size_t index) const;
    [[nodiscard]] size_t size() const { return size_; }

    [[nodiscard]] auto begin() const { return sequence_iterator(this, 0); }
    [[nodiscard]] auto end() const { return sequence_iterator(this, size()); }

private:
    storage stor_ = storage();
    counter_t size_ = counter_t();
};

// Specialization for simple arrays
// not all string_view implementations are abi-compatible.
template<typename c_type>
class sequence<c_type, void>
{
public:
    sequence() = default;
    sequence(const c_type* data, size_t size)
        : data_(data)
        , size_(size)
    {}
    template<typename U, typename = std::void_t<decltype(std::declval<U>().data()), decltype(std::declval<U>().size())>>
    explicit sequence(U&& sv)
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
    [[nodiscard]] size_t size() const { return size_; }
    [[nodiscard]] const c_type* data() const { return data_; }

    [[nodiscard]] const c_type* begin() const { return data_; }
    [[nodiscard]] const c_type* end() const { return data_ + size_; }

private:
    const c_type* data_ = nullptr;
    size_t size_ = 0;
};

// Specialization for std::string
template<typename c_type>
class sequence<c_type, std::string>
{
public:
    sequence() = default;
    sequence(std::string s)
        : data_(std::move(s))
    {}

    [[nodiscard]] c_type item(size_t index) const { return data_[index]; }
    [[nodiscard]] size_t size() const { return data_.size(); }
    [[nodiscard]] const c_type* data() const { return data_.data(); }

    [[nodiscard]] const c_type* begin() const { return data_.begin(); }
    [[nodiscard]] const c_type* end() const { return data_.end(); }

private:
    const std::string data_;
};
template<class T>
explicit sequence(T&&) -> sequence<std::decay_t<decltype(*std::declval<T>().data())>, void>;

} // namespace hlasm_plugin::parser_library

#pragma warning(pop)

#endif // !HLASMPLUGIN_PARSERLIBRARY_SEQUENCE_H
