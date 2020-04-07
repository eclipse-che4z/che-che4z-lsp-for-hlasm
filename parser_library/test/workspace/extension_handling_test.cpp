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

#include "workspace/file_manager_impl.h"
#include "workspace/wildcard.h"
#include "workspace/workspace.h"

using namespace hlasm_plugin::parser_library::workspace;

#ifdef _WIN32
    const std::string lib_path = "lib\\";
    const std::string lib_path2 = "lib2\\";
#else
    const std::string lib_path = "lib/";
    const std::string lib_path2 = "lib2/";
#endif

class file_manager_extension_mock : public file_manager_impl
{
    virtual std::unordered_map<std::string, std::string> list_directory_files(const std::string&) override
    {
        return { {"Mac.hlasm",lib_path + "Mac.hlasm"} };
    }
};

TEST(extension_handling_test, wildcard)
{
    std::string test = "this is a test sentence.";

    auto regex = wildcard2regex("*test*");
    EXPECT_TRUE(std::regex_match(test, regex));

    regex = wildcard2regex("*.");
    EXPECT_TRUE(std::regex_match(test, regex));

    regex = wildcard2regex("this is a test ?entence.");
    EXPECT_TRUE(std::regex_match(test, regex));

    regex = wildcard2regex("*.?");
    EXPECT_FALSE(std::regex_match(test, regex));
}

TEST(extension_handling_test, extension_removal)
{
    file_manager_extension_mock file_mngr;
    // file must end with hlasm, true for lib/Mac.hlasm
    extension_regex_map map{ { ".hlasm", wildcard2regex("*.hlasm") } };
    library_local lib(file_mngr, "lib", std::make_shared<const extension_regex_map>(map));
    EXPECT_NE(lib.find_file("MAC"), nullptr);

    // file must end with hlasm and be in folder lib, true for lib/Mac.hlasm
    map = { { ".hlasm", wildcard2regex("*" + lib_path + "*.hlasm")} };
    library_local lib2(file_mngr, "lib", std::make_shared<const extension_regex_map>(map));
    EXPECT_NE(lib2.find_file("MAC"), nullptr);

    // file must end with asm, false for lib/Mac.hlasm
    map = { { ".asm",wildcard2regex("*.asm")} };
    library_local lib3(file_mngr, "lib", std::make_shared<const extension_regex_map>(map));
    EXPECT_EQ(lib3.find_file("MAC"), nullptr);

    // file must end with hlasm and be in folder lib2, false for lib/Mac.hlasm
    extension_regex_map map2{ { ".hlasm", wildcard2regex("*" + lib_path2 + "*.hlasm") } };
    library_local lib4(file_mngr, "lib2", std::make_shared<const extension_regex_map>(map2));
    EXPECT_EQ(lib4.find_file("MAC"), nullptr);
}
