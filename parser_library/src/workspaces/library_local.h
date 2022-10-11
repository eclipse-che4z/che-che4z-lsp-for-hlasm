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

#ifndef HLASMPLUGIN_PARSERLIBRARY_LOCAL_LIBRARY_H
#define HLASMPLUGIN_PARSERLIBRARY_LOCAL_LIBRARY_H

#include <string>
#include <unordered_map>
#include <vector>

#include "diagnosable_impl.h"
#include "file_manager.h"
#include "library.h"

namespace hlasm_plugin::parser_library::workspaces {

#pragma warning(push)
#pragma warning(disable : 4250)

struct library_local_options
{
    std::vector<std::string> extensions;
    bool extensions_from_deprecated_source = false;
    bool optional_library = false;
};

// library holds absolute path to a directory and finds macro files in it
class library_local final : public library, public diagnosable_impl
{
public:
    // takes reference to file manager that provides access to the files
    // and normalised path to directory that it wraps.
    library_local(file_manager& file_manager,
        utils::resource::resource_location lib_loc,
        library_local_options options,
        utils::resource::resource_location proc_grps_loc);

    library_local(const library_local&) = delete;
    library_local& operator=(const library_local&) = delete;

    library_local(library_local&&) noexcept;

    void collect_diags() const override;

    const utils::resource::resource_location& get_location() const;

    std::shared_ptr<processor> find_file(const std::string& file) override;

    void refresh() override;

    bool is_once_only() const override { return false; }

private:
    file_manager& m_file_manager;

    utils::resource::resource_location m_lib_loc;
    std::unordered_map<std::string, utils::resource::resource_location, utils::hashers::string_hasher, std::equal_to<>>
        m_files;
    std::vector<std::string> m_extensions;
    // indicates whether load_files function was called (not whether it was successful)
    bool m_files_loaded = false;
    bool m_optional = false;
    bool m_extensions_from_deprecated_source = false;
    utils::resource::resource_location m_proc_grps_loc;

    void load_files();
};
#pragma warning(pop)

} // namespace hlasm_plugin::parser_library::workspaces
#endif
