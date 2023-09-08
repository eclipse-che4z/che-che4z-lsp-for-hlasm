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

#include "utils/content_loader.h"

#include <memory>

#include "utils/filesystem_content_loader.h"

namespace hlasm_plugin::utils::resource {

const content_loader& get_content_loader()
{
    static auto fs_cl = filesystem_content_loader();

    // Todo Additional content loaders can be returned here (e.g. for network resources)
    return fs_cl;
}

std::optional<std::string> load_text(const resource_location& res_loc)
{
    const auto& cl = get_content_loader();

    return cl.load_text(res_loc);
};

list_directory_result list_directory_files(const utils::resource::resource_location& directory_loc)
{
    const auto& cl = get_content_loader();

    return cl.list_directory_files(directory_loc);
}

list_directory_result list_directory_subdirs_and_symlinks(const utils::resource::resource_location& directory_loc)
{
    const auto& cl = get_content_loader();

    return cl.list_directory_subdirs_and_symlinks(directory_loc);
}

std::string filename(const utils::resource::resource_location& res_loc)
{
    const auto& cl = get_content_loader();

    return cl.filename(res_loc);
}

std::string canonical(const utils::resource::resource_location& res_loc, std::error_code& ec)
{
    const auto& cl = get_content_loader();

    return cl.canonical(res_loc, ec);
}

} // namespace hlasm_plugin::utils::resource
