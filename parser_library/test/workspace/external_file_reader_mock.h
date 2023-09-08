/*
 * Copyright (c) 2023 Broadcom.
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

#include <optional>
#include <string>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "utils/resource_location.h"
#include "utils/task.h"
#include "workspaces/file_manager_impl.h"

namespace {
struct external_file_reader_mock : hlasm_plugin::parser_library::workspaces::external_file_reader
{
    MOCK_METHOD(hlasm_plugin::utils::value_task<std::optional<std::string>>,
        load_text,
        (const hlasm_plugin::utils::resource::resource_location&),
        (const, override));
    MOCK_METHOD(hlasm_plugin::utils::value_task<hlasm_plugin::parser_library::workspaces::list_directory_result>,
        list_directory_files,
        (const hlasm_plugin::utils::resource::resource_location&),
        (const, override));
    MOCK_METHOD(hlasm_plugin::utils::value_task<hlasm_plugin::parser_library::workspaces::list_directory_result>,
        list_directory_subdirs_and_symlinks,
        (const hlasm_plugin::utils::resource::resource_location& directory),
        (const, override));
};
} // namespace
