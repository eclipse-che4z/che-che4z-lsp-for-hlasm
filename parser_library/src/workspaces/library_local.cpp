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

#include "library_local.h"

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <locale>

#include "utils/path.h"
#include "utils/platform.h"
#include "wildcard.h"

namespace hlasm_plugin::parser_library::workspaces {

namespace {
void adjust_extensions_vector(std::vector<std::string>& extensions, bool extensions_from_deprecated_source)
{
    bool contains_empty = false;
    for (auto& ext : extensions)
    {
        if (ext.empty())
            contains_empty = true;
        else
        {
            if (ext[0] != '.')
                ext.insert(0, 1, '.');
        }
    }
    // from longest to shortest, then lexicographically
    std::sort(extensions.begin(), extensions.end(), [](const std::string& l, const std::string& r) {
        if (l.size() > r.size())
            return true;
        if (l.size() < r.size())
            return false;
        return l < r;
    });
    if (extensions_from_deprecated_source && !contains_empty)
        extensions.emplace_back(); // alwaysRecognize always implied accepting files without an extension
    extensions.erase(std::unique(extensions.begin(), extensions.end()), extensions.end());
}
} // namespace

library_local::library_local(file_manager& file_manager,
    utils::resource::resource_location lib_loc,
    library_local_options options,
    utils::resource::resource_location proc_grps_loc)
    : m_file_manager(file_manager)
    , m_lib_loc(std::move(lib_loc))
    , m_extensions(std::move(options.extensions))
    , m_optional(options.optional_library)
    , m_extensions_from_deprecated_source(options.extensions_from_deprecated_source)
    , m_proc_grps_loc(std::move(proc_grps_loc))
{
    if (m_extensions.size())
        adjust_extensions_vector(m_extensions, m_extensions_from_deprecated_source);
}

library_local::library_local(library_local&& l) noexcept
    : m_file_manager(l.m_file_manager)
    , m_lib_loc(std::move(l.m_lib_loc))
    , m_files(std::move(l.m_files))
    , m_extensions(std::move(l.m_extensions))
    , m_files_loaded(l.m_files_loaded)
    , m_optional(l.m_optional)
    , m_extensions_from_deprecated_source(l.m_extensions_from_deprecated_source)
    , m_proc_grps_loc(std::move(l.m_proc_grps_loc))
{}

void library_local::collect_diags() const
{
    // does not have any diagnosable children
}

void library_local::refresh()
{
    m_files.clear();
    load_files();
}

std::vector<std::string> library_local::list_files()
{
    if (!m_files_loaded)
        load_files();

    std::vector<std::string> result;
    result.reserve(m_files.size());
    std::transform(m_files.begin(), m_files.end(), std::back_inserter(result), [](const auto& f) { return f.first; });
    return result;
}

const utils::resource::resource_location& library_local::get_location() const { return m_lib_loc; }

std::shared_ptr<processor> library_local::find_file(const std::string& file_name)
{
    if (!m_files_loaded)
        load_files();

    if (auto found = m_files.find(file_name); found != m_files.end())
        return m_file_manager.add_processor_file(found->second);
    else
        return nullptr;
}

void library_local::load_files()
{
    auto [files_list, rc] = m_file_manager.list_directory_files(m_lib_loc);
    m_files.clear();
    diags().clear();

    switch (rc)
    {
        case hlasm_plugin::utils::path::list_directory_rc::done:
            break;
        case hlasm_plugin::utils::path::list_directory_rc::not_exists:
            if (!m_optional)
                add_diagnostic(diagnostic_s::error_L0002(m_proc_grps_loc, m_lib_loc));
            break;
        case hlasm_plugin::utils::path::list_directory_rc::not_a_directory:
            add_diagnostic(diagnostic_s::error_L0002(m_proc_grps_loc, m_lib_loc));
            break;
        case hlasm_plugin::utils::path::list_directory_rc::other_failure:
            add_diagnostic(diagnostic_s::error_L0001(m_proc_grps_loc, m_lib_loc));
            break;
    }

    bool extension_removed = false;
    for (auto& [file, rl] : files_list)
    {
        if (m_extensions.empty())
        {
            m_files[context::to_upper_copy(file)] = std::move(rl);
            continue;
        }

        for (const auto& extension : m_extensions)
        {
            std::string_view filename(file);

            if (filename.size() <= extension.size())
                continue;

            if (filename.substr(filename.size() - extension.size()) != extension)
                continue;
            filename.remove_suffix(extension.size());

            const auto [_, inserted] = m_files.try_emplace(context::to_upper_copy(std::string(filename)), rl);
            // TODO: the stored value is a full path, yet we try to interpret it as a relative one later on
            if (!inserted)
                add_diagnostic(diagnostic_s::warning_L0004(
                    m_proc_grps_loc, m_lib_loc, context::to_upper_copy(std::string(filename))));

            if (extension.size())
                extension_removed = true;
            break;
        }
    }
    if (extension_removed && m_extensions_from_deprecated_source)
        add_diagnostic(diagnostic_s::warning_L0003(m_proc_grps_loc, m_lib_loc));

    m_files_loaded = true;
}

} // namespace hlasm_plugin::parser_library::workspaces
