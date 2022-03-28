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

#ifndef HLASMPLUGIN_PARSERLIBRARY_FILE_MANAGER_VFM_H
#define HLASMPLUGIN_PARSERLIBRARY_FILE_MANAGER_VFM_H

#include <string_view>

#include "virtual_file_monitor.h"

namespace hlasm_plugin::parser_library::workspaces {
class file_manager;

struct file_manager_vfm final : public virtual_file_monitor
{
    file_manager& fm;

    // Inherited via virtual_file_monitor
    virtual_file_handle file_generated(std::string_view content) override;

    explicit file_manager_vfm(file_manager& fm)
        : fm(fm)
    {}

private:
    static unsigned long long next_virtual_file_id();
};

} // namespace hlasm_plugin::parser_library::workspaces

#endif
