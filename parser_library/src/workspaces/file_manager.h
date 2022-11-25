/*
 * Copyright (c) 2019 Broadcom.
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

#ifndef HLASMPLUGIN_PARSERLIBRARY_FILE_MANAGER_H
#define HLASMPLUGIN_PARSERLIBRARY_FILE_MANAGER_H

#include <filesystem>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "diagnosable.h"
#include "file.h"
#include "processor.h"
#include "utils/general_hashers.h"
#include "utils/list_directory_rc.h"
#include "utils/resource_location.h"

namespace hlasm_plugin::parser_library::workspaces {

using file_ptr = std::shared_ptr<file>;
using processor_file_ptr = std::shared_ptr<processor_file>;
using list_directory_result =
    std::pair<std::vector<std::pair<std::string, utils::resource::resource_location>>, utils::path::list_directory_rc>;

// Wraps an associative array of file names and files.
// Implements LSP text synchronization methods.
class file_manager : public virtual diagnosable
{
public:
    // Adds a file with specified file name and returns it.
    // If such processor file already exists, it is returned.
    virtual file_ptr add_file(const file_location&) = 0;

    // Adds processor file with specified file name and returns it.
    // If such processor file already exists, it is returned.
    // If such file already exists, it is changed into processor file.
    virtual processor_file_ptr add_processor_file(const file_location&) = 0;
    virtual processor_file_ptr get_processor_file(const file_location&) = 0;

    virtual void remove_file(const file_location&) = 0;

    // Finds file with specified file name, return nullptr if not found.
    virtual file_ptr find(const file_location& key) const = 0;
    // Finds processor file with specified file name.
    // If there is a file with the file name, it is changed to processor_file.
    // Returns nullptr if there is no such file.
    virtual processor_file_ptr find_processor_file(const file_location& key) = 0;

    // Returns list of all files in a directory. Returns associative array with pairs file name - file location.
    virtual list_directory_result list_directory_files(const utils::resource::resource_location& directory) const = 0;

    // Returns list of all sub directories and symbolic links. Returns associative array with pairs {canonical path -
    // file location}.
    // TODO Used as a shortcut for easier testing with mocks - refactor it out of this class together with canonical()
    virtual list_directory_result list_directory_subdirs_and_symlinks(
        const utils::resource::resource_location& directory) const = 0;

    virtual std::string canonical(const utils::resource::resource_location& res_loc, std::error_code& ec) const = 0;

    virtual bool dir_exists(const utils::resource::resource_location& dir_loc) const = 0;

    virtual void did_open_file(const file_location& document_loc, version_t version, std::string text) = 0;
    virtual void did_change_file(
        const file_location& document_loc, version_t version, const document_change* changes, size_t ch_size) = 0;
    virtual void did_close_file(const file_location& document_loc) = 0;

    virtual void put_virtual_file(
        unsigned long long id, std::string_view text, utils::resource::resource_location related_workspace) = 0;
    virtual void remove_virtual_file(unsigned long long id) = 0;
    virtual std::string get_virtual_file(unsigned long long id) const = 0;
    virtual utils::resource::resource_location get_virtual_file_workspace(unsigned long long id) const = 0;

protected:
    ~file_manager() = default;
};

} // namespace hlasm_plugin::parser_library::workspaces

#endif // FILE_MANAGER_H
