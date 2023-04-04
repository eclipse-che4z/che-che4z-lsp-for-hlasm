/*
 * Copyright (c) 2021 Broadcom.
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

#ifndef HLASMPLUGIN_UTILS_SCOPE_EXIT_H
#define HLASMPLUGIN_UTILS_SCOPE_EXIT_H

namespace hlasm_plugin::utils {
template<typename T>
concept scope_exit_handler = requires(T t)
{
    {
        t()
    }
    noexcept;
};

template<scope_exit_handler T>
class scope_exit
{
    T m_scope_exit;

public:
    explicit scope_exit(T&& t)
        : m_scope_exit(static_cast<T&&>(t))
    {}
    scope_exit(const scope_exit&) = delete;
    scope_exit(scope_exit&&) = delete;
    scope_exit& operator=(const scope_exit&) = delete;
    scope_exit& operator=(scope_exit&&) = delete;
    ~scope_exit() { m_scope_exit(); }
};
} // namespace hlasm_plugin::utils

#endif // HLASMPLUGIN_UTILS_SCOPE_EXIT_H
