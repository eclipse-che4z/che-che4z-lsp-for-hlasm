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

#ifndef HLASMPLUGIN_UTILS_MERGE_SORTED_H
#define HLASMPLUGIN_UTILS_MERGE_SORTED_H

#include <algorithm>
#include <compare>
#include <concepts>
#include <functional>
#include <iterator>
#include <type_traits>
#include <utility>
#include <vector>

namespace hlasm_plugin::utils {

struct trivial_merger
{
    void operator()(auto&&, const auto&) const noexcept {}
};

template<typename T, typename TargetVector, typename SourceIterator>
concept merge_key_comparator =
    std::is_same_v<std::strong_ordering,
        std::invoke_result_t<T, typename TargetVector::reference, typename TargetVector::reference>>
    && std::is_same_v<std::strong_ordering,
        std::invoke_result_t<T,
            typename TargetVector::reference,
            typename std::iterator_traits<SourceIterator>::reference>>;

template<typename T, typename TargetVector, typename SourceIterator>
concept merge_updater =
    std::invocable<T, typename TargetVector::reference, typename std::iterator_traits<SourceIterator>::reference>;

// KeyComparator should not alter the objects in any way
// Merger should not alter ordering established by the KeyComparator

template<typename T,
    /* std::forward_iterator - move_iterator is viewed as input_iterator only before libc++17 (and possibly c++23) */
    typename It,
    std::sentinel_for<It> S,
    merge_key_comparator<std::vector<T>, It> KeyComparator = std::compare_three_way,
    merge_updater<std::vector<T>, It> Merger = trivial_merger>
void merge_sorted(
    std::vector<T>& sorted_vec, It it, const S ite, KeyComparator&& cmp = KeyComparator(), Merger&& m = Merger())
{
    constexpr const auto trivial_inserter = !std::invocable<Merger, typename std::iterator_traits<It>::reference>;
    auto&& p = [&m]() -> std::conditional_t<trivial_inserter, std::identity, Merger&> {
        if constexpr (trivial_inserter)
            return std::identity();
        else
            return m;
    }();
    const auto init_size = sorted_vec.size();
    for (size_t i = 0; i < init_size && it != ite;)
    {
        auto&& el = sorted_vec[i];
        if (auto c = std::invoke(cmp, el, *it); c == 0)
        {
            std::invoke(m, el, *it);
            ++i;
            ++it;
        }
        else if (c > 0)
            sorted_vec.push_back(std::invoke(p, *it++));
        else
            ++i;
    }

    if constexpr (trivial_inserter)
        sorted_vec.insert(sorted_vec.end(), it, ite);
    else
        std::ranges::transform(it, ite, std::back_inserter(sorted_vec), std::ref(p));

    const auto less = [&cmp](const auto& l, const auto& r) { return std::invoke(cmp, l, r) < 0; };
    std::inplace_merge(sorted_vec.begin(), sorted_vec.begin() + init_size, sorted_vec.end(), less);
}

template<typename T,
    typename R,
    merge_key_comparator<std::vector<T>, decltype(std::begin(std::declval<R>()))> KeyComparator =
        std::compare_three_way,
    merge_updater<std::vector<T>, decltype(std::begin(std::declval<R>()))> Merger = trivial_merger>
void merge_sorted(
    std::vector<T>& sorted_vec, R&& sorted_range, KeyComparator&& cmp = KeyComparator(), Merger&& m = Merger())
{
    merge_sorted(sorted_vec,
        std::begin(sorted_range),
        std::end(sorted_range),
        std::forward<KeyComparator>(cmp),
        std::forward<Merger>(m));
}

template<typename T,
    /* std::forward_iterator - move_iterator is viewed as input_iterator only before libc++17 (and possibly c++23) */
    typename It,
    std::sentinel_for<It> S,
    merge_key_comparator<std::vector<T>, It> KeyComparator = std::compare_three_way,
    merge_updater<std::vector<T>, It> Merger = trivial_merger>
void merge_unsorted(
    std::vector<T>& sorted_vec, It it, const S ite, KeyComparator&& cmp = KeyComparator(), Merger&& m = Merger())
{
    constexpr const auto trivial_inserter = !std::invocable<Merger, typename std::iterator_traits<It>::reference>;
    auto&& p = [&m]() -> std::conditional_t<trivial_inserter, std::identity, Merger&> {
        if constexpr (trivial_inserter)
            return std::identity();
        else
            return m;
    }();
    const auto less = [&cmp](const auto& l, const auto& r) { return std::invoke(cmp, l, r) < 0; };
    const auto init_size = sorted_vec.size();
    for (; it != ite; ++it)
    {
        auto match = std::lower_bound(sorted_vec.begin(), sorted_vec.begin() + init_size, *it, less);
        if (match != sorted_vec.begin() + init_size && std::invoke(cmp, *match, *it) == 0)
            std::invoke(m, *match, *it);
        else
            sorted_vec.push_back(std::invoke(p, *it));
    }

    if constexpr (trivial_inserter)
        sorted_vec.insert(sorted_vec.end(), it, ite);
    else
        std::ranges::transform(it, ite, std::back_inserter(sorted_vec), std::ref(p));

    std::stable_sort(sorted_vec.begin() + init_size, sorted_vec.end(), less);
    std::inplace_merge(sorted_vec.begin(), sorted_vec.begin() + init_size, sorted_vec.end(), less);
}

template<typename T,
    typename R,
    merge_key_comparator<std::vector<T>, decltype(std::begin(std::declval<R>()))> KeyComparator =
        std::compare_three_way,
    merge_updater<std::vector<T>, decltype(std::begin(std::declval<R>()))> Merger = trivial_merger>
void merge_unsorted(
    std::vector<T>& sorted_vec, R&& unsorted_range, KeyComparator&& cmp = KeyComparator(), Merger&& m = Merger())
{
    merge_unsorted(sorted_vec,
        std::begin(unsorted_range),
        std::end(unsorted_range),
        std::forward<KeyComparator>(cmp),
        std::forward<Merger>(m));
}

} // namespace hlasm_plugin::utils

#endif
