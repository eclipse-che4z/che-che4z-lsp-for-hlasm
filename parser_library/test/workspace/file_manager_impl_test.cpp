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

#include <array>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "external_file_reader_mock.h"
#include "utils/platform.h"
#include "utils/resource_location.h"
#include "utils/text_convertor.h"
#include "workspaces/file.h"
#include "workspaces/file_manager_impl.h"

using namespace hlasm_plugin::parser_library::workspaces;
using namespace hlasm_plugin::utils::resource;
using namespace hlasm_plugin::utils;
using namespace ::testing;

namespace {
auto load_text_coroutine(std::optional<std::string> v)
{
    return [v]() { return hlasm_plugin::utils::value_task<std::optional<std::string>>::from_value(v); };
}

template<typename T>
T run_or_default(value_task<T> t, T default_value)
{
    if (!t.valid())
        return default_value;

    return std::move(t).run().value();
}

} // namespace

TEST(file_manager, update_file)
{
    const resource_location file("test/library/test_wks/correct");
    const std::string text1 = "aaa";
    const std::string text2 = "bbb";

    NiceMock<external_file_reader_mock> reader_mock;
    file_manager_impl fm(reader_mock, nullptr);

    // nobody is working with the file, so assume it has not changed
    EXPECT_EQ(run_or_default(fm.update_file(file), file_content_state::identical), file_content_state::identical);

    EXPECT_CALL(reader_mock, load_text(file)).WillOnce(Invoke(load_text_coroutine(text1)));

    auto f = fm.add_file(file).run().value();

    EXPECT_EQ(f->get_text(), text1);
    EXPECT_EQ(f->get_converted_text(), text1);

    EXPECT_CALL(reader_mock, load_text(file)).WillRepeatedly(Invoke(load_text_coroutine(text2)));

    EXPECT_EQ(run_or_default(fm.update_file(file), file_content_state::identical), file_content_state::changed_content);
    EXPECT_EQ(f->get_text(), text1); // old file version
    EXPECT_EQ(f->get_converted_text(), text1); // old file version
    EXPECT_EQ(run_or_default(fm.update_file(file), file_content_state::identical), file_content_state::identical);

    f = fm.add_file(file).run().value();
    EXPECT_EQ(f->get_text(), text2);
    EXPECT_EQ(f->get_converted_text(), text2);

    EXPECT_EQ(run_or_default(fm.update_file(file), file_content_state::identical), file_content_state::identical);
}

TEST(file_manger, keep_content_on_close)
{
    const resource_location file("filename");
    const std::string text = "aaa";

    NiceMock<external_file_reader_mock> reader_mock;
    file_manager_impl fm(reader_mock, nullptr);

    fm.did_open_file(file, 1, text);

    auto f1 = fm.add_file(file).run().value();

    fm.did_close_file(file);

    EXPECT_CALL(reader_mock, load_text(file)).WillOnce(Invoke(load_text_coroutine(text)));
    auto f2 = fm.add_file(file).run().value();

    EXPECT_EQ(f1.get(), f2.get());
    EXPECT_EQ(f1->get_version(), f2->get_version());
}

TEST(file_manager, get_file_content)
{
    if (hlasm_plugin::utils::platform::is_web())
        GTEST_SKIP() << "Direct I/O not available in Web mode";

    const resource_location correct(SrcDir() + "test/library/test_wks/correct");
    const resource_location notexists(SrcDir() + "test/library/test_wks/notexists");
    file_manager_impl fm;

    // nobody is working with the file, so assume it has not changed
    EXPECT_TRUE(fm.get_file_content(correct).run().value().has_value());
    EXPECT_FALSE(fm.get_file_content(notexists).run().value().has_value());
}

TEST(file_manager, conversions)
{
    using namespace hlasm_plugin::parser_library;
    using namespace std::string_view_literals;

    NiceMock<external_file_reader_mock> reader_mock;
    struct : text_convertor
    {
        void from(std::string& dst, std::string_view src) const override
        {
            std::ranges::transform(src, std::back_inserter(dst), [](auto c) { return c == 'A' ? 'B' : c; });
        }
        void to(std::string&, std::string_view) const override { assert(false); }
    } constexpr tc;
    file_manager_impl fm(reader_mock, &tc);

    const resource_location file("filename");
    const std::string text = "ABC";

    fm.did_open_file(file, 1, text);

    EXPECT_EQ(fm.get_file_content(file).run().value(), "ABC");
    EXPECT_EQ(fm.get_converted_file_content(file).run().value(), "BBC");

    fm.did_change_file(file, 2, std::array<document_change, 1> { { { range(), "A"sv } } });

    EXPECT_EQ(fm.get_file_content(file).run().value(), "AABC");
    EXPECT_EQ(fm.get_converted_file_content(file).run().value(), "BBBC");

    [[maybe_unused]] auto f = fm.add_file(file); // while holding a reference

    fm.did_change_file(file, 3, std::array<document_change, 1> { { { range(), "A"sv } } });

    EXPECT_EQ(fm.get_file_content(file).run().value(), "AAABC");
    EXPECT_EQ(fm.get_converted_file_content(file).run().value(), "BBBBC");
}
