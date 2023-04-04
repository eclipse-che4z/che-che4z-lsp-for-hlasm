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

#ifndef HLASMPLUGIN_LANGUAGESERVER_TEST_WS_MNGR_MOCK_H
#define HLASMPLUGIN_LANGUAGESERVER_TEST_WS_MNGR_MOCK_H

#include "gmock/gmock.h"

#include "workspace_manager.h"
#include "workspace_manager_response.h"

namespace hlasm_plugin::language_server::test {

using namespace parser_library;

class ws_mngr_mock : public workspace_manager
{
public:
    MOCK_METHOD2(get_workspaces, size_t(ws_id* workspaces, size_t max_size));
    MOCK_METHOD0(get_workspaces_count, size_t());
    MOCK_METHOD2(add_workspace, void(const char* name, const char* uri));
    MOCK_METHOD1(find_workspace, ws_id(const char* document_uri));
    MOCK_METHOD1(remove_workspace, void(const char* uri));

    MOCK_METHOD4(
        did_open_file, void(const char* document_uri, version_t version, const char* text, size_t text_length));
    MOCK_METHOD4(did_change_file,
        void(const char* document_uri, version_t version, const document_change* changes, size_t ch_size));
    MOCK_METHOD1(did_close_file, void(const char* document_uri));

    MOCK_METHOD2(configuration_changed, void(const lib_config& new_config, const char* whole_settings));

    MOCK_METHOD(void,
        definition,
        (const char* document_uri, position pos, workspace_manager_response<position_uri>),
        (override));
    MOCK_METHOD(void,
        references,
        (const char* document_uri, position pos, workspace_manager_response<position_uri_list>),
        (override));
    MOCK_METHOD(
        void, hover, (const char* document_uri, position pos, workspace_manager_response<sequence<char>>), (override));
    MOCK_METHOD(void,
        completion,
        (const char* document_uri,
            position pos,
            const char trigger_char,
            completion_trigger_kind trigger_kind,
            workspace_manager_response<completion_list>),
        (override));


    MOCK_METHOD(
        void, semantic_tokens, (const char*, workspace_manager_response<continuous_sequence<token_info>>), (override));
    MOCK_METHOD(void,
        document_symbol,
        (const char*, long long limit, workspace_manager_response<document_symbol_list>),
        (override));

    MOCK_METHOD(continuous_sequence<char>, get_virtual_file_content, (unsigned long long id), (const override));


    MOCK_METHOD(void,
        make_opcode_suggestion,
        (const char* document_uri,
            const char* opcode,
            bool extended,
            workspace_manager_response<continuous_sequence<opcode_suggestion>>),
        (const override));
};

} // namespace hlasm_plugin::language_server::test
#endif // !HLASMPLUGIN_LANGUAGESERVER_TEST_WS_MNGR_MOCK_H
