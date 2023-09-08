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

#include "utils/filesystem_content_loader.h"

#include "utils/path.h"
#include "utils/path_conversions.h"
#include "utils/platform.h"
#include "utils/unicode_text.h"

namespace hlasm_plugin::utils::resource {

std::optional<std::string> filesystem_content_loader::load_text(const resource_location& resource) const
{
    return platform::read_file(resource.get_path());
}

list_directory_result filesystem_content_loader::list_directory_files(
    const utils::resource::resource_location& directory_loc) const
{
    std::filesystem::path path(directory_loc.get_path());
    list_directory_result result;

    result.second = utils::path::list_directory_regular_files(path, [&result](const std::filesystem::path& f) {
        result.first.emplace_back(utils::path::filename(f).string(),
            utils::resource::resource_location(utils::path::path_to_uri(utils::path::absolute(f).string())));
    });

    return result;
}

list_directory_result filesystem_content_loader::list_directory_subdirs_and_symlinks(
    const utils::resource::resource_location& directory_loc) const
{
    std::filesystem::path path(directory_loc.get_path());
    list_directory_result result;

    result.second = utils::path::list_directory_subdirs_and_symlinks(path, [&result](const std::filesystem::path& p) {
        std::error_code ec;
        auto cp = utils::path::canonical(p, ec);

        if (!ec && utils::path::is_directory(cp))
        {
            auto cp_str = cp.string();
            auto found_dir = utils::resource::resource_location(utils::path::path_to_uri(cp_str));
            found_dir.join(""); // Ensure that this is a directory
            result.first.emplace_back(std::move(cp_str), std::move(found_dir));
        }
    });

    return result;
}

std::string filesystem_content_loader::filename(const utils::resource::resource_location& res_loc) const
{
    return utils::path::filename(std::filesystem::path(res_loc.get_path())).string();
}

std::string filesystem_content_loader::canonical(
    const utils::resource::resource_location& res_loc, std::error_code& ec) const
{
    return utils::path::canonical(res_loc.get_path(), ec).string();
}

} // namespace hlasm_plugin::utils::resource
