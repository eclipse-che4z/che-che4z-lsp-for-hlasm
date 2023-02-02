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
#include <span>

#include "file_impl.h"
#include "utils/content_loader.h"
#include "utils/path.h"
#include "utils/path_conversions.h"
#include "utils/platform.h"

namespace hlasm_plugin::parser_library::workspaces {

std::shared_ptr<file> file_manager_impl::add_file(const file_location& file)
{
    std::lock_guard guard(files_mutex);
    auto [ret, _] = files_.try_emplace(file, std::make_shared<file_impl>(file));
    return ret->second;
}

std::optional<std::string> file_manager_impl::get_file_content(
    const utils::resource::resource_location& file_name) const
{
    std::lock_guard guard(files_mutex);
    auto it = files_.find(file_name);
    auto file = it == files_.end() ? std::make_shared<file_impl>(file_name) : it->second;

    std::optional<std::string> result(file->get_text());
    if (file->is_bad())
        result.reset();

    return result;
}

void file_manager_impl::remove_file(const file_location& file)
{
    std::lock_guard guard(files_mutex);
    if (!files_.contains(file))
        return;

    // close the file internally
    files_.erase(file);
}

std::shared_ptr<file> file_manager_impl::find(const utils::resource::resource_location& key) const
{
    std::lock_guard guard(files_mutex);
    auto ret = files_.find(key);
    if (ret == files_.end())
        return nullptr;

    return ret->second;
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
    file = std::make_shared<file_impl>(*file);
}

open_file_result file_manager_impl::did_open_file(
    const file_location& document_loc, version_t version, std::string text)
{
    std::lock_guard guard(files_mutex);
    auto [ret, inserted] = files_.try_emplace(document_loc, std::make_shared<file_impl>(document_loc));
    if (ret->second->is_text_loaded() && ret->second->get_text() != text)
        prepare_file_for_change_(ret->second);
    auto changed = ret->second->did_open(std::move(text), version);
    return inserted ? open_file_result::changed_content : changed;
}

void file_manager_impl::did_change_file(
    const file_location& document_loc, version_t, const document_change* changes_start, size_t ch_size)
{
    // TODO
    // the version is the version after the changes -> I don't see how is that useful
    // should we just overwrite the version??
    // on the other hand, the spec clearly specifies that each change increments version by one.

    std::lock_guard guard(files_mutex);

    auto file = files_.find(document_loc);
    if (file == files_.end())
        return; // if the file does not exist, no action is taken

    if (!ch_size)
        return;

    prepare_file_for_change_(file->second);

    auto last_whole = changes_start + ch_size;
    while (last_whole != changes_start)
    {
        --last_whole;
        if (last_whole->whole)
            break;
    }

    for (const auto& change : std::span(last_whole, changes_start + ch_size))
    {
        std::string text_s(change.text, change.text_length);
        if (change.whole)
            file->second->did_change(std::move(text_s));
        else
            file->second->did_change(change.change_range, std::move(text_s));
    }
}

void file_manager_impl::did_close_file(const file_location& document_loc)
{
    std::lock_guard guard(files_mutex);
    auto file = files_.find(document_loc);
    if (file == files_.end())
        return;

    if (!file->second->is_text_loaded()
        || file->second->get_text() == utils::resource::load_text(file->second->get_location()))
        file->second->did_close(); // close the file externally, content is accurate
    else
        file->second = std::make_shared<file_impl>(file->second->get_location()); // our version is not accurate

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

open_file_result file_manager_impl::update_file(const file_location& document_loc)
{
    std::lock_guard guard(files_mutex);
    auto f = files_.find(document_loc);
    if (f == files_.end())
        return open_file_result::identical;

    if (f->second->update_and_get_bad() == update_file_result::identical)
        return open_file_result::identical;
    else
        return open_file_result::changed_content;
}

} // namespace hlasm_plugin::parser_library::workspaces
