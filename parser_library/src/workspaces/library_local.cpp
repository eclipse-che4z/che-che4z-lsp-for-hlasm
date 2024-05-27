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
#include <utility>

#include "file_manager.h"
#include "utils/platform.h"
#include "utils/projectors.h"
#include "utils/string_operations.h"
#include "wildcard.h"

namespace hlasm_plugin::parser_library::workspaces {

namespace {
void adjust_extensions_vector(std::vector<std::string>& extensions)
{
    for (auto& ext : extensions)
        if (!ext.empty() && ext[0] != '.')
            ext.insert(0, 1, '.');

    // from longest to shortest, then lexicographically
    std::ranges::sort(extensions, [](const std::string& l, const std::string& r) {
        if (auto c = l.size() <=> r.size(); c != 0)
            return c > 0;
        return l < r;
    });
    extensions.erase(std::ranges::unique(extensions).begin(), extensions.end());
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
    , m_err_loc(std::move(proc_grps_loc))
{
    if (m_extensions.size())
        adjust_extensions_vector(m_extensions);
}

library_local::library_local(library_local&& l) noexcept
    : m_file_manager(l.m_file_manager)
    , m_lib_loc(std::move(l.m_lib_loc))
    , m_files_collection(l.m_files_collection.exchange(nullptr))
    , m_extensions(std::move(l.m_extensions))
    , m_optional(l.m_optional)
    , m_err_loc(std::move(l.m_err_loc))
{}

utils::task library_local::refresh()
{
    return m_file_manager.list_directory_files(m_lib_loc).then([this](auto res) { load_files(std::move(res)); });
}

utils::task library_local::prefetch()
{
    if (m_files_collection.load())
        return {};
    else
        return refresh();
}

std::vector<std::string> library_local::list_files()
{
    auto files = m_files_collection.load();
    if (!files)
        return {};

    std::vector<std::string> result;
    result.reserve(files->first.size());
    std::ranges::transform(files->first, std::back_inserter(result), utils::first_element);
    return result;
}

const utils::resource::resource_location& library_local::get_location() const { return m_lib_loc; }

bool library_local::has_file(std::string_view file, utils::resource::resource_location* url)
{
    auto files = m_files_collection.load();
    if (!files)
        return {};
    auto it = files->first.find(file);
    if (it == files->first.end())
        return false;

    if (url)
        *url = it->second;

    return true;
}

void library_local::copy_diagnostics(std::vector<diagnostic>& target) const
{
    if (auto files = m_files_collection.load(); files)
        target.insert(target.end(), files->second.begin(), files->second.end());
}

bool library_local::has_cached_content() const { return m_files_collection.load() != nullptr; }

library_local::files_collection_t library_local::load_files(
    std::pair<std::vector<std::pair<std::string, utils::resource::resource_location>>, utils::path::list_directory_rc>
        res)
{
    auto& [files_list, rc] = res;
    auto new_state = std::make_shared<std::pair<std::unordered_map<std::string,
                                                    utils::resource::resource_location,
                                                    utils::hashers::string_hasher,
                                                    std::equal_to<>>,
        std::vector<diagnostic>>>();
    auto& [new_files, new_diags] = *new_state;

    switch (rc)
    {
        case hlasm_plugin::utils::path::list_directory_rc::done:
            break;
        case hlasm_plugin::utils::path::list_directory_rc::not_exists:
            if (!m_optional)
                new_diags.push_back(error_L0002(m_err_loc, m_lib_loc));
            break;
        case hlasm_plugin::utils::path::list_directory_rc::not_a_directory:
            new_diags.push_back(error_L0002(m_err_loc, m_lib_loc));
            break;
        case hlasm_plugin::utils::path::list_directory_rc::other_failure:
            new_diags.push_back(error_L0001(m_err_loc, m_lib_loc));
            break;
    }

    size_t conflict_count = 0;
    std::string file_name_conflicts;
    const auto add_conflict = [&conflict_count, &file_name_conflicts](std::string_view file_name) {
        constexpr const size_t max_conflict_count = 3;
        if (conflict_count < max_conflict_count)
        {
            if (conflict_count)
                file_name_conflicts.append(", ");
            file_name_conflicts.append(file_name);
        }
        else if (conflict_count == max_conflict_count)
            file_name_conflicts.append(" and others");
        ++conflict_count;
    };

    for (auto& [file, rl] : files_list)
    {
        if (m_extensions.empty())
        {
            // ".hidden" is not an extension ------v
            if (auto off = file.find_first_of('.', 1); off != std::string::npos)
                file.erase(off);
        }
        else if (auto ext = std::ranges::find_if(m_extensions, [&f = file](const auto& e) { return f.ends_with(e); });
                 ext != m_extensions.end())

            file.erase(file.size() - ext->size());
        else
            continue;

        utils::to_upper(file);

        if (auto [it, inserted] = new_files.try_emplace(std::move(file), std::move(rl)); !inserted)
        {
            // file, rl was not moved
            add_conflict(file);

            // keep shortest (i.e. without extension for compatibility) or lexicographically smaller
            const std::string_view rl_uri(rl.get_uri());
            const std::string_view old_uri(it->second.get_uri());
            if (std::pair(rl_uri.size(), rl_uri) < std::pair(old_uri.size(), old_uri))
                it->second = std::move(rl);
        }
    }

    if (conflict_count > 0)
        new_diags.push_back(warning_L0004(m_err_loc, m_lib_loc, file_name_conflicts, !m_extensions.empty()));

    m_files_collection.store(new_state);

    return new_state;
}

} // namespace hlasm_plugin::parser_library::workspaces
