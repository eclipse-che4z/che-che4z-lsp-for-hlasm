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

    MOCK_METHOD(position_uri, definition, (const char* document_uri, position pos), (override));
    MOCK_METHOD(position_uri_list, references, (const char* document_uri, position pos), (override));
    MOCK_METHOD(sequence<char>, hover, (const char* document_uri, position pos), (override));
    MOCK_METHOD(completion_list,
        completion,
        (const char* document_uri, position pos, const char trigger_char, completion_trigger_kind trigger_kind),
        (override));

    MOCK_METHOD(continuous_sequence<char>, get_virtual_file_content, (unsigned long long id), (const override));


    MOCK_METHOD(continuous_sequence<opcode_suggestion>,
        make_opcode_suggestion,
        (const char* document_uri, const char* opcode, bool extended),
        (const override));
};

} // namespace hlasm_plugin::language_server::test
#endif // !HLASMPLUGIN_LANGUAGESERVER_TEST_WS_MNGR_MOCK_H
