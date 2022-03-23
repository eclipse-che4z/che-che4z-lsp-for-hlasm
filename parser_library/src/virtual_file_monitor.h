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

namespace hlasm_plugin::parser_library {

class virtual_file_monitor
{
public:
    virtual void file_generated(unsigned long long id, std::string_view content) = 0;

protected:
    ~virtual_file_monitor() = default;
};

} // namespace hlasm_plugin::parser_library

#endif