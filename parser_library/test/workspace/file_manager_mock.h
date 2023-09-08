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

class file_manager_mock : public hlasm_plugin::parser_library::workspaces::file_manager,
                          public hlasm_plugin::parser_library::diagnosable_impl
{
    using resource_location = hlasm_plugin::utils::resource::resource_location;
    template<typename T>
    using value_task = hlasm_plugin::utils::value_task<T>;

public:
    void collect_diags() const override
    {
        // nothing to do
    }
    MOCK_METHOD(value_task<std::shared_ptr<hlasm_plugin::parser_library::workspaces::file>>,
        add_file,
        (const resource_location&),
        (override));
    MOCK_METHOD(std::shared_ptr<hlasm_plugin::parser_library::workspaces::file>,
        find,
        (const resource_location& key),
        (const, override));
    MOCK_METHOD(value_task<hlasm_plugin::parser_library::workspaces::list_directory_result>,
        list_directory_files,
        (const resource_location& path),
        (const, override));
    MOCK_METHOD(value_task<hlasm_plugin::parser_library::workspaces::list_directory_result>,
        list_directory_subdirs_and_symlinks,
        (const resource_location& path),
        (const, override));
    MOCK_METHOD(std::string, canonical, (const resource_location& res_loc, std::error_code& ec), (const, override));
    MOCK_METHOD(hlasm_plugin::parser_library::workspaces::file_content_state,
        did_open_file,
        (const resource_location& document_loc, hlasm_plugin::parser_library::version_t version, std::string text),
        (override));
    MOCK_METHOD(void,
        did_change_file,
        (const resource_location& document_loc,
            hlasm_plugin::parser_library::version_t version,
            const hlasm_plugin::parser_library::document_change* changes,
            size_t ch_size),
        (override));
    MOCK_METHOD(void, did_close_file, (const resource_location& document_loc), (override));
    MOCK_METHOD(std::string_view,
        put_virtual_file,
        (unsigned long long id, std::string_view text, resource_location related_workspace),
        (override));
    MOCK_METHOD(void, remove_virtual_file, (unsigned long long id), (override));
    MOCK_METHOD(std::string, get_virtual_file, (unsigned long long id), (const, override));
    MOCK_METHOD(resource_location, get_virtual_file_workspace, (unsigned long long id), (const, override));

    MOCK_METHOD(value_task<hlasm_plugin::parser_library::workspaces::file_content_state>,
        update_file,
        (const resource_location& document_loc),
        (override));

    MOCK_METHOD(value_task<std::optional<std::string>>, get_file_content, (const resource_location&), (override));
};

} // namespace
