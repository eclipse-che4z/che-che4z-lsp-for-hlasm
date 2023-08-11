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

#ifndef HLASMPLUGIN_UTILS_FILTER_VECTOR_H
#define HLASMPLUGIN_UTILS_FILTER_VECTOR_H

#include <array>
#include <concepts>
#include <cstddef>
#include <limits>
#include <vector>

namespace hlasm_plugin::utils {

template<std::unsigned_integral T>
class filter_vector
{
    static constexpr size_t bit_count = std::numeric_limits<T>::digits;
    static constexpr size_t bucket_count = bit_count - 1;
    static constexpr T one = 1;
    std::array<std::vector<T>, 1 + bucket_count> filters;

    static constexpr auto deconstruct_value(size_t v)
    {
        struct result_t
        {
            size_t bucket;
            size_t bit;
        };
        return result_t { 1 + (v / bit_count) % bucket_count, v % bit_count };
    }

    class global_reset_accumulator
    {
        friend class filter_vector;

        std::array<T, 1 + bucket_count> values = {};

    public:
        void reset(size_t v)
        {
            const auto [bucket, bit] = deconstruct_value(v);
            values[bucket] |= one << bit;
        }
    };

public:
    global_reset_accumulator get_global_reset_accumulator() const { return {}; }

    static constexpr size_t effective_bit_count = bucket_count * bit_count;

    std::array<T, bucket_count> get(size_t idx) noexcept
    {
        std::array<T, bucket_count> result;
        for (size_t bucket = 1; bucket < filters.size(); ++bucket)
            result[bucket - 1] = filters[bucket][idx];
        return result;
    }

    void set(const std::array<T, bucket_count>& bits, size_t idx) noexcept
    {
        T summary = 0;
        for (size_t bucket = 1; bucket < filters.size(); ++bucket)
        {
            filters[bucket][idx] = bits[bucket - 1];
            summary |= static_cast<T>(!!bits[bucket - 1]) << bucket;
        }
        summary |= !!summary;
        filters[0][idx] = summary;
    }

    void assign(size_t to, size_t from) noexcept
    {
        for (auto& f : filters)
            f[to] = f[from];
    }

    bool get(size_t v, size_t idx) noexcept
    {
        const auto [bucket, bit] = deconstruct_value(v);
        return filters[bucket][idx] & (one << bit);
    }

    void set(size_t v, size_t idx) noexcept
    {
        const auto [bucket, bit] = deconstruct_value(v);
        filters[bucket][idx] |= one << bit;
        filters[0][idx] |= one << bucket | one;
    }

    void reset(size_t v, size_t idx) noexcept
    {
        const auto [bucket, bit] = deconstruct_value(v);
        filters[bucket][idx] &= ~(one << bit);
        filters[0][idx] &= ~(static_cast<T>(!filters[bucket][idx]) << bucket);
        filters[0][idx] &= -!!(filters[0][idx] & ~one);
    }

    void reset(size_t idx) noexcept
    {
        for (auto& f : filters)
            f[idx] = 0;
    }

    void reset_global(size_t v) noexcept
    {
        const auto [bucket, bit] = deconstruct_value(v);
        const T keep_on_mask = ~(one << bit);
        const T summary_test_bit = one << bucket;

        for (auto it = filters[bucket].begin(); auto& sum : filters[0])
        {
            auto& b = *it++;
            if (!(sum & summary_test_bit))
                continue;
            b &= keep_on_mask;
            sum &= ~(static_cast<T>(!b) << bucket);
            sum &= -!!(sum & ~one);
        }
    }

    void reset_global(const global_reset_accumulator& acc) noexcept
    {
        for (size_t bucket = 1; bucket < filters.size(); ++bucket)
        {
            if (acc.values[bucket] == 0)
                continue;
            const T keep_on_mask = ~acc.values[bucket];
            const T summary_test_bit = one << bucket;

            for (auto it = filters[bucket].begin(); auto& sum : filters[0])
            {
                auto& b = *it++;
                if (!(sum & summary_test_bit))
                    continue;
                b &= keep_on_mask;
                sum &= ~(static_cast<T>(!b) << bucket);
                sum &= -!!(sum & ~one);
            }
        }
    }

    bool any(size_t idx) const noexcept { return filters[0][idx] & one; }

    void emplace_back()
    {
        for (auto& f : filters)
            f.emplace_back(0);
    }

    void pop_back() noexcept
    {
        for (auto& f : filters)
            f.pop_back();
    }

    void swap(size_t l, size_t r) noexcept
    {
        if (((filters[0][l] | filters[0][r]) & one) == 0)
            return;
        for (auto& f : filters)
            std::swap(f[l], f[r]);
    }

    auto size() const noexcept { return filters.front().size(); }

    void clear() noexcept
    {
        for (auto& f : filters)
            f.clear();
    }
};

} // namespace hlasm_plugin::utils

#endif
