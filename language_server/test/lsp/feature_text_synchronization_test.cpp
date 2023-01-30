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

#ifndef HLASMPLUGIN_LANGUAGESERVER_TEST_FEATURE_TEXT_SYNCHRONIZATION_TEST_H
#define HLASMPLUGIN_LANGUAGESERVER_TEST_FEATURE_TEXT_SYNCHRONIZATION_TEST_H

#include "gmock/gmock.h"

#include "../response_provider_mock.h"
#include "../ws_mngr_mock.h"
#include "lsp/feature_text_synchronization.h"
#include "nlohmann/json.hpp"
#include "utils/platform.h"
#include "utils/resource_location.h"

const std::string txt_file_uri =
    hlasm_plugin::utils::platform::is_windows() ? R"(file:///c%3A/test/one/blah.txt)" : R"(file:///home/user/somefile)";

using namespace hlasm_plugin;
using namespace hlasm_plugin::language_server;


TEST(text_synchronization, did_open_file)
{
    using namespace ::testing;
    test::ws_mngr_mock ws_mngr;
    response_provider_mock response_mock;
    lsp::feature_text_synchronization f(ws_mngr, response_mock);
    std::map<std::string, method> notifs;
    f.register_methods(notifs);

    auto params1 = nlohmann::json::parse(
        R"({"textDocument":{"uri":")" + txt_file_uri + R"(","languageId":"plaintext","version":4,"text":"sad"}})");

    EXPECT_CALL(ws_mngr, did_open_file(StrEq(txt_file_uri), 4, StrEq("sad"), 3));

    notifs["textDocument/didOpen"].handler("", params1);
}

MATCHER_P2(PointerAndSizeEqArray, pointer, size, "")
{
    (void)result_listener;
    const size_t actual_size = std::get<1>(arg);
    if (actual_size != size)
        return false;
    for (size_t i = 0; i < size; i++)
    {
        if (!(std::get<0>(arg)[i] == pointer[i]))
            return false;
    }
    return true;
}

TEST(text_synchronization, did_change_file)
{
    using namespace ::testing;
    test::ws_mngr_mock ws_mngr;
    response_provider_mock response_mock;
    lsp::feature_text_synchronization f(ws_mngr, response_mock);
    std::map<std::string, method> notifs;
    f.register_methods(notifs);

    auto params1 = nlohmann::json::parse(R"({"textDocument":{"uri":")" + txt_file_uri
        + R"(","version":7},"contentChanges":[{"range":{"start":{"line":0,"character":0},"end":{"line":0,"character":8}},"rangeLength":8,"text":"sad"}, {"range":{"start":{"line":1,"character":12},"end":{"line":1,"character":14}},"rangeLength":2,"text":""}]})");

    parser_library::document_change expected1[2] { { { { 0, 0 }, { 0, 8 } }, "sad", 3 },
        { { { 1, 12 }, { 1, 14 } }, "", 0 } };

    EXPECT_CALL(ws_mngr, did_change_file(StrEq(txt_file_uri), 7, _, 2))
        .With(Args<2, 3>(PointerAndSizeEqArray(expected1, std::size(expected1))));
    notifs["textDocument/didChange"].handler("", params1);



    parser_library::document_change expected2[1] { { "sad", 3 } };
    auto params2 = nlohmann::json::parse(
        R"({"textDocument":{"uri":")" + txt_file_uri + R"(","version":7},"contentChanges":[{"text":"sad"}]})");
    EXPECT_CALL(ws_mngr, did_change_file(StrEq(txt_file_uri), 7, _, 1))
        .With(Args<2, 3>(PointerAndSizeEqArray(expected2, std::size(expected2))));

    notifs["textDocument/didChange"].handler("", params2);



    auto params3 = nlohmann::json::parse(R"({"textDocument":{"uri":")" + txt_file_uri
        + R"("},"contentChanges":[{"range":{"start":{"line":0,"character":0},"end":{"line":0,"character":8}},"rangeLength":8,"text":"sad"}, {"range":{"start":{"line":1,"character":12},"end":{"line":1,"character":14}},"rangeLength":2,"text":""}]})");

    EXPECT_THROW(notifs["textDocument/didChange"].handler("", params3), nlohmann::basic_json<>::exception);
}

TEST(text_synchronization, did_close_file)
{
    using namespace ::testing;
    test::ws_mngr_mock ws_mngr;
    response_provider_mock response_mock;
    lsp::feature_text_synchronization f(ws_mngr, response_mock);
    std::map<std::string, method> notifs;
    f.register_methods(notifs);

    auto params1 = nlohmann::json::parse(R"({"textDocument":{"uri":")" + txt_file_uri + R"("}})");
    EXPECT_CALL(ws_mngr, did_close_file(StrEq(txt_file_uri))),

        notifs["textDocument/didClose"].handler("", params1);
}

#endif // !HLASMPLUGIN_LANGUAGESERVER_TEST_FEATURE_TEXT_SYNCHRONIZATION_TEST_H