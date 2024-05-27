/*
 * Copyright (c) 2024 Broadcom.
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

#ifndef HLASMPLUGIN_UTILS_PROJECTOR_H
#define HLASMPLUGIN_UTILS_PROJECTOR_H

#include <utility>

namespace hlasm_plugin::utils {

struct
{
    template<class T>
    auto&& operator()(T&& t) const noexcept
    {
        return std::forward<T>(t).first;
    }
} constexpr first_element;

struct
{
    template<class T>
    auto&& operator()(T&& t) const noexcept
    {
        return std::forward<T>(t).second;
    }
} constexpr second_element;

struct
{
    template<typename T>
    auto&& operator()(T&& t) const noexcept
    {
        return *std::forward<T>(t);
    }
} constexpr dereference;

} // namespace hlasm_plugin::utils

#endif
