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
    MOCK_METHOD(void, add_workspace, (std::string_view name, std::string_view uri), (override));
    MOCK_METHOD(void, remove_workspace, (std::string_view uri), (override));

    MOCK_METHOD(
        void, did_open_file, (std::string_view document_uri, version_t version, std::string_view text), (override));
    MOCK_METHOD(void,
        did_change_file,
        (std::string_view document_uri, version_t version, std::span<const document_change> changes),
        (override));
    MOCK_METHOD(void, did_close_file, (std::string_view document_uri), (override));
    MOCK_METHOD(void, did_change_watched_files, (std::span<const fs_change> changes), (override));

    MOCK_METHOD(void,
        definition,
        (std::string_view document_uri, position pos, workspace_manager_response<const location&>),
        (override));
    MOCK_METHOD(void,
        references,
        (std::string_view document_uri, position pos, workspace_manager_response<std::span<const location>>),
        (override));
    MOCK_METHOD(void,
        hover,
        (std::string_view document_uri, position pos, workspace_manager_response<std::string_view>),
        (override));
    MOCK_METHOD(void,
        completion,
        (std::string_view document_uri,
            position pos,
            const char trigger_char,
            completion_trigger_kind trigger_kind,
            workspace_manager_response<std::span<const completion_item>>),
        (override));


    MOCK_METHOD(
        void, semantic_tokens, (std::string_view, workspace_manager_response<std::span<const token_info>>), (override));
    MOCK_METHOD(void,
        document_symbol,
        (std::string_view, workspace_manager_response<std::span<const document_symbol_item>>),
        (override));

    MOCK_METHOD(void, configuration_changed, (const lib_config& new_config, std::string_view full_cfg), (override));

    MOCK_METHOD(void, register_diagnostics_consumer, (diagnostics_consumer * consumer), (override));
    MOCK_METHOD(void, unregister_diagnostics_consumer, (diagnostics_consumer * consumer), (override));
    MOCK_METHOD(void, register_parsing_metadata_consumer, (parsing_metadata_consumer * consumer), (override));
    MOCK_METHOD(void, unregister_parsing_metadata_consumer, (parsing_metadata_consumer * consumer), (override));
    MOCK_METHOD(void, set_message_consumer, (message_consumer * consumer), (override));
    MOCK_METHOD(void, set_request_interface, (workspace_manager_requests * requests), (override));

    MOCK_METHOD(std::string, get_virtual_file_content, (unsigned long long id), (const, override));
    MOCK_METHOD(void, toggle_advisory_configuration_diagnostics, (), (override));


    MOCK_METHOD(void,
        make_opcode_suggestion,
        (std::string_view document_uri,
            std::string_view opcode,
            bool extended,
            workspace_manager_response<std::span<const opcode_suggestion>>),
        (override));

    MOCK_METHOD(void, idle_handler, (const std::atomic<unsigned char>* yield_indicator), (override));

    MOCK_METHOD(debugger_configuration_provider&, get_debugger_configuration_provider, (), (override));

    MOCK_METHOD(void, invalidate_external_configuration, (std::string_view uri), (override));

    MOCK_METHOD(void,
        branch_information,
        (std::string_view document_uri, workspace_manager_response<std::span<const branch_info>> resp),
        (override));

    MOCK_METHOD(void,
        folding,
        (std::string_view document_uri, workspace_manager_response<std::span<const folding_range>> resp),
        (override));

    MOCK_METHOD(void,
        retrieve_output,
        (std::string_view document_uri, workspace_manager_response<std::span<const output_line>> resp),
        (override));
};

} // namespace hlasm_plugin::language_server::test
#endif // !HLASMPLUGIN_LANGUAGESERVER_TEST_WS_MNGR_MOCK_H
