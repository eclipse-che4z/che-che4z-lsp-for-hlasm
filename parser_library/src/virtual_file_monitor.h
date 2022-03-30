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

#ifndef HLASMPLUGIN_PARSERLIBRARY_VIRTUAL_FILE_MONITOR_H
#define HLASMPLUGIN_PARSERLIBRARY_VIRTUAL_FILE_MONITOR_H

#include <memory>
#include <string_view>
#include <utility>

#include "tagged_index.h"

namespace hlasm_plugin::parser_library {
class virtual_file_monitor;
using virtual_file_id = index_t<virtual_file_monitor, unsigned long long>;

class virtual_file_handle
{
    std::shared_ptr<const virtual_file_id> handle;

public:
    virtual_file_handle() = default;
    explicit virtual_file_handle(std::shared_ptr<const virtual_file_id> id)
        : handle(std::move(id))
    {}
    virtual_file_id file_id() const
    {
        if (handle)
            return *handle;
        else
            return virtual_file_id();
    }
};

class virtual_file_monitor
{
public:
    // notifies the monitor that a new virtual file has been produced
    virtual virtual_file_handle file_generated(std::string_view content) = 0;

protected:
    ~virtual_file_monitor() = default;
};

} // namespace hlasm_plugin::parser_library

#endif