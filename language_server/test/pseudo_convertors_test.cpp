/*
 * Copyright (c) 2026 Broadcom.
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

#include "pseudo_convertors.h"

using namespace hlasm_plugin::language_server;

TEST(pseudo_convertors, default_is_empty) { EXPECT_EQ(get_text_convertor(pseudo_charsets::ibm1148), nullptr); }

TEST(pseudo_convertors, ibm278)
{
    const auto* convertor = get_text_convertor(pseudo_charsets::ibm278);
    ASSERT_TRUE(convertor);

    const std::string input = reinterpret_cast<const char*>(u8"\U000000C5\U000000A4");
    std::string output;
    convertor->from(output, input);
    EXPECT_EQ(output, "$]");

    std::string back;
    convertor->to(back, output);
    EXPECT_EQ(back, input);
}

TEST(pseudo_convertors, ibm1143)
{
    const auto* convertor = get_text_convertor(pseudo_charsets::ibm1143);
    ASSERT_TRUE(convertor);

    const std::string input = reinterpret_cast<const char*>(u8"\U000000C5\U000020AC");
    std::string output;
    convertor->from(output, input);
    EXPECT_EQ(output, "$]");

    std::string back;
    convertor->to(back, output);
    EXPECT_EQ(back, input);
}

TEST(pseudo_convertors, ibm1143_prefix_match_without_conversion)
{
    const auto* convertor = get_text_convertor(pseudo_charsets::ibm1143);
    ASSERT_TRUE(convertor);

    const std::string input = reinterpret_cast<const char*>(u8"\U000020AB");
    std::string output;
    convertor->from(output, input);
    EXPECT_EQ(output, input);
}
