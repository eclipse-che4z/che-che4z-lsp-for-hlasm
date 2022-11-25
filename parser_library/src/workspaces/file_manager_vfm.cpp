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

#include "file_manager_vfm.h"

#include <atomic>
#include <memory>
#include <utility>

#include "file_manager.h"

namespace hlasm_plugin::parser_library::workspaces {

virtual_file_handle file_manager_vfm::file_generated(std::string_view content)
{
    struct result_t
    {
        file_manager& fm;
        virtual_file_id id;

        result_t(file_manager& fm, virtual_file_id id)
            : fm(fm)
            , id(id)
        {}
        result_t(const result_t&) = delete;
        result_t(result_t&&) = delete;
        result_t& operator=(const result_t&) = delete;
        result_t& operator=(result_t&&) = delete;
        ~result_t()
        {
            if (id)
                fm.remove_virtual_file(id.value());
        }
    };
    auto complete_handle = std::make_shared<result_t>(fm, virtual_file_id(next_virtual_file_id()));

    fm.put_virtual_file(complete_handle->id.value(), content, related_workspace);

    return virtual_file_handle(
        std::shared_ptr<const virtual_file_id>(std::move(complete_handle), &complete_handle->id));
}

unsigned long long file_manager_vfm::next_virtual_file_id()
{
    static std::atomic<unsigned long long> id = 0;
    return id++;
}

} // namespace hlasm_plugin::parser_library::workspaces
