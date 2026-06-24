/*
 * Copyright (c) 2026 Broadcom.
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

#ifndef HLASMPLUGIN_UTILS_INTCONV_H
#define HLASMPLUGIN_UTILS_INTCONV_H

#include <concepts>
#include <type_traits>

namespace hlasm_plugin::utils {

[[nodiscard]] constexpr auto to_signed(std::integral auto v) noexcept
{
    return static_cast<std::make_signed_t<decltype(v)>>(v);
}

[[nodiscard]] constexpr auto to_unsigned(std::integral auto v) noexcept
{
    return static_cast<std::make_unsigned_t<decltype(v)>>(v);
}

} // namespace hlasm_plugin::utils

#endif
