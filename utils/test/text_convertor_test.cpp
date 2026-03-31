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


#include <algorithm>
#include <iterator>

#include "gtest/gtest.h"

#include "utils/text_convertor.h"

using namespace hlasm_plugin::utils;

struct test_convertor_t final : text_convertor
{
    void from(std::string& dst, std::string_view src) const override
    {
        std::ranges::transform(src, std::back_inserter(dst), [](auto c) { return c == 'A' ? 'B' : c; });
    }

    virtual void to(std::string& dst, std::string_view src) const override
    {
        std::ranges::transform(src, std::back_inserter(dst), [](auto c) { return c == 'B' ? 'A' : c; });
    }
} constexpr test_convertor;

TEST(conversion_helper, null)
{
    using namespace std::string_literals;
    using namespace std::string_view_literals;

    conversion_helper helper(nullptr);

    EXPECT_EQ(helper.convert_from("A"sv, "A"sv, "C"sv), "AAC");
    EXPECT_EQ(helper.convert_to("B"sv, "B"sv, "C"sv), "BBC");
    EXPECT_EQ(helper.convert_from("A"sv), "A");
    EXPECT_EQ(helper.convert_to("B"sv), "B");
    EXPECT_EQ(helper.convert_from("A"s), "A");
    EXPECT_EQ(helper.convert_to("B"s), "B");

    std::string output;

    helper.append_from(output, "A");
    helper.append_to(output, "B");

    EXPECT_EQ(output, "AB");

    std::string inplace = "A";

    helper.inplace_from(inplace);
    EXPECT_EQ(inplace, "A");

    helper.inplace_to(inplace);
    EXPECT_EQ(inplace, "A");
}

TEST(conversion_helper, nonnull)
{
    using namespace std::string_literals;
    using namespace std::string_view_literals;

    conversion_helper helper(&test_convertor);

    EXPECT_EQ(helper.convert_from("A"sv, "A"sv, "C"sv), "BBC");
    EXPECT_EQ(helper.convert_to("B"sv, "B"sv, "C"sv), "AAC");
    EXPECT_EQ(helper.convert_from("A"sv), "B");
    EXPECT_EQ(helper.convert_to("B"sv), "A");
    EXPECT_EQ(helper.convert_from("A"s), "B");
    EXPECT_EQ(helper.convert_to("B"s), "A");

    std::string output;

    helper.append_from(output, "A");
    helper.append_to(output, "B");

    EXPECT_EQ(output, "BA");

    std::string inplace = "A";

    helper.inplace_from(inplace);
    EXPECT_EQ(inplace, "B");

    helper.inplace_to(inplace);
    EXPECT_EQ(inplace, "A");
}
