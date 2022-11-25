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

#include "file_manager_impl.h"

#include <map>

#include "processor_file_impl.h"
#include "utils/content_loader.h"
#include "utils/path.h"
#include "utils/path_conversions.h"
#include "utils/platform.h"

namespace hlasm_plugin::parser_library::workspaces {

void file_manager_impl::collect_diags() const
{
    for (auto& it : files_)
    {
        collect_diags_from_child(*it.second);
    }
}

file_ptr file_manager_impl::add_file(const file_location& file)
{
    std::lock_guard guard(files_mutex);
    auto [ret, _] = files_.try_emplace(file, std::make_shared<file_impl>(file));
    return ret->second;
}

processor_file_ptr file_manager_impl::change_into_processor_file_if_not_already_(std::shared_ptr<file_impl>& to_change)
{
    auto processor = std::dynamic_pointer_cast<processor_file>(to_change);
    if (processor)
        return processor;
    else
    {
        auto proc_file = std::make_shared<processor_file_impl>(std::move(*to_change), *this, cancel_);
        to_change = proc_file;
        return proc_file;
    }
}

processor_file_ptr file_manager_impl::add_processor_file(const file_location& file)
{
    std::lock_guard guard(files_mutex);
    auto ret = files_.find(file);
    if (ret == files_.end())
    {
        auto ptr = std::make_shared<processor_file_impl>(file, *this, cancel_);
        files_.try_emplace(file, ptr);
        return ptr;
    }
    else
        return change_into_processor_file_if_not_already_(ret->second);
}

processor_file_ptr file_manager_impl::get_processor_file(const file_location& file)
{
    std::lock_guard guard(files_mutex);
    auto ret = files_.find(file);
    if (ret == files_.end())
    {
        return std::make_shared<processor_file_impl>(file, *this);
    }
    else
        return change_into_processor_file_if_not_already_(ret->second);
}

void file_manager_impl::remove_file(const file_location& file)
{
    std::lock_guard guard(files_mutex);
    if (!files_.contains(file))
        return;

    // close the file internally
    files_.erase(file);
}

file_ptr file_manager_impl::find(const utils::resource::resource_location& key) const
{
    std::lock_guard guard(files_mutex);
    auto ret = files_.find(key);
    if (ret == files_.end())
        return nullptr;

    return ret->second;
}

processor_file_ptr file_manager_impl::find_processor_file(const utils::resource::resource_location& key)
{
    std::lock_guard guard(files_mutex);
    auto ret = files_.find(key);
    if (ret == files_.end())
        return nullptr;

    return change_into_processor_file_if_not_already_(ret->second);
}

list_directory_result file_manager_impl::list_directory_files(const utils::resource::resource_location& directory) const
{
    return utils::resource::list_directory_files(directory);
}

list_directory_result file_manager_impl::list_directory_subdirs_and_symlinks(
    const utils::resource::resource_location& directory) const
{
    return utils::resource::list_directory_subdirs_and_symlinks(directory);
}

std::string file_manager_impl::canonical(const utils::resource::resource_location& res_loc, std::error_code& ec) const
{
    return utils::resource::canonical(res_loc, ec);
}

void file_manager_impl::prepare_file_for_change_(std::shared_ptr<file_impl>& file)
{
    if (file.use_count() == 1) // TODO: possible weak_ptr issue
        return;
    // another shared ptr to this file exists, we need to create a copy
    auto proc_file = std::dynamic_pointer_cast<processor_file>(file);
    if (proc_file)
        file = std::make_shared<processor_file_impl>(*file, *this, cancel_);
    else
        file = std::make_shared<file_impl>(*file);
}

void file_manager_impl::did_open_file(const file_location& document_loc, version_t version, std::string text)
{
    std::lock_guard guard(files_mutex);
    auto [ret, _] = files_.try_emplace(document_loc, std::make_shared<file_impl>(document_loc));
    prepare_file_for_change_(ret->second);
    ret->second->did_open(std::move(text), version);
}

void file_manager_impl::did_change_file(
    const file_location& document_loc, version_t, const document_change* changes, size_t ch_size)
{
    // TODO
    // the version is the version after the changes -> I don't see how is that useful
    // should we just overwrite the version??
    // on the other hand, the spec clearly specifies that each change increments version by one.

    std::lock_guard guard(files_mutex);

    auto file = files_.find(document_loc);
    if (file == files_.end())
        return; // if the file does not exist, no action is taken

    prepare_file_for_change_(file->second);

    for (size_t i = 0; i < ch_size; ++i)
    {
        std::string text_s(changes[i].text, changes[i].text_length);
        if (changes[i].whole)
            file->second->did_change(std::move(text_s));
        else
            file->second->did_change(changes[i].change_range, std::move(text_s));
    }
}

void file_manager_impl::did_close_file(const file_location& document_loc)
{
    std::lock_guard guard(files_mutex);
    auto file = files_.find(document_loc);
    if (file == files_.end())
        return;

    prepare_file_for_change_(file->second);
    // close the file externally
    file->second->did_close();

    // if the file does not exist, no action is taken
}

bool file_manager_impl::dir_exists(const utils::resource::resource_location& dir_loc) const
{
    return utils::resource::dir_exists(dir_loc);
}

void file_manager_impl::put_virtual_file(
    unsigned long long id, std::string_view text, utils::resource::resource_location related_workspace)
{
    std::lock_guard guard(virtual_files_mutex);
    m_virtual_files.try_emplace(id, text, std::move(related_workspace));
}

void file_manager_impl::remove_virtual_file(unsigned long long id)
{
    std::lock_guard guard(virtual_files_mutex);
    m_virtual_files.erase(id);
}

std::string file_manager_impl::get_virtual_file(unsigned long long id) const
{
    std::lock_guard guard(virtual_files_mutex);
    if (auto it = m_virtual_files.find(id); it != m_virtual_files.end())
        return it->second.text;
    return {};
}

utils::resource::resource_location file_manager_impl::get_virtual_file_workspace(unsigned long long id) const
{
    std::lock_guard guard(virtual_files_mutex);
    if (auto it = m_virtual_files.find(id); it != m_virtual_files.end())
        return it->second.related_workspace;
    return utils::resource::resource_location();
}

} // namespace hlasm_plugin::parser_library::workspaces
