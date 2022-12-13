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
    , m_files_collection(l.m_files_collection.exchange(nullptr))
    , m_extensions(std::move(l.m_extensions))
    , m_optional(l.m_optional)
    , m_extensions_from_deprecated_source(l.m_extensions_from_deprecated_source)
    , m_proc_grps_loc(std::move(l.m_proc_grps_loc))
{}

void library_local::refresh() { load_files(); }

std::vector<std::string> library_local::list_files()
{
    auto files = get_or_load_files();

    std::vector<std::string> result;
    result.reserve(files->first.size());
    std::transform(
        files->first.begin(), files->first.end(), std::back_inserter(result), [](const auto& f) { return f.first; });
    return result;
}

std::string library_local::refresh_url_prefix() const { return m_lib_loc.get_uri(); }

const utils::resource::resource_location& library_local::get_location() const { return m_lib_loc; }

bool library_local::has_file(std::string_view file, utils::resource::resource_location* url)
{
    auto files = get_or_load_files();
    auto it = files->first.find(file);
    if (it == files->first.end())
        return false;

    if (url)
        *url = it->second;

    return true;
}

void library_local::copy_diagnostics(std::vector<diagnostic_s>& target) const
{
    if (auto files = m_files_collection.load(); files)
        target.insert(target.end(), files->second.begin(), files->second.end());
}

library_local::files_collection_t library_local::load_files()
{
    auto [files_list, rc] = m_file_manager.list_directory_files(m_lib_loc);
    auto new_state = std::make_shared<std::pair<std::unordered_map<std::string,
                                                    utils::resource::resource_location,
                                                    utils::hashers::string_hasher,
                                                    std::equal_to<>>,
        std::vector<diagnostic_s>>>();
    auto& [new_files, new_diags] = *new_state;

    switch (rc)
    {
        case hlasm_plugin::utils::path::list_directory_rc::done:
            break;
        case hlasm_plugin::utils::path::list_directory_rc::not_exists:
            if (!m_optional)
                new_diags.push_back(diagnostic_s::error_L0002(m_proc_grps_loc, m_lib_loc));
            break;
        case hlasm_plugin::utils::path::list_directory_rc::not_a_directory:
            new_diags.push_back(diagnostic_s::error_L0002(m_proc_grps_loc, m_lib_loc));
            break;
        case hlasm_plugin::utils::path::list_directory_rc::other_failure:
            new_diags.push_back(diagnostic_s::error_L0001(m_proc_grps_loc, m_lib_loc));
            break;
    }

    bool extension_removed = false;
    for (auto& [file, rl] : files_list)
    {
        if (m_extensions.empty())
        {
            new_files.try_emplace(context::to_upper_copy(file), std::move(rl));
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

            const auto [_, inserted] =
                new_files.try_emplace(context::to_upper_copy(std::string(filename)), std::move(rl));
            // TODO: the stored value is a full path, yet we try to interpret it as a relative one later on
            if (!inserted)
                new_diags.push_back(diagnostic_s::warning_L0004(
                    m_proc_grps_loc, m_lib_loc, context::to_upper_copy(std::string(filename))));

            if (extension.size())
                extension_removed = true;
            break;
        }
    }
    if (extension_removed && m_extensions_from_deprecated_source)
        new_diags.push_back(diagnostic_s::warning_L0003(m_proc_grps_loc, m_lib_loc));

    m_files_collection.store(new_state);

    return new_state;
}

library_local::files_collection_t library_local::get_or_load_files()
{
    if (auto files = m_files_collection.load(); files)
        return files;
    return load_files();
}

} // namespace hlasm_plugin::parser_library::workspaces
