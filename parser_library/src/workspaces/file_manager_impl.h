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

#ifndef HLASMPLUGIN_PARSERLIBRARY_FILE_MANAGER_IMPL_H
#define HLASMPLUGIN_PARSERLIBRARY_FILE_MANAGER_IMPL_H

#include <memory>

#include "diagnosable_impl.h"
#include "file_manager.h"
#include "processor_file_impl.h"
#include "utils/resource_location.h"

namespace hlasm_plugin::parser_library::workspaces {

#pragma warning(push)
#pragma warning(disable : 4250)

// Implementation of the file_manager interface.
class file_manager_impl : public file_manager, public diagnosable_impl
{
    mutable std::mutex files_mutex;
    mutable std::mutex virtual_files_mutex;
    std::atomic<bool>* cancel_;

public:
    file_manager_impl(std::atomic<bool>* cancel = nullptr)
        : cancel_(cancel) {};
    file_manager_impl(const file_manager_impl&) = delete;
    file_manager_impl& operator=(const file_manager_impl&) = delete;

    file_manager_impl(file_manager_impl&&) = delete;
    file_manager_impl& operator=(file_manager_impl&&) = delete;

    void collect_diags() const override;

    file_ptr add_file(const file_location&) override;
    processor_file_ptr add_processor_file(const file_location&) override;
    processor_file_ptr get_processor_file(const file_location&) override;
    void remove_file(const file_location&) override;

    file_ptr find(const utils::resource::resource_location& key) const override;
    processor_file_ptr find_processor_file(const utils::resource::resource_location& key) override;

    list_directory_result list_directory_files(const utils::resource::resource_location& directory) const override;
    list_directory_result list_directory_subdirs_and_symlinks(
        const utils::resource::resource_location& directory) const override;

    std::string canonical(const utils::resource::resource_location& res_loc, std::error_code& ec) const override;

    void did_open_file(const file_location& document_loc, version_t version, std::string text) override;
    void did_change_file(
        const file_location& document_loc, version_t version, const document_change* changes, size_t ch_size) override;
    void did_close_file(const file_location& document_loc) override;

    bool dir_exists(const utils::resource::resource_location& dir_loc) const override;

    virtual ~file_manager_impl() = default;

    void put_virtual_file(unsigned long long id, std::string_view text) override;
    void remove_virtual_file(unsigned long long id) override;
    std::string get_virtual_file(unsigned long long id) const override;

private:
    std::unordered_map<unsigned long long, std::string> m_virtual_files;
    // m_virtual_files must outlive the files_
    std::unordered_map<utils::resource::resource_location,
        std::shared_ptr<file_impl>,
        utils::resource::resource_location_hasher>
        files_;

    processor_file_ptr change_into_processor_file_if_not_already_(std::shared_ptr<file_impl>& ret);
    void prepare_file_for_change_(std::shared_ptr<file_impl>& file);
};

#pragma warning(pop)

} // namespace hlasm_plugin::parser_library::workspaces

#endif // !HLASMPLUGIN_PARSERLIBRARY_FILE_MANAGER_IMPL_H
