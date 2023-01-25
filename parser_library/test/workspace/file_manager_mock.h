/*
 * Copyright (c) 2022 Broadcom.
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

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "diagnosable_impl.h"
#include "workspaces/file_manager.h"

namespace {

class file_manager_mock : public file_manager, public diagnosable_impl
{
public:
    void collect_diags() const override
    {
        // nothing to do
    }
    MOCK_METHOD(file_ptr, add_file, (const file_location&), (override));
    MOCK_METHOD(processor_file_ptr, add_processor_file, (const file_location&), (override));
    MOCK_METHOD(processor_file_ptr, get_processor_file, (const file_location&), (override));
    MOCK_METHOD(void, remove_file, (const file_location&), (override));
    MOCK_METHOD(file_ptr, find, (const file_location& key), (const override));
    MOCK_METHOD(processor_file_ptr, find_processor_file, (const file_location& key), (override));
    MOCK_METHOD(list_directory_result,
        list_directory_files,
        (const hlasm_plugin::utils::resource::resource_location& path),
        (const override));
    MOCK_METHOD(list_directory_result,
        list_directory_subdirs_and_symlinks,
        (const hlasm_plugin::utils::resource::resource_location& path),
        (const override));
    MOCK_METHOD(std::string,
        canonical,
        (const hlasm_plugin::utils::resource::resource_location& res_loc, std::error_code& ec),
        (const override));
    MOCK_METHOD(
        bool, file_exists, (const hlasm_plugin::utils::resource::resource_location& file_loc), (const override));
    MOCK_METHOD(bool,
        lib_file_exists,
        (const hlasm_plugin::utils::resource::resource_location& lib_root, std::string_view file_name),
        (const override));
    MOCK_METHOD(bool, dir_exists, (const hlasm_plugin::utils::resource::resource_location& dir_loc), (const override));
    MOCK_METHOD(open_file_result,
        did_open_file,
        (const file_location& document_loc, version_t version, std::string text),
        (override));
    MOCK_METHOD(void,
        did_change_file,
        (const file_location& document_loc, version_t version, const document_change* changes, size_t ch_size),
        (override));
    MOCK_METHOD(void, did_close_file, (const file_location& document_loc), (override));
    MOCK_METHOD(void,
        put_virtual_file,
        (unsigned long long id,
            std::string_view text,
            hlasm_plugin::utils::resource::resource_location related_workspace),
        (override));
    MOCK_METHOD(void, remove_virtual_file, (unsigned long long id), (override));
    MOCK_METHOD(std::string, get_virtual_file, (unsigned long long id), (const override));
    MOCK_METHOD(hlasm_plugin::utils::resource::resource_location,
        get_virtual_file_workspace,
        (unsigned long long id),
        (const override));

    MOCK_METHOD(open_file_result, update_file, (const file_location& document_loc), (override));

    MOCK_METHOD(std::optional<std::string>,
        get_file_content,
        (const hlasm_plugin::utils::resource::resource_location&),
        (override));

    MOCK_METHOD(void, retrieve_fade_messages, (std::vector<fade_message_s>&), (const override));
};

} // namespace
