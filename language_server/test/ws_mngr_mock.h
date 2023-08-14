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
    MOCK_METHOD(void, add_workspace, (const char* name, const char* uri), (override));
    MOCK_METHOD(void, remove_workspace, (const char* uri), (override));

    MOCK_METHOD(void,
        did_open_file,
        (const char* document_uri, version_t version, const char* text, size_t text_length),
        (override));
    MOCK_METHOD(void,
        did_change_file,
        (const char* document_uri, version_t version, const document_change* changes, size_t ch_size),
        (override));
    MOCK_METHOD(void, did_close_file, (const char* document_uri), (override));
    MOCK_METHOD(void, did_change_watched_files, (sequence<fs_change> changes), (override));

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

    MOCK_METHOD(void, configuration_changed, (const lib_config& new_config), (override));

    MOCK_METHOD(void, register_diagnostics_consumer, (diagnostics_consumer * consumer), (override));
    MOCK_METHOD(void, unregister_diagnostics_consumer, (diagnostics_consumer * consumer), (override));
    MOCK_METHOD(void, register_parsing_metadata_consumer, (parsing_metadata_consumer * consumer), (override));
    MOCK_METHOD(void, unregister_parsing_metadata_consumer, (parsing_metadata_consumer * consumer), (override));
    MOCK_METHOD(void, set_message_consumer, (message_consumer * consumer), (override));
    MOCK_METHOD(void, set_request_interface, (workspace_manager_requests * requests), (override));

    MOCK_METHOD(continuous_sequence<char>, get_virtual_file_content, (unsigned long long id), (const, override));
    MOCK_METHOD(void, toggle_advisory_configuration_diagnostics, (), (override));


    MOCK_METHOD(void,
        make_opcode_suggestion,
        (const char* document_uri,
            const char* opcode,
            bool extended,
            workspace_manager_response<continuous_sequence<opcode_suggestion>>),
        (override));

    MOCK_METHOD(void, idle_handler, (const std::atomic<unsigned char>* yield_indicator), (override));

    MOCK_METHOD(debugger_configuration_provider&, get_debugger_configuration_provider, (), (override));

    MOCK_METHOD(void, invalidate_external_configuration, (sequence<char> uri), (override));
};

} // namespace hlasm_plugin::language_server::test
#endif // !HLASMPLUGIN_LANGUAGESERVER_TEST_WS_MNGR_MOCK_H
