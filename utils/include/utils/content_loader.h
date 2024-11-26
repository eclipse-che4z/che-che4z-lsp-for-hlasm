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

#ifndef HLASMPLUGIN_UTILS_CONTENT_LOADER_H
#define HLASMPLUGIN_UTILS_CONTENT_LOADER_H

#include <limits>
#include <optional>
#include <string>
#include <system_error>
#include <vector>

#include "list_directory_rc.h"
#include "resource_location.h"

namespace hlasm_plugin::utils::resource {

using list_directory_result =
    std::pair<std::vector<std::pair<std::string, utils::resource::resource_location>>, utils::path::list_directory_rc>;

using translation_table = std::array<char16_t, (1 << std::numeric_limits<unsigned char>::digits)>;

class content_loader
{
public:
    // Loads text
    virtual std::optional<std::string> load_text(const resource_location& res_loc) const = 0;

    // Returns list of all files in a directory. Returns associative array with pairs file name - file location.
    virtual list_directory_result list_directory_files(
        const utils::resource::resource_location& directory_loc) const = 0;

    // Returns list of all sub directories and symbolic links. Returns associative array with pairs {canonical path -
    // file location}.
    virtual list_directory_result list_directory_subdirs_and_symlinks(
        const utils::resource::resource_location& directory_loc) const = 0;

    // Returns file name
    virtual std::string filename(const utils::resource::resource_location& res_loc) const = 0;

    // Gets canonical representation if possible
    virtual std::string canonical(const utils::resource::resource_location& res_loc, std::error_code& ec) const = 0;

    virtual void set_translation_table(const translation_table* t) = 0;

protected:
    ~content_loader() = default;
};

content_loader& get_content_loader();
void set_content_loader_translation(const translation_table* translation, content_loader& cl = get_content_loader());
std::optional<std::string> load_text(const resource_location& res_loc, const content_loader& cl = get_content_loader());
list_directory_result list_directory_files(
    const utils::resource::resource_location& directory_loc, const content_loader& cl = get_content_loader());
list_directory_result list_directory_subdirs_and_symlinks(
    const utils::resource::resource_location& directory_loc, const content_loader& cl = get_content_loader());
std::string filename(
    const utils::resource::resource_location& res_loc, const content_loader& cl = get_content_loader());
std::string canonical(const utils::resource::resource_location& res_loc,
    std::error_code& ec,
    const content_loader& cl = get_content_loader());

} // namespace hlasm_plugin::utils::resource

#endif
