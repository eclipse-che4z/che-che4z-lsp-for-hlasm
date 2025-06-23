/*
 * Copyright (c) 2025 Broadcom.
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

#ifndef HLASMPLUGIN_UTILS_INSIST_H
#define HLASMPLUGIN_UTILS_INSIST_H

namespace hlasm_plugin::utils {

[[noreturn]] void insist_fail(const char* explanation) noexcept;

constexpr void insist(bool test, const char* explanation = nullptr) noexcept
{
    if (!test)
        insist_fail(explanation);
}

} // namespace hlasm_plugin::utils

#endif
