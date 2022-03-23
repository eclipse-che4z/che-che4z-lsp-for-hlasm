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

#ifndef HLASMPLUGIN_PARSERLIBRARY_VIRTUAL_FILE_MONITOR_H
#define HLASMPLUGIN_PARSERLIBRARY_VIRTUAL_FILE_MONITOR_H

#include <string_view>

#include "tagged_index.h"

namespace hlasm_plugin::parser_library {
class virtual_file_monitor;
using virtual_file_id = index_t<virtual_file_monitor, unsigned long long>;

class virtual_file_monitor
{
public:
    // notifies the monitor that a new virtual file has been produced
    virtual void file_generated(virtual_file_id id, std::string_view content) = 0;
    // notifies the monitor that the virtual file is no longer required
    virtual void file_released(virtual_file_id id) = 0;

protected:
    ~virtual_file_monitor() = default;
};

} // namespace hlasm_plugin::parser_library

#endif