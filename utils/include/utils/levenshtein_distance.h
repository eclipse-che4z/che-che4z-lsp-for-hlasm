/*
 * Copyright (c) 2022 Broadcom.
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

#ifndef HLASMPLUGIN_UTILS_LEVENSHTEIN_DISTANCE_H
#define HLASMPLUGIN_UTILS_LEVENSHTEIN_DISTANCE_H

#include <algorithm>
#include <array>
#include <cassert>
#include <iterator>
#include <numeric>
#include <vector>

namespace hlasm_plugin::utils {

template<size_t limit = 0>
struct levenshtein_distance_t
{
    /* constexpr */ size_t operator()(auto&& l, auto&& r) const noexcept(limit > 0)
    {
        using std::size;
        const auto l_size = size(l);
        const auto r_size = size(r);
        if (l_size < r_size)
            return operator()(r, l);
        if (r_size == 0)
            return l_size;

        std::conditional_t<limit == 0, std::vector<size_t>, std::array<size_t, 2 * (limit + 1)>> workarea;
        if constexpr (limit == 0)
            workarea.resize(2 * (r_size + 1));
        else
            assert(r_size <= limit);

        auto* cur_b = workarea.data();
        auto* next_b = workarea.data() + (limit ? limit : r_size) + 1;

        std::iota(cur_b, cur_b + r_size + 1, (size_t)0);

        for (const auto& l_e : l)
        {
            auto* cur = cur_b;
            auto* next = next_b;
            *next = *cur + 1;
            for (const auto& r_e : r)
            {
                *(next + 1) = std::min({ *(cur + 1) + 1, *next + 1, *cur + !(l_e == r_e) });

                ++cur;
                ++next;
            }
            std::swap(cur_b, next_b);
        }

        return cur_b[r_size];
    }
};

template<size_t limit>
requires(limit > 0) constexpr levenshtein_distance_t<limit> levenshtein_distance_n;
constexpr levenshtein_distance_t<0> levenshtein_distance;

} // namespace hlasm_plugin::utils


#endif