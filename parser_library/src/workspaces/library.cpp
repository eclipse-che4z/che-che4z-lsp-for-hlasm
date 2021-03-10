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

#include "library.h"

#include <filesystem>
#include <locale>
#include <regex>

#include "nlohmann/json.hpp"

#include "wildcard.h"

namespace hlasm_plugin::parser_library::workspaces {

library_local::library_local(file_manager& file_manager,
    std::string lib_path,
    std::shared_ptr<const extension_regex_map> extensions,
    bool optional)
    : file_manager_(file_manager)
    , lib_path_(std::move(lib_path))
    , extensions_(std::move(extensions))
    , optional_(optional)
{}

library_local::library_local(library_local&& l) noexcept
    : file_manager_(l.file_manager_)
    , lib_path_(std::move(l.lib_path_))
    , files_(std::move(l.files_))
    , extensions_(std::move(l.extensions_))
    , files_loaded_(l.files_loaded_)
    , optional_(l.optional_)
{}

void library_local::collect_diags() const
{
    // does not have any diagnosable children
}

void library_local::refresh()
{
    files_.clear();
    load_files();
}

const std::string& library_local::get_lib_path() const { return lib_path_; }

std::shared_ptr<processor> library_local::find_file(const std::string& file_name)
{
    if (!files_loaded_)
        load_files();

    auto found = files_.find(file_name);
    if (found != files_.end())
    {
        std::filesystem::path lib_path(lib_path_);
        return file_manager_.add_processor_file((lib_path / found->second).string());
    }
    else
        return nullptr;
}


void library_local::load_files()
{
    auto files_list = file_manager_.list_directory_files(lib_path_, optional_);
    files_.clear();
    for (const auto& file : files_list)
    {
        bool added = false;

        for (const auto& extension : *extensions_)
        {
            // current file matches regex (it has extension)
            // e.g. file "files/open.hlasm" matches both extensions "files/*.hlasm" and "*.hlasm"
            if (extension.first.size() < file.second.size() && std::regex_match(file.second, extension.second))
            {
                files_[context::to_upper_copy(file.first.substr(0, file.first.size() - extension.first.size()))] =
                    file.second;
                added = true;
                break;
            }
        }
        if (!added)
            files_[context::to_upper_copy(file.first)] = file.second;
    }

    files_loaded_ = true;
}

} // namespace hlasm_plugin::parser_library::workspaces
