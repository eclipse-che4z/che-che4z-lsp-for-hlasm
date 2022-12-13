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

#include "gtest/gtest.h"

#include "utils/resource_location.h"
#include "workspaces/file_manager_impl.h"

using namespace hlasm_plugin::parser_library::workspaces;
using namespace hlasm_plugin::utils::resource;

TEST(file_manager, update_file)
{
    const resource_location file("test/library/test_wks/correct");
    file_manager_impl fm;

    // nobody is working with the file, so assume it has not changed
    EXPECT_EQ(fm.update_file(file), open_file_result::identical);

    auto f = fm.add_file(file);

    EXPECT_EQ(fm.update_file(file), open_file_result::changed_content);
    EXPECT_EQ(fm.update_file(file), open_file_result::identical);
}

TEST(file_manager, get_file_content)
{
    const resource_location file("test/library/test_wks/correct");
    file_manager_impl fm;

    // nobody is working with the file, so assume it has not changed
    EXPECT_TRUE(fm.get_file_content(resource_location("test/library/test_wks/correct")).has_value());
    EXPECT_FALSE(fm.get_file_content(resource_location("test/library/test_wks/notexists")).has_value());
}
