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

#ifndef HLASMPLUGIN_LANGUAGESERVER_SCOPE_EXIT_H
#define HLASMPLUGIN_LANGUAGESERVER_SCOPE_EXIT_H

namespace hlasm_plugin::language_server {
template<typename T>
class scope_exit
{
    T scope_exit_;

public:
    explicit scope_exit(T&& t)
        : scope_exit_(std::move(t))
    {}
    scope_exit(const scope_exit&) = delete;
    ~scope_exit() { scope_exit_(); }
};
} // namespace hlasm_plugin::language_server

#endif // HLASMPLUGIN_LANGUAGESERVER_SCOPE_EXIT_H
