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
#include <unordered_map>
#include <vector>

#include "diagnosable.h"
#include "file.h"
#include "processor.h"

namespace hlasm_plugin::parser_library::workspaces {

using file_ptr = std::shared_ptr<file>;
using processor_file_ptr = std::shared_ptr<processor_file>;

// Wraps an associative array of file names and files.
// Implements LSP text synchronization methods.
class file_manager : public virtual diagnosable
{
public:
    // Adds a file with specified file name and returns it.
    // If such processor file already exists, it is returned.
    virtual file_ptr add_file(const file_uri&) = 0;

    // Adds processor file with specified file name and returns it.
    // If such processor file already exists, it is returned.
    // If such file already exists, it is changed into processor file.
    virtual processor_file_ptr add_processor_file(const file_uri&) = 0;

    virtual void remove_file(const file_uri&) = 0;

    // Finds file with specified file name, return nullptr if not found.
    virtual file_ptr find(const std::string& key) = 0;
    // Finds processor file with specified file name.
    // If there is a file with the file name, it is changed to processor_file.
    // Returns nullptr if there is no such file.
    virtual processor_file_ptr find_processor_file(const std::string& key) = 0;

    // Returns list of all files in a directory. Returns associative array with pairs file path - file name.
    virtual std::unordered_map<std::string, std::string> list_directory_files(
        const std::string& path, bool optional) = 0;

    virtual bool file_exists(const std::string& file_name) = 0;
    virtual bool lib_file_exists(const std::string& lib_path, const std::string& file_name) = 0;

    virtual void did_open_file(const std::string& document_uri, version_t version, std::string text) = 0;
    virtual void did_change_file(
        const std::string& document_uri, version_t version, const document_change* changes, size_t ch_size) = 0;
    virtual void did_close_file(const std::string& document_uri) = 0;
};

} // namespace hlasm_plugin::parser_library::workspaces

#endif // FILE_MANAGER_H
