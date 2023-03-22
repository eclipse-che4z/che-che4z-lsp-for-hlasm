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

#ifndef HLASMPLUGIN_UTILS_FACTORY_H
#define HLASMPLUGIN_UTILS_FACTORY_H

namespace hlasm_plugin::utils {

template<typename T>
class factory
{
    T m_f;

public:
    explicit factory(T f)
        : m_f(static_cast<T&&>(f))
    {}

    explicit(false) operator decltype(m_f())() const { return m_f(); }
};


} // namespace hlasm_plugin::utils

#endif
