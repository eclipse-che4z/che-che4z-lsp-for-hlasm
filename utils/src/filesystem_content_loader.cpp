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

#include <fstream>

#include "utils/path.h"
#include "utils/path_conversions.h"
#include "utils/utf8text.h"

namespace hlasm_plugin::utils::resource {

std::optional<std::string> filesystem_content_loader::load_text(const resource_location& resource) const
{
    std::ifstream fin(resource.get_path(), std::ios::in | std::ios::binary);
    std::string text;

    if (fin)
    {
        try
        {
            fin.seekg(0, std::ios::end);
            auto file_size = fin.tellg();

            if (file_size == -1)
                return std::nullopt;

            text.resize(file_size);
            fin.seekg(0, std::ios::beg);
            fin.read(&text[0], text.size());
            fin.close();

            return text;
        }
        catch (const std::exception&)
        {
            return std::nullopt;
        }
    }
    else
    {
        return std::nullopt;
    }
}

list_directory_result filesystem_content_loader::list_directory_files(
    const utils::resource::resource_location& directory_loc) const
{
    std::filesystem::path lib_p(directory_loc.get_path());
    list_directory_result result;

    result.second = utils::path::list_directory_regular_files(lib_p, [&result](const std::filesystem::path& f) {
        result.first[utils::path::filename(f).string()] =
            utils::resource::resource_location(utils::path::path_to_uri(utils::path::absolute(f).string()));
    });

    return result;
}

std::string filesystem_content_loader::filename(const utils::resource::resource_location& res_loc) const
{
    return utils::path::filename(std::filesystem::path(res_loc.get_path())).string();
}

} // namespace hlasm_plugin::utils::resource