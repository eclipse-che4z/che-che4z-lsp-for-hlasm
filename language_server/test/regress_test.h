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

#ifndef HLASMPLUGIN_LANGUAGESERVER_TEST_REGRESS_TEST_H
#define HLASMPLUGIN_LANGUAGESERVER_TEST_REGRESS_TEST_H

#include <memory>

#include "server.h"

using namespace hlasm_plugin;
using namespace language_server;

using json = nlohmann::json;
using server_notification = std::pair<std::string, json>;


class message_provider_mock : public send_message_provider
{
public:
    virtual void reply(const json& result) override { notfs.push_back(result); }

    std::vector<json> notfs;
};

json make_notification(std::string method_name, json parameter)
{
    return { { "jsonrpc", "2.0" }, { "method", method_name }, { "params", parameter } };
}

TEST(regress_test, behaviour_correct)
{
    workspace_manager ws_mngr;
    message_provider_mock mess_p;
    lsp::server s(ws_mngr);
    s.set_send_message_provider(&mess_p);

    auto notf = make_notification("textDocument/didOpen",
        R"#({"textDocument":{"uri":"file:///c%3A/test/behaviour_correct.hlasm","languageId":"plaintext","version":1,"text":"LABEL LR 1,1 REMARK"}})#"_json);
    s.message_received(notf);

    ASSERT_EQ(mess_p.notfs.size(), (size_t)1);
    ASSERT_EQ(mess_p.notfs[0]["method"], "textDocument/semanticHighlighting");
    auto tokens = mess_p.notfs[0]["params"]["tokens"];
    for (auto token : tokens)
    {
        switch (token["columnStart"].get<int>())
        {
            case 0:
                EXPECT_EQ(token["scope"].get<std::string>(), "label");
                break;
            case 6:
                EXPECT_EQ(token["scope"].get<std::string>(), "instruction");
                break;
            case 9:
                EXPECT_EQ(token["scope"].get<std::string>(), "operand");
                break;
            case 10:
                EXPECT_EQ(token["scope"].get<std::string>(), "operator");
                break;
            case 11:
                EXPECT_EQ(token["scope"].get<std::string>(), "operand");
                break;
            case 13:
                EXPECT_EQ(token["scope"].get<std::string>(), "remark");
                break;
            default:
                FAIL();
                break;
        }
    }

    mess_p.notfs.clear();
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/behaviour_correct.hlasm","version":2},"contentChanges":[{"range":{"start":{"line":0,"character":6},"end":{"line":0,"character":8}},"rangeLength":5,"text":"SAM24"}]})#"_json);
    s.message_received(notf);

    ASSERT_EQ(mess_p.notfs.size(), (size_t)1);
    ASSERT_EQ(mess_p.notfs[0]["method"], "textDocument/semanticHighlighting");
    tokens = mess_p.notfs[0]["params"]["tokens"];
    for (auto token : tokens)
    {
        switch (token["columnStart"].get<int>())
        {
            case 0:
                EXPECT_EQ(token["scope"].get<std::string>(), "label");
                break;
            case 6:
                EXPECT_EQ(token["scope"].get<std::string>(), "instruction");
                break;
            case 12:
                EXPECT_EQ(token["scope"].get<std::string>(), "remark");
                break;
            default:
                FAIL();
                break;
        }
    }
    mess_p.notfs.clear();
}

TEST(regress_test, behaviour_error)
{
    workspace_manager ws_mngr;
    message_provider_mock mess_p;
    lsp::server s(ws_mngr);
    s.set_send_message_provider(&mess_p);

    auto notf = make_notification("textDocument/didOpen",
        R"#({"textDocument":{"uri":"file:///c%3A/test/behaviour_error.hlasm","languageId":"plaintext","version":1,"text":"LABEL LR 1,20 REMARK"}})#"_json);
    s.message_received(notf);

    ASSERT_EQ(mess_p.notfs.size(), (size_t)2);
    ASSERT_EQ(mess_p.notfs[0]["method"], "textDocument/semanticHighlighting");
    ASSERT_EQ(mess_p.notfs[1]["method"], "textDocument/publishDiagnostics");
    auto diagnostics = mess_p.notfs[1]["params"]["diagnostics"];
    ASSERT_EQ(diagnostics.size(), (size_t)1);
    EXPECT_EQ(diagnostics[0]["code"].get<std::string>(), "M120");

    mess_p.notfs.clear();

    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/behaviour_error.hlasm","version":2},"contentChanges":[{"range":{"start":{"line":0,"character":12},"end":{"line":0,"character":13}},"rangeLength":1,"text":""}]})#"_json);
    s.message_received(notf);

    ASSERT_EQ(mess_p.notfs.size(), (size_t)2);
    ASSERT_EQ(mess_p.notfs[0]["method"], "textDocument/semanticHighlighting");
    ASSERT_EQ(mess_p.notfs[1]["method"], "textDocument/publishDiagnostics");
    EXPECT_EQ(mess_p.notfs[1]["params"]["diagnostics"].size(), (size_t)0);

    mess_p.notfs.clear();

    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/behaviour_error.hlasm","version":2},"contentChanges":[{"range":{"start":{"line":0,"character":5},"end":{"line":0,"character":19}},"rangeLength":14,"text":""}]})#"_json);
    s.message_received(notf);

    ASSERT_EQ(mess_p.notfs.size(), (size_t)2);
    ASSERT_EQ(mess_p.notfs[0]["method"], "textDocument/semanticHighlighting");
    ASSERT_EQ(mess_p.notfs[1]["method"], "textDocument/publishDiagnostics");
    diagnostics = mess_p.notfs[1]["params"]["diagnostics"];
    ASSERT_EQ(diagnostics.size(), (size_t)1);
    EXPECT_EQ(diagnostics[0]["code"].get<std::string>(), "S0003");

    mess_p.notfs.clear();
}

TEST(regress_test, stability)
{
    // test simulates an user typing. No results are expected, the server is just expected not to crash.
    workspace_manager ws_mngr;
    message_provider_mock mess_p;
    lsp::server s(ws_mngr);
    s.set_send_message_provider(&mess_p);

    // series of lsp notification calls
    auto notf = make_notification("textDocument/didOpen",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","languageId":"plaintext","version":1,"text":"LABEL LR 1,1 REMARK"}})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":2},"contentChanges":[{"range":{"start":{"line":0,"character":7},"end":{"line":0,"character":8}},"rangeLength":1,"text":""}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":3},"contentChanges":[{"range":{"start":{"line":0,"character":6},"end":{"line":0,"character":7}},"rangeLength":1,"text":""}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":4},"contentChanges":[{"range":{"start":{"line":0,"character":6},"end":{"line":0,"character":6}},"rangeLength":0,"text":"S"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":5},"contentChanges":[{"range":{"start":{"line":0,"character":7},"end":{"line":0,"character":7}},"rangeLength":0,"text":"A"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":6},"contentChanges":[{"range":{"start":{"line":0,"character":8},"end":{"line":0,"character":8}},"rangeLength":0,"text":"M"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":7},"contentChanges":[{"range":{"start":{"line":0,"character":9},"end":{"line":0,"character":9}},"rangeLength":0,"text":"2"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":8},"contentChanges":[{"range":{"start":{"line":0,"character":10},"end":{"line":0,"character":10}},"rangeLength":0,"text":"4"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":9},"contentChanges":[{"range":{"start":{"line":0,"character":10},"end":{"line":0,"character":11}},"rangeLength":1,"text":""}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":10},"contentChanges":[{"range":{"start":{"line":0,"character":9},"end":{"line":0,"character":10}},"rangeLength":1,"text":""}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":11},"contentChanges":[{"range":{"start":{"line":0,"character":8},"end":{"line":0,"character":9}},"rangeLength":1,"text":""}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":12},"contentChanges":[{"range":{"start":{"line":0,"character":7},"end":{"line":0,"character":8}},"rangeLength":1,"text":""}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":13},"contentChanges":[{"range":{"start":{"line":0,"character":6},"end":{"line":0,"character":7}},"rangeLength":1,"text":""}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":14},"contentChanges":[{"range":{"start":{"line":0,"character":6},"end":{"line":0,"character":6}},"rangeLength":0,"text":"L"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":15},"contentChanges":[{"range":{"start":{"line":0,"character":7},"end":{"line":0,"character":7}},"rangeLength":0,"text":"R"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":16},"contentChanges":[{"range":{"start":{"line":0,"character":9},"end":{"line":0,"character":10}},"rangeLength":1,"text":""}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":17},"contentChanges":[{"range":{"start":{"line":0,"character":9},"end":{"line":0,"character":9}},"rangeLength":0,"text":"2"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":18},"contentChanges":[{"range":{"start":{"line":0,"character":10},"end":{"line":0,"character":10}},"rangeLength":0,"text":"0"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":19},"contentChanges":[{"range":{"start":{"line":0,"character":10},"end":{"line":0,"character":11}},"rangeLength":1,"text":""}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":20},"contentChanges":[{"range":{"start":{"line":0,"character":9},"end":{"line":0,"character":10}},"rangeLength":1,"text":""}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":21},"contentChanges":[{"range":{"start":{"line":0,"character":9},"end":{"line":0,"character":9}},"rangeLength":0,"text":"1"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":22},"contentChanges":[{"range":{"start":{"line":0,"character":19},"end":{"line":0,"character":19}},"rangeLength":0,"text":"\r\n"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":23},"contentChanges":[{"range":{"start":{"line":1,"character":0},"end":{"line":1,"character":0}},"rangeLength":0,"text":"*"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":24},"contentChanges":[{"range":{"start":{"line":1,"character":1},"end":{"line":1,"character":1}},"rangeLength":0,"text":"K"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":25},"contentChanges":[{"range":{"start":{"line":1,"character":2},"end":{"line":1,"character":2}},"rangeLength":0,"text":"O"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":26},"contentChanges":[{"range":{"start":{"line":1,"character":3},"end":{"line":1,"character":3}},"rangeLength":0,"text":"M"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":27},"contentChanges":[{"range":{"start":{"line":1,"character":4},"end":{"line":1,"character":4}},"rangeLength":0,"text":"E"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":28},"contentChanges":[{"range":{"start":{"line":1,"character":5},"end":{"line":1,"character":5}},"rangeLength":0,"text":"N"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":29},"contentChanges":[{"range":{"start":{"line":1,"character":6},"end":{"line":1,"character":6}},"rangeLength":0,"text":"T"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":30},"contentChanges":[{"range":{"start":{"line":1,"character":7},"end":{"line":1,"character":7}},"rangeLength":0,"text":"A"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":31},"contentChanges":[{"range":{"start":{"line":1,"character":8},"end":{"line":1,"character":8}},"rangeLength":0,"text":"R"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":32},"contentChanges":[{"range":{"start":{"line":1,"character":9},"end":{"line":1,"character":9}},"rangeLength":0,"text":"\r\n"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":33},"contentChanges":[{"range":{"start":{"line":2,"character":0},"end":{"line":2,"character":0}},"rangeLength":0,"text":"\r\n"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":34},"contentChanges":[{"range":{"start":{"line":3,"character":0},"end":{"line":3,"character":0}},"rangeLength":0,"text":"&"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":35},"contentChanges":[{"range":{"start":{"line":3,"character":1},"end":{"line":3,"character":1}},"rangeLength":0,"text":"V"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":36},"contentChanges":[{"range":{"start":{"line":3,"character":2},"end":{"line":3,"character":2}},"rangeLength":0,"text":"A"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":37},"contentChanges":[{"range":{"start":{"line":3,"character":3},"end":{"line":3,"character":3}},"rangeLength":0,"text":"R"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":38},"contentChanges":[{"range":{"start":{"line":3,"character":4},"end":{"line":3,"character":4}},"rangeLength":0,"text":" "}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":39},"contentChanges":[{"range":{"start":{"line":3,"character":5},"end":{"line":3,"character":5}},"rangeLength":0,"text":"S"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":40},"contentChanges":[{"range":{"start":{"line":3,"character":6},"end":{"line":3,"character":6}},"rangeLength":0,"text":"E"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":41},"contentChanges":[{"range":{"start":{"line":3,"character":7},"end":{"line":3,"character":7}},"rangeLength":0,"text":"T"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":42},"contentChanges":[{"range":{"start":{"line":3,"character":8},"end":{"line":3,"character":8}},"rangeLength":0,"text":"A"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":43},"contentChanges":[{"range":{"start":{"line":3,"character":9},"end":{"line":3,"character":9}},"rangeLength":0,"text":" "}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":44},"contentChanges":[{"range":{"start":{"line":3,"character":10},"end":{"line":3,"character":10}},"rangeLength":0,"text":"5"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":45},"contentChanges":[{"range":{"start":{"line":3,"character":11},"end":{"line":3,"character":11}},"rangeLength":0,"text":"+"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":46},"contentChanges":[{"range":{"start":{"line":3,"character":12},"end":{"line":3,"character":12}},"rangeLength":0,"text":"5"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":47},"contentChanges":[{"range":{"start":{"line":3,"character":13},"end":{"line":3,"character":13}},"rangeLength":0,"text":")"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":48},"contentChanges":[{"range":{"start":{"line":3,"character":10},"end":{"line":3,"character":10}},"rangeLength":0,"text":"("}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":49},"contentChanges":[{"range":{"start":{"line":3,"character":15},"end":{"line":3,"character":15}},"rangeLength":0,"text":"\r\n"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":50},"contentChanges":[{"range":{"start":{"line":4,"character":0},"end":{"line":4,"character":0}},"rangeLength":0,"text":" "}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":51},"contentChanges":[{"range":{"start":{"line":4,"character":1},"end":{"line":4,"character":1}},"rangeLength":0,"text":"L"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":52},"contentChanges":[{"range":{"start":{"line":4,"character":2},"end":{"line":4,"character":2}},"rangeLength":0,"text":" "}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":53},"contentChanges":[{"range":{"start":{"line":4,"character":3},"end":{"line":4,"character":3}},"rangeLength":0,"text":"&"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":54},"contentChanges":[{"range":{"start":{"line":4,"character":4},"end":{"line":4,"character":4}},"rangeLength":0,"text":"V"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":55},"contentChanges":[{"range":{"start":{"line":4,"character":5},"end":{"line":4,"character":5}},"rangeLength":0,"text":"A"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":56},"contentChanges":[{"range":{"start":{"line":4,"character":6},"end":{"line":4,"character":6}},"rangeLength":0,"text":"R"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":57},"contentChanges":[{"range":{"start":{"line":4,"character":7},"end":{"line":4,"character":7}},"rangeLength":0,"text":","}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":58},"contentChanges":[{"range":{"start":{"line":4,"character":8},"end":{"line":4,"character":8}},"rangeLength":0,"text":"0"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":59},"contentChanges":[{"range":{"start":{"line":4,"character":9},"end":{"line":4,"character":9}},"rangeLength":0,"text":"("}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":60},"contentChanges":[{"range":{"start":{"line":4,"character":10},"end":{"line":4,"character":10}},"rangeLength":0,"text":"5"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":61},"contentChanges":[{"range":{"start":{"line":4,"character":11},"end":{"line":4,"character":11}},"rangeLength":0,"text":")"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":62},"contentChanges":[{"range":{"start":{"line":3,"character":15},"end":{"line":3,"character":15}},"rangeLength":0,"text":"\r\n"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":63},"contentChanges":[{"range":{"start":{"line":4,"character":0},"end":{"line":4,"character":0}},"rangeLength":0,"text":"&"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":64},"contentChanges":[{"range":{"start":{"line":4,"character":1},"end":{"line":4,"character":1}},"rangeLength":0,"text":"V"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":65},"contentChanges":[{"range":{"start":{"line":4,"character":2},"end":{"line":4,"character":2}},"rangeLength":0,"text":"A"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":66},"contentChanges":[{"range":{"start":{"line":4,"character":3},"end":{"line":4,"character":3}},"rangeLength":0,"text":"R"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":67},"contentChanges":[{"range":{"start":{"line":4,"character":4},"end":{"line":4,"character":4}},"rangeLength":0,"text":"2"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":68},"contentChanges":[{"range":{"start":{"line":4,"character":5},"end":{"line":4,"character":5}},"rangeLength":0,"text":" "}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":69},"contentChanges":[{"range":{"start":{"line":4,"character":6},"end":{"line":4,"character":6}},"rangeLength":0,"text":"S"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":70},"contentChanges":[{"range":{"start":{"line":4,"character":7},"end":{"line":4,"character":7}},"rangeLength":0,"text":"E"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":71},"contentChanges":[{"range":{"start":{"line":4,"character":8},"end":{"line":4,"character":8}},"rangeLength":0,"text":"T"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":72},"contentChanges":[{"range":{"start":{"line":4,"character":9},"end":{"line":4,"character":9}},"rangeLength":0,"text":"C"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":73},"contentChanges":[{"range":{"start":{"line":4,"character":10},"end":{"line":4,"character":10}},"rangeLength":0,"text":" "}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":74},"contentChanges":[{"range":{"start":{"line":4,"character":11},"end":{"line":4,"character":11}},"rangeLength":0,"text":"'"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":75},"contentChanges":[{"range":{"start":{"line":4,"character":12},"end":{"line":4,"character":12}},"rangeLength":0,"text":"'"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":76},"contentChanges":[{"range":{"start":{"line":4,"character":12},"end":{"line":4,"character":12}},"rangeLength":0,"text":"&"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":77},"contentChanges":[{"range":{"start":{"line":4,"character":13},"end":{"line":4,"character":13}},"rangeLength":0,"text":"V"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":78},"contentChanges":[{"range":{"start":{"line":4,"character":14},"end":{"line":4,"character":14}},"rangeLength":0,"text":"A"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":79},"contentChanges":[{"range":{"start":{"line":4,"character":15},"end":{"line":4,"character":15}},"rangeLength":0,"text":"R"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":80},"contentChanges":[{"range":{"start":{"line":4,"character":16},"end":{"line":4,"character":16}},"rangeLength":0,"text":","}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":81},"contentChanges":[{"range":{"start":{"line":4,"character":17},"end":{"line":4,"character":17}},"rangeLength":0,"text":"0"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":82},"contentChanges":[{"range":{"start":{"line":4,"character":18},"end":{"line":4,"character":18}},"rangeLength":0,"text":"("}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":83},"contentChanges":[{"range":{"start":{"line":4,"character":19},"end":{"line":4,"character":19}},"rangeLength":0,"text":"5"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":84},"contentChanges":[{"range":{"start":{"line":4,"character":20},"end":{"line":4,"character":20}},"rangeLength":0,"text":")"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":85},"contentChanges":[{"range":{"start":{"line":5,"character":11},"end":{"line":5,"character":12}},"rangeLength":1,"text":""}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":86},"contentChanges":[{"range":{"start":{"line":5,"character":10},"end":{"line":5,"character":11}},"rangeLength":1,"text":""}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":87},"contentChanges":[{"range":{"start":{"line":5,"character":9},"end":{"line":5,"character":10}},"rangeLength":1,"text":""}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":88},"contentChanges":[{"range":{"start":{"line":5,"character":8},"end":{"line":5,"character":9}},"rangeLength":1,"text":""}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":89},"contentChanges":[{"range":{"start":{"line":5,"character":7},"end":{"line":5,"character":8}},"rangeLength":1,"text":""}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":90},"contentChanges":[{"range":{"start":{"line":5,"character":7},"end":{"line":5,"character":7}},"rangeLength":0,"text":"2"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":91},"contentChanges":[{"range":{"start":{"line":3,"character":12},"end":{"line":3,"character":13}},"rangeLength":1,"text":""}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":92},"contentChanges":[{"range":{"start":{"line":3,"character":12},"end":{"line":3,"character":12}},"rangeLength":0,"text":"+"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":93},"contentChanges":[{"range":{"start":{"line":5,"character":8},"end":{"line":5,"character":8}},"rangeLength":0,"text":"\r\n"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":94},"contentChanges":[{"range":{"start":{"line":6,"character":0},"end":{"line":6,"character":0}},"rangeLength":0,"text":"\r\n"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":95},"contentChanges":[{"range":{"start":{"line":7,"character":0},"end":{"line":7,"character":0}},"rangeLength":0,"text":" "}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":96},"contentChanges":[{"range":{"start":{"line":7,"character":1},"end":{"line":7,"character":1}},"rangeLength":0,"text":"A"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":97},"contentChanges":[{"range":{"start":{"line":7,"character":2},"end":{"line":7,"character":2}},"rangeLength":0,"text":"G"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":98},"contentChanges":[{"range":{"start":{"line":7,"character":3},"end":{"line":7,"character":3}},"rangeLength":0,"text":"O"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":99},"contentChanges":[{"range":{"start":{"line":7,"character":4},"end":{"line":7,"character":4}},"rangeLength":0,"text":" "}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":100},"contentChanges":[{"range":{"start":{"line":7,"character":5},"end":{"line":7,"character":5}},"rangeLength":0,"text":"."}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":101},"contentChanges":[{"range":{"start":{"line":7,"character":6},"end":{"line":7,"character":6}},"rangeLength":0,"text":"H"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":102},"contentChanges":[{"range":{"start":{"line":7,"character":7},"end":{"line":7,"character":7}},"rangeLength":0,"text":"E"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":103},"contentChanges":[{"range":{"start":{"line":7,"character":8},"end":{"line":7,"character":8}},"rangeLength":0,"text":"R"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":104},"contentChanges":[{"range":{"start":{"line":7,"character":9},"end":{"line":7,"character":9}},"rangeLength":0,"text":"E"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":105},"contentChanges":[{"range":{"start":{"line":7,"character":10},"end":{"line":7,"character":10}},"rangeLength":0,"text":"\r\n"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":106},"contentChanges":[{"range":{"start":{"line":8,"character":0},"end":{"line":8,"character":0}},"rangeLength":0,"text":" "}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":107},"contentChanges":[{"range":{"start":{"line":8,"character":1},"end":{"line":8,"character":1}},"rangeLength":0,"text":"J"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":108},"contentChanges":[{"range":{"start":{"line":8,"character":2},"end":{"line":8,"character":2}},"rangeLength":0,"text":" "}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":109},"contentChanges":[{"range":{"start":{"line":8,"character":3},"end":{"line":8,"character":3}},"rangeLength":0,"text":"5"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":110},"contentChanges":[{"range":{"start":{"line":8,"character":4},"end":{"line":8,"character":4}},"rangeLength":0,"text":"\r\n"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":111},"contentChanges":[{"range":{"start":{"line":9,"character":0},"end":{"line":9,"character":0}},"rangeLength":0,"text":"."}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":112},"contentChanges":[{"range":{"start":{"line":9,"character":1},"end":{"line":9,"character":1}},"rangeLength":0,"text":"H"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":113},"contentChanges":[{"range":{"start":{"line":9,"character":2},"end":{"line":9,"character":2}},"rangeLength":0,"text":"E"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":114},"contentChanges":[{"range":{"start":{"line":9,"character":3},"end":{"line":9,"character":3}},"rangeLength":0,"text":"R"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":115},"contentChanges":[{"range":{"start":{"line":9,"character":4},"end":{"line":9,"character":4}},"rangeLength":0,"text":"E"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":116},"contentChanges":[{"range":{"start":{"line":9,"character":5},"end":{"line":9,"character":5}},"rangeLength":0,"text":" "}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":117},"contentChanges":[{"range":{"start":{"line":9,"character":6},"end":{"line":9,"character":6}},"rangeLength":0,"text":"A"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":118},"contentChanges":[{"range":{"start":{"line":9,"character":7},"end":{"line":9,"character":7}},"rangeLength":0,"text":"N"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":119},"contentChanges":[{"range":{"start":{"line":9,"character":8},"end":{"line":9,"character":8}},"rangeLength":0,"text":"O"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":120},"contentChanges":[{"range":{"start":{"line":9,"character":9},"end":{"line":9,"character":9}},"rangeLength":0,"text":"P"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":121},"contentChanges":[{"range":{"start":{"line":6,"character":0},"end":{"line":6,"character":0}},"rangeLength":0,"text":"\r\n"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":122},"contentChanges":[{"range":{"start":{"line":7,"character":0},"end":{"line":7,"character":0}},"rangeLength":0,"text":"&"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":123},"contentChanges":[{"range":{"start":{"line":7,"character":1},"end":{"line":7,"character":1}},"rangeLength":0,"text":"V"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":124},"contentChanges":[{"range":{"start":{"line":7,"character":2},"end":{"line":7,"character":2}},"rangeLength":0,"text":"A"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":125},"contentChanges":[{"range":{"start":{"line":7,"character":3},"end":{"line":7,"character":3}},"rangeLength":0,"text":"A"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":126},"contentChanges":[{"range":{"start":{"line":7,"character":3},"end":{"line":7,"character":4}},"rangeLength":1,"text":""}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":127},"contentChanges":[{"range":{"start":{"line":7,"character":3},"end":{"line":7,"character":3}},"rangeLength":0,"text":"R"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":128},"contentChanges":[{"range":{"start":{"line":7,"character":4},"end":{"line":7,"character":4}},"rangeLength":0,"text":"3"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":129},"contentChanges":[{"range":{"start":{"line":7,"character":5},"end":{"line":7,"character":5}},"rangeLength":0,"text":" "}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":130},"contentChanges":[{"range":{"start":{"line":7,"character":6},"end":{"line":7,"character":6}},"rangeLength":0,"text":"S"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":131},"contentChanges":[{"range":{"start":{"line":7,"character":7},"end":{"line":7,"character":7}},"rangeLength":0,"text":"E"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":132},"contentChanges":[{"range":{"start":{"line":7,"character":8},"end":{"line":7,"character":8}},"rangeLength":0,"text":"T"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":133},"contentChanges":[{"range":{"start":{"line":7,"character":9},"end":{"line":7,"character":9}},"rangeLength":0,"text":"B"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":134},"contentChanges":[{"range":{"start":{"line":7,"character":10},"end":{"line":7,"character":10}},"rangeLength":0,"text":" "}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":135},"contentChanges":[{"range":{"start":{"line":7,"character":11},"end":{"line":7,"character":11}},"rangeLength":0,"text":"("}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":136},"contentChanges":[{"range":{"start":{"line":7,"character":12},"end":{"line":7,"character":12}},"rangeLength":0,"text":"%"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":137},"contentChanges":[{"range":{"start":{"line":7,"character":13},"end":{"line":7,"character":13}},"rangeLength":0,"text":"V"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":138},"contentChanges":[{"range":{"start":{"line":7,"character":13},"end":{"line":7,"character":14}},"rangeLength":1,"text":""}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":139},"contentChanges":[{"range":{"start":{"line":7,"character":12},"end":{"line":7,"character":13}},"rangeLength":1,"text":""}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":140},"contentChanges":[{"range":{"start":{"line":7,"character":12},"end":{"line":7,"character":12}},"rangeLength":0,"text":"4"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":141},"contentChanges":[{"range":{"start":{"line":7,"character":12},"end":{"line":7,"character":13}},"rangeLength":1,"text":""}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":142},"contentChanges":[{"range":{"start":{"line":7,"character":12},"end":{"line":7,"character":12}},"rangeLength":0,"text":"&"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":143},"contentChanges":[{"range":{"start":{"line":7,"character":13},"end":{"line":7,"character":13}},"rangeLength":0,"text":"V"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":144},"contentChanges":[{"range":{"start":{"line":7,"character":14},"end":{"line":7,"character":14}},"rangeLength":0,"text":"A"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":145},"contentChanges":[{"range":{"start":{"line":7,"character":15},"end":{"line":7,"character":15}},"rangeLength":0,"text":"R"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":146},"contentChanges":[{"range":{"start":{"line":7,"character":16},"end":{"line":7,"character":16}},"rangeLength":0,"text":" "}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":147},"contentChanges":[{"range":{"start":{"line":7,"character":17},"end":{"line":7,"character":17}},"rangeLength":0,"text":"E"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":148},"contentChanges":[{"range":{"start":{"line":7,"character":18},"end":{"line":7,"character":18}},"rangeLength":0,"text":"Q"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":149},"contentChanges":[{"range":{"start":{"line":7,"character":19},"end":{"line":7,"character":19}},"rangeLength":0,"text":" "}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":150},"contentChanges":[{"range":{"start":{"line":7,"character":20},"end":{"line":7,"character":20}},"rangeLength":0,"text":"1"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":151},"contentChanges":[{"range":{"start":{"line":7,"character":21},"end":{"line":7,"character":21}},"rangeLength":0,"text":"0"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":152},"contentChanges":[{"range":{"start":{"line":7,"character":22},"end":{"line":7,"character":22}},"rangeLength":0,"text":")"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":153},"contentChanges":[{"range":{"start":{"line":8,"character":9},"end":{"line":8,"character":10}},"rangeLength":1,"text":""}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":154},"contentChanges":[{"range":{"start":{"line":8,"character":8},"end":{"line":8,"character":9}},"rangeLength":1,"text":""}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":155},"contentChanges":[{"range":{"start":{"line":8,"character":7},"end":{"line":8,"character":8}},"rangeLength":1,"text":""}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":156},"contentChanges":[{"range":{"start":{"line":8,"character":6},"end":{"line":8,"character":7}},"rangeLength":1,"text":""}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":157},"contentChanges":[{"range":{"start":{"line":8,"character":5},"end":{"line":8,"character":6}},"rangeLength":1,"text":""}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":158},"contentChanges":[{"range":{"start":{"line":8,"character":4},"end":{"line":8,"character":5}},"rangeLength":1,"text":""}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":159},"contentChanges":[{"range":{"start":{"line":8,"character":3},"end":{"line":8,"character":4}},"rangeLength":1,"text":""}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":160},"contentChanges":[{"range":{"start":{"line":8,"character":2},"end":{"line":8,"character":3}},"rangeLength":1,"text":""}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":161},"contentChanges":[{"range":{"start":{"line":8,"character":2},"end":{"line":8,"character":2}},"rangeLength":0,"text":"I"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":162},"contentChanges":[{"range":{"start":{"line":8,"character":3},"end":{"line":8,"character":3}},"rangeLength":0,"text":"F"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":163},"contentChanges":[{"range":{"start":{"line":8,"character":4},"end":{"line":8,"character":4}},"rangeLength":0,"text":" "}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":164},"contentChanges":[{"range":{"start":{"line":8,"character":5},"end":{"line":8,"character":5}},"rangeLength":0,"text":"("}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":165},"contentChanges":[{"range":{"start":{"line":8,"character":6},"end":{"line":8,"character":6}},"rangeLength":0,"text":"&"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":166},"contentChanges":[{"range":{"start":{"line":8,"character":7},"end":{"line":8,"character":7}},"rangeLength":0,"text":"V"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":167},"contentChanges":[{"range":{"start":{"line":8,"character":8},"end":{"line":8,"character":8}},"rangeLength":0,"text":"A"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":168},"contentChanges":[{"range":{"start":{"line":8,"character":9},"end":{"line":8,"character":9}},"rangeLength":0,"text":"R"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":169},"contentChanges":[{"range":{"start":{"line":8,"character":10},"end":{"line":8,"character":10}},"rangeLength":0,"text":"3"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":170},"contentChanges":[{"range":{"start":{"line":8,"character":11},"end":{"line":8,"character":11}},"rangeLength":0,"text":")"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":171},"contentChanges":[{"range":{"start":{"line":8,"character":12},"end":{"line":8,"character":12}},"rangeLength":0,"text":"."}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":172},"contentChanges":[{"range":{"start":{"line":8,"character":13},"end":{"line":8,"character":13}},"rangeLength":0,"text":"H"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":173},"contentChanges":[{"range":{"start":{"line":8,"character":14},"end":{"line":8,"character":14}},"rangeLength":0,"text":"E"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":174},"contentChanges":[{"range":{"start":{"line":8,"character":15},"end":{"line":8,"character":15}},"rangeLength":0,"text":"R"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":175},"contentChanges":[{"range":{"start":{"line":8,"character":16},"end":{"line":8,"character":16}},"rangeLength":0,"text":"E"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":176},"contentChanges":[{"range":{"start":{"line":7,"character":22},"end":{"line":7,"character":22}},"rangeLength":0,"text":"="}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":177},"contentChanges":[{"range":{"start":{"line":7,"character":22},"end":{"line":7,"character":23}},"rangeLength":1,"text":""}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":178},"contentChanges":[{"range":{"start":{"line":7,"character":21},"end":{"line":7,"character":22}},"rangeLength":1,"text":""}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":179},"contentChanges":[{"range":{"start":{"line":10,"character":10},"end":{"line":10,"character":10}},"rangeLength":0,"text":"\r\n"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":180},"contentChanges":[{"range":{"start":{"line":11,"character":0},"end":{"line":11,"character":0}},"rangeLength":0,"text":"\r\n"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":181},"contentChanges":[{"range":{"start":{"line":12,"character":0},"end":{"line":12,"character":0}},"rangeLength":0,"text":" "}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":182},"contentChanges":[{"range":{"start":{"line":12,"character":1},"end":{"line":12,"character":1}},"rangeLength":0,"text":"M"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":183},"contentChanges":[{"range":{"start":{"line":12,"character":2},"end":{"line":12,"character":2}},"rangeLength":0,"text":"A"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":184},"contentChanges":[{"range":{"start":{"line":12,"character":3},"end":{"line":12,"character":3}},"rangeLength":0,"text":"T"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":185},"contentChanges":[{"range":{"start":{"line":12,"character":4},"end":{"line":12,"character":4}},"rangeLength":0,"text":"R"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":186},"contentChanges":[{"range":{"start":{"line":12,"character":5},"end":{"line":12,"character":5}},"rangeLength":0,"text":"O"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":187},"contentChanges":[{"range":{"start":{"line":12,"character":5},"end":{"line":12,"character":6}},"rangeLength":1,"text":""}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":188},"contentChanges":[{"range":{"start":{"line":12,"character":4},"end":{"line":12,"character":5}},"rangeLength":1,"text":""}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":189},"contentChanges":[{"range":{"start":{"line":12,"character":3},"end":{"line":12,"character":4}},"rangeLength":1,"text":""}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":190},"contentChanges":[{"range":{"start":{"line":12,"character":3},"end":{"line":12,"character":3}},"rangeLength":0,"text":"C"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":191},"contentChanges":[{"range":{"start":{"line":12,"character":4},"end":{"line":12,"character":4}},"rangeLength":0,"text":"R"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":192},"contentChanges":[{"range":{"start":{"line":12,"character":5},"end":{"line":12,"character":5}},"rangeLength":0,"text":"O"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":193},"contentChanges":[{"range":{"start":{"line":12,"character":6},"end":{"line":12,"character":6}},"rangeLength":0,"text":"I"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":194},"contentChanges":[{"range":{"start":{"line":12,"character":7},"end":{"line":12,"character":7}},"rangeLength":0,"text":"N"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":195},"contentChanges":[{"range":{"start":{"line":12,"character":8},"end":{"line":12,"character":8}},"rangeLength":0,"text":"S"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":196},"contentChanges":[{"range":{"start":{"line":12,"character":9},"end":{"line":12,"character":9}},"rangeLength":0,"text":"T"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":197},"contentChanges":[{"range":{"start":{"line":12,"character":10},"end":{"line":12,"character":10}},"rangeLength":0,"text":"R"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":198},"contentChanges":[{"range":{"start":{"line":12,"character":11},"end":{"line":12,"character":11}},"rangeLength":0,"text":" "}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":199},"contentChanges":[{"range":{"start":{"line":12,"character":12},"end":{"line":12,"character":12}},"rangeLength":0,"text":"P"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":200},"contentChanges":[{"range":{"start":{"line":12,"character":13},"end":{"line":12,"character":13}},"rangeLength":0,"text":"A"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":201},"contentChanges":[{"range":{"start":{"line":12,"character":14},"end":{"line":12,"character":14}},"rangeLength":0,"text":"R"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":202},"contentChanges":[{"range":{"start":{"line":12,"character":15},"end":{"line":12,"character":15}},"rangeLength":0,"text":"A"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":203},"contentChanges":[{"range":{"start":{"line":12,"character":16},"end":{"line":12,"character":16}},"rangeLength":0,"text":"M"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":204},"contentChanges":[{"range":{"start":{"line":12,"character":17},"end":{"line":12,"character":17}},"rangeLength":0,"text":"1"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":205},"contentChanges":[{"range":{"start":{"line":12,"character":18},"end":{"line":12,"character":18}},"rangeLength":0,"text":","}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":206},"contentChanges":[{"range":{"start":{"line":12,"character":19},"end":{"line":12,"character":19}},"rangeLength":0,"text":"P"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":207},"contentChanges":[{"range":{"start":{"line":12,"character":20},"end":{"line":12,"character":20}},"rangeLength":0,"text":"A"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":208},"contentChanges":[{"range":{"start":{"line":12,"character":21},"end":{"line":12,"character":21}},"rangeLength":0,"text":"R"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":209},"contentChanges":[{"range":{"start":{"line":12,"character":22},"end":{"line":12,"character":22}},"rangeLength":0,"text":"A"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":210},"contentChanges":[{"range":{"start":{"line":12,"character":23},"end":{"line":12,"character":23}},"rangeLength":0,"text":"M"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":211},"contentChanges":[{"range":{"start":{"line":12,"character":24},"end":{"line":12,"character":24}},"rangeLength":0,"text":"2"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":212},"contentChanges":[{"range":{"start":{"line":12,"character":25},"end":{"line":12,"character":25}},"rangeLength":0,"text":","}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":213},"contentChanges":[{"range":{"start":{"line":12,"character":26},"end":{"line":12,"character":26}},"rangeLength":0,"text":"\r\n               "},{"range":{"start":{"line":12,"character":26},"end":{"line":12,"character":26}},"rangeLength":0,"text":"X"},{"range":{"start":{"line":12,"character":26},"end":{"line":12,"character":26}},"rangeLength":0,"text":"                                             "}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":214},"contentChanges":[{"range":{"start":{"line":13,"character":15},"end":{"line":13,"character":15}},"rangeLength":0,"text":"P"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":215},"contentChanges":[{"range":{"start":{"line":13,"character":16},"end":{"line":13,"character":16}},"rangeLength":0,"text":"A"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":216},"contentChanges":[{"range":{"start":{"line":13,"character":17},"end":{"line":13,"character":17}},"rangeLength":0,"text":"R"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":217},"contentChanges":[{"range":{"start":{"line":13,"character":18},"end":{"line":13,"character":18}},"rangeLength":0,"text":"A"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":218},"contentChanges":[{"range":{"start":{"line":13,"character":19},"end":{"line":13,"character":19}},"rangeLength":0,"text":"M"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":219},"contentChanges":[{"range":{"start":{"line":13,"character":20},"end":{"line":13,"character":20}},"rangeLength":0,"text":"3"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":220},"contentChanges":[{"range":{"start":{"line":13,"character":21},"end":{"line":13,"character":21}},"rangeLength":0,"text":" "}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":221},"contentChanges":[{"range":{"start":{"line":13,"character":22},"end":{"line":13,"character":22}},"rangeLength":0,"text":"r"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":222},"contentChanges":[{"range":{"start":{"line":13,"character":23},"end":{"line":13,"character":23}},"rangeLength":0,"text":"e"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":223},"contentChanges":[{"range":{"start":{"line":13,"character":24},"end":{"line":13,"character":24}},"rangeLength":0,"text":"m"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":224},"contentChanges":[{"range":{"start":{"line":13,"character":25},"end":{"line":13,"character":25}},"rangeLength":0,"text":"a"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":225},"contentChanges":[{"range":{"start":{"line":13,"character":26},"end":{"line":13,"character":26}},"rangeLength":0,"text":"r"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":226},"contentChanges":[{"range":{"start":{"line":13,"character":27},"end":{"line":13,"character":27}},"rangeLength":0,"text":"k"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":227},"contentChanges":[{"range":{"start":{"line":13,"character":28},"end":{"line":13,"character":28}},"rangeLength":0,"text":"\r\n               "},{"range":{"start":{"line":13,"character":28},"end":{"line":13,"character":28}},"rangeLength":0,"text":"X"},{"range":{"start":{"line":13,"character":28},"end":{"line":13,"character":28}},"rangeLength":0,"text":"                                           "}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":228},"contentChanges":[{"range":{"start":{"line":14,"character":15},"end":{"line":14,"character":15}},"rangeLength":0,"text":"r"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":229},"contentChanges":[{"range":{"start":{"line":14,"character":16},"end":{"line":14,"character":16}},"rangeLength":0,"text":"e"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":230},"contentChanges":[{"range":{"start":{"line":14,"character":17},"end":{"line":14,"character":17}},"rangeLength":0,"text":"m"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":231},"contentChanges":[{"range":{"start":{"line":14,"character":18},"end":{"line":14,"character":18}},"rangeLength":0,"text":"a"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":232},"contentChanges":[{"range":{"start":{"line":14,"character":19},"end":{"line":14,"character":19}},"rangeLength":0,"text":"r"}]})#"_json);
    s.message_received(notf);
    notf = make_notification("textDocument/didChange",
        R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm","version":233},"contentChanges":[{"range":{"start":{"line":14,"character":20},"end":{"line":14,"character":20}},"rangeLength":0,"text":"k"}]})#"_json);
    s.message_received(notf);
}

#endif // !HLASMPLUGIN_LANGUAGESERVER_TEST_REGRESS_TEST_H
