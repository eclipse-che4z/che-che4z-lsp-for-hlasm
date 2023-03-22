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

#include <atomic>
#include <map>
#include <memory>
#include <span>
#include <variant>

#include "utils/content_loader.h"
#include "utils/path.h"
#include "utils/path_conversions.h"
#include "utils/platform.h"

namespace {
auto next_global_version()
{
    static std::atomic<hlasm_plugin::parser_library::version_t> global_version = 0;
    return global_version.fetch_add(1, std::memory_order_relaxed) + 1;
}
} // namespace

namespace hlasm_plugin::parser_library::workspaces {

struct file_manager_impl::mapped_file final : file
{
    // there is no noexcept version of std::enable_shared_from_this::shared_from_this
    std::weak_ptr<mapped_file> m_self;
    std::shared_ptr<mapped_file> shared_from_this() const noexcept { return m_self.lock(); }

    file_location m_location;
    std::string m_text;
    struct file_error
    {};
    std::optional<file_error> m_error;
    // Array of "pointers" to m_text where lines start.
    std::vector<size_t> m_lines;

    file_manager_impl& m_fm;

    version_t m_lsp_version = 0;

    version_t m_version = next_global_version();

    std::shared_ptr<mapped_file> m_editing_self_reference;
    decltype(file_manager_impl::m_files)::const_iterator m_it = m_fm.m_files.end();

    mapped_file(const file_location& file_name, file_manager_impl& fm, std::string text)
        : m_location(file_name)
        , m_text(std::move(text))
        , m_lines(create_line_indices(m_text))
        , m_fm(fm)
    {}

    mapped_file(const file_location& file_name, file_manager_impl& fm, file_error error)
        : m_location(file_name)
        , m_error(std::move(error))
        , m_fm(fm)
    {}

    mapped_file(const mapped_file& that)
        : m_location(that.m_location)
        , m_text(that.m_text)
        , m_error(that.m_error)
        , m_lines(that.m_lines)
        , m_fm(that.m_fm)
        , m_lsp_version(that.m_lsp_version)
    {}

    ~mapped_file()
    {
        std::lock_guard guard(m_fm.files_mutex);
        if (m_it != m_fm.m_files.end())
            m_fm.m_files.erase(m_it);
    }

    // Inherited via file
    const file_location& get_location() const override { return m_location; }
    const std::string& get_text() const override { return m_text; }
    bool get_lsp_editing() const override { return m_editing_self_reference != nullptr; }
    version_t get_version() const override { return m_version; }
    bool error() const override { return m_error.has_value(); }

    bool up_to_date() const override
    {
        std::lock_guard guard(m_fm.files_mutex);
        return m_it != m_fm.m_files.end();
    }

    std::optional<std::string_view> get_text_or_error() const
    {
        if (m_error.has_value())
            return std::nullopt;
        else
            return m_text;
    }
};

class : public external_file_reader
{
    std::optional<std::string> load_text(const utils::resource::resource_location& document_loc) const final
    {
        return utils::resource::load_text(document_loc);
    }
} constexpr default_reader;

file_manager_impl::file_manager_impl()
    : file_manager_impl(default_reader)
{}

file_manager_impl::file_manager_impl(const external_file_reader& file_reader)
    : m_file_reader(&file_reader)
{}

file_manager_impl::~file_manager_impl()
{
    std::shared_ptr<mapped_file> to_release;
    std::shared_ptr<mapped_file>* release = &to_release;

    std::unique_lock lock(files_mutex);

    for (auto& [_, mf] : m_files)
    {
        if (mf.file->m_editing_self_reference)
        {
            *release = std::move(mf.file->m_editing_self_reference);
            release = &mf.file->m_editing_self_reference;
        }
    }

    lock.unlock();

    while (to_release)
    {
        auto next = std::move(to_release->m_editing_self_reference);
        std::swap(next, to_release);
    }
}

template<typename... Args>
std::shared_ptr<file_manager_impl::mapped_file> file_manager_impl::make_mapped_file(Args&&... args)
{
    auto result = std::make_shared<mapped_file>(std::forward<Args>(args)...);
    result->m_self = result;
    return result;
}

std::shared_ptr<file> file_manager_impl::add_file(const file_location& file_name)
{
    std::unique_lock lock(files_mutex);

    if (auto result = try_obtaining_file_unsafe(file_name, nullptr))
        return result;

    lock.unlock();

    auto loaded_text = m_file_reader->load_text(file_name);

    lock.lock();

    if (auto result = try_obtaining_file_unsafe(file_name, &loaded_text))
        return result;

    auto result = loaded_text.has_value() ? make_mapped_file(file_name, *this, loaded_text.value())
                                          : make_mapped_file(file_name, *this, mapped_file::file_error());

    result->m_it = m_files.try_emplace(file_name, result.get()).first;

    return result;
}

std::shared_ptr<file_manager_impl::mapped_file> file_manager_impl::try_obtaining_file_unsafe(
    const utils::resource::resource_location& file_name, const std::optional<std::string>* expected_text)
{
    auto it = m_files.find(file_name);
    if (it == m_files.end())
        return {};

    auto& [file, closed] = it->second;

    auto result = file->shared_from_this();
    if (!result)
    {
        file->m_it = m_files.end();
        m_files.erase(it);
        return {};
    }

    if (closed)
    {
        if (!expected_text)
            return {};

        if (file->m_text != *expected_text)
        {
            file->m_it = m_files.end();
            m_files.erase(it);
            return {};
        }

        closed = false;
    }

    return result;
}

std::optional<std::string> file_manager_impl::get_file_content(const utils::resource::resource_location& file_name)
{
    auto f = add_file(file_name);
    if (f->error())
        return std::nullopt;
    else
        return f->get_text();
}

std::shared_ptr<file> file_manager_impl::find(const utils::resource::resource_location& key) const
{
    std::lock_guard guard(files_mutex);
    auto ret = m_files.find(key);
    if (ret == m_files.end() || ret->second.closed)
        return nullptr;

    return ret->second.file->shared_from_this();
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

open_file_result file_manager_impl::did_open_file(
    const file_location& document_loc, version_t version, std::string new_text)
{
    std::shared_ptr<mapped_file> locked;

    std::lock_guard guard(files_mutex);

    auto it = m_files.find(document_loc);
    if (it != m_files.end())
        locked = it->second.file->shared_from_this();

    if (!locked || locked->m_error || locked->m_text != new_text)
    {
        if (it != m_files.end())
        {
            it->second.file->m_it = m_files.end();
            m_files.erase(it);
        }

        auto file = make_mapped_file(document_loc, *this, std::move(new_text));

        file->m_lsp_version = version;
        file->m_it = m_files.try_emplace(document_loc, file.get()).first;
        file->m_editing_self_reference = std::move(file);

        return open_file_result::changed_content;
    }
    else
    {
        locked->m_lsp_version = version;
        locked->m_editing_self_reference = std::move(locked);
        it->second.closed = false;

        return open_file_result::changed_lsp;
    }
}

void file_manager_impl::did_change_file(
    const file_location& document_loc, version_t, const document_change* changes_start, size_t ch_size)
{
    if (!ch_size)
        return;

    std::shared_ptr<mapped_file> to_release;

    std::lock_guard guard(files_mutex);

    auto it = m_files.find(document_loc);
    if (it == m_files.end() || it->second.closed)
        return; // if the file does not exist, no action is taken

    auto file = it->second.file->shared_from_this();
    if (!file)
        return;

    if (file.use_count() > 1 + (file->m_editing_self_reference != nullptr))
    {
        // this file is already being used by others
        auto new_file = make_mapped_file(*file);

        new_file->m_editing_self_reference = new_file;
        new_file->m_it = it;

        file->m_it = m_files.end();
        std::swap(to_release, file->m_editing_self_reference);

        it->second = mapped_file_entry(new_file.get());

        std::swap(file, new_file);
    }

    // now we can be sure that we are the only ones who have access to file

    auto last_whole = changes_start + ch_size;
    while (last_whole != changes_start)
    {
        --last_whole;
        if (last_whole->whole)
            break;
    }

    if (last_whole->whole)
    {
        file->m_text = std::string_view(last_whole->text, last_whole->text_length);
        ++last_whole;
    }

    for (const auto& change : std::span(last_whole, changes_start + ch_size))
    {
        std::string_view text_s(change.text, change.text_length);
        apply_text_diff(file->m_text, file->m_lines, change.change_range, text_s);
    }

    file->m_lsp_version += ch_size;
    file->m_version = next_global_version();
}

void file_manager_impl::did_close_file(const file_location& document_loc)
{
    std::shared_ptr<mapped_file> to_release;

    std::lock_guard guard(files_mutex);

    auto it = m_files.find(document_loc);
    if (it == m_files.end())
        return;

    if (auto file = it->second.file->shared_from_this();
        file && file.use_count() > 1 + (file->m_editing_self_reference != nullptr) && !file->m_error)
    {
        std::swap(to_release, file->m_editing_self_reference);
        it->second.closed = true;
    }
    else
    {
        it->second.file->m_it = m_files.end();
        m_files.erase(it);
    }
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
    std::unique_lock lock(files_mutex);

    if (auto f = m_files.find(document_loc); f == m_files.end() || f->second.file->get_lsp_editing())
        return open_file_result::identical;

    lock.unlock();

    auto current_text = m_file_reader->load_text(document_loc);

    lock.lock();

    auto f = m_files.find(document_loc);
    if (f == m_files.end() || f->second.file->get_lsp_editing())
        return open_file_result::identical;

    if (f->second.file->get_text_or_error() == current_text)
    {
        f->second.closed = false;
        return open_file_result::identical;
    }

    f->second.file->m_it = m_files.end();
    m_files.erase(f);
    return open_file_result::changed_content;
}

} // namespace hlasm_plugin::parser_library::workspaces
