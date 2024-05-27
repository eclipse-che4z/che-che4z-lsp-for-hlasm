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

#include "gtest/gtest.h"

#include "../common_testing.h"
#include "diagnostic.h"
#include "utils/platform.h"
#include "workspaces/file_manager_impl.h"
#include "workspaces/library_local.h"
#include "workspaces/workspace.h"

using namespace hlasm_plugin::parser_library::workspaces;
using namespace hlasm_plugin::utils::resource;

namespace {
const auto lib_loc =
    hlasm_plugin::utils::platform::is_windows() ? resource_location("lib\\") : resource_location("lib/");
} // namespace

class file_manager_extension_mock : public file_manager_impl
{
    hlasm_plugin::utils::value_task<list_directory_result> list_directory_files(const resource_location&) const override
    {
        return hlasm_plugin::utils::value_task<list_directory_result>::from_value({
            {
                { "Mac.hlasm", resource_location::join(lib_loc, "Mac.hlasm") },
            },
            hlasm_plugin::utils::path::list_directory_rc::done,
        });
    }
};

TEST(extension_handling_test, extension_removal)
{
    file_manager_extension_mock file_mngr;
    resource_location empty_loc;

    // file must end with hlasm
    library_local lib(file_mngr, lib_loc, { { ".hlasm" } }, empty_loc);
    run_if_valid(lib.prefetch());
    EXPECT_TRUE(lib.has_file("MAC"));

    // file must end with hlasm
    library_local lib2(file_mngr, lib_loc, { { ".hlasm" } }, empty_loc);
    run_if_valid(lib2.prefetch());
    EXPECT_TRUE(lib2.has_file("MAC"));

    // file must end with asm
    library_local lib3(file_mngr, lib_loc, { { ".asm" } }, empty_loc);
    run_if_valid(lib3.prefetch());
    EXPECT_FALSE(lib3.has_file("MAC"));

    // test multiple extensions
    library_local lib4(file_mngr, lib_loc, { { ".hlasm", ".asm" } }, empty_loc);
    run_if_valid(lib4.prefetch());
    EXPECT_TRUE(lib4.has_file("MAC"));

    // test no extensions
    library_local lib5(file_mngr, lib_loc, { {} }, empty_loc);
    run_if_valid(lib5.prefetch());
    EXPECT_TRUE(lib5.has_file("MAC"));

    // test empty extension
    library_local lib6(file_mngr, lib_loc, { { "" } }, empty_loc);
    run_if_valid(lib6.prefetch());
    EXPECT_FALSE(lib6.has_file("MAC"));

    // tolerate missing dot
    library_local lib7(file_mngr, lib_loc, { { "hlasm", "asm" } }, empty_loc);
    run_if_valid(lib7.prefetch());
    EXPECT_TRUE(lib7.has_file("MAC"));
}

TEST(extension_handling_test, legacy_extension_selection)
{
    file_manager_extension_mock file_mngr;
    resource_location empty_loc;
    library_local lib(file_mngr, lib_loc, { { ".hlasm" } }, empty_loc);
    run_if_valid(lib.prefetch());

    EXPECT_TRUE(lib.has_file("MAC"));
    std::vector<hlasm_plugin::parser_library::diagnostic> diags;
    lib.copy_diagnostics(diags);
    EXPECT_TRUE(diags.empty());
}

class file_manager_extension_mock2 : public file_manager_impl
{
    hlasm_plugin::utils::value_task<list_directory_result> list_directory_files(const resource_location&) const override
    {
        return hlasm_plugin::utils::value_task<list_directory_result>::from_value({
            {
                { "Mac.hlasm", resource_location::join(lib_loc, "Mac.hlasm") },
                { "Mac", resource_location::join(lib_loc, "Mac") },
            },
            hlasm_plugin::utils::path::list_directory_rc::done,
        });
    }
};

TEST(extension_handling_test, multiple_macro_definitions)
{
    file_manager_extension_mock2 file_mngr;
    resource_location empty_loc;
    library_local lib(file_mngr, lib_loc, { { ".hlasm", "" } }, empty_loc);
    run_if_valid(lib.prefetch());

    EXPECT_TRUE(lib.has_file("MAC"));
    std::vector<hlasm_plugin::parser_library::diagnostic> diags;
    lib.copy_diagnostics(diags);
    EXPECT_TRUE(contains_message_codes(diags, { "L0004" }));
}

TEST(extension_handling_test, no_multiple_macro_definitions)
{
    file_manager_extension_mock2 file_mngr;
    resource_location empty_loc;
    library_local lib(file_mngr, lib_loc, { { ".hlasm" } }, empty_loc);
    run_if_valid(lib.prefetch());

    EXPECT_TRUE(lib.has_file("MAC"));
    std::vector<hlasm_plugin::parser_library::diagnostic> diags;
    lib.copy_diagnostics(diags);
    EXPECT_FALSE(contains_message_codes(diags, { "L0004" }));
}

TEST(extension_handling_test, multiple_macros_extensions_not_provided)
{
    file_manager_extension_mock2 file_mngr;
    resource_location empty_loc;
    library_local lib(file_mngr, lib_loc, {}, empty_loc);
    run_if_valid(lib.prefetch());

    EXPECT_TRUE(lib.has_file("MAC"));
    std::vector<hlasm_plugin::parser_library::diagnostic> diags;
    lib.copy_diagnostics(diags);
    EXPECT_EQ(std::ranges::count(diags, "L0004", &diagnostic::code), 1);
}

class file_manager_extension_mock_no_ext : public file_manager_impl
{
    hlasm_plugin::utils::value_task<list_directory_result> list_directory_files(const resource_location&) const override
    {
        return hlasm_plugin::utils::value_task<list_directory_result>::from_value({
            {
                { "Mac", resource_location::join(lib_loc, "Mac") },
            },
            hlasm_plugin::utils::path::list_directory_rc::done,
        });
    }
};

TEST(extension_handling_test, legacy_extension_selection_file_without_ext)
{
    file_manager_extension_mock_no_ext file_mngr;
    resource_location empty_loc;
    library_local lib(file_mngr, lib_loc, { { ".hlasm" } }, empty_loc);
    run_if_valid(lib.prefetch());

    EXPECT_FALSE(lib.has_file("MAC"));
    std::vector<hlasm_plugin::parser_library::diagnostic> diags;
    lib.copy_diagnostics(diags);
    EXPECT_TRUE(diags.empty());
}
