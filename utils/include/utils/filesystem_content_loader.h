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

#ifndef HLASMPLUGIN_UTILS_FILESYSTEM_CONTENT_LOADER_H
#define HLASMPLUGIN_UTILS_FILESYSTEM_CONTENT_LOADER_H

#include <optional>

#include "utils/content_loader.h"
#include "utils/resource_location.h"

namespace hlasm_plugin::utils::resource {

class filesystem_content_loader : public content_loader
{
public:
    filesystem_content_loader() = default;
    virtual ~filesystem_content_loader() = default;

    std::optional<std::string> load_text(const resource_location& resource) const override;
    list_directory_result list_directory_files(const utils::resource::resource_location& directory_loc) const override;
    list_directory_result list_directory_subdirs_and_symlinks(
        const utils::resource::resource_location& directory_loc) const override;
    std::string filename(const utils::resource::resource_location& res_loc) const override;
    std::string canonical(const utils::resource::resource_location& res_loc, std::error_code& ec) const override;
};

} // namespace hlasm_plugin::utils::resource

#endif
