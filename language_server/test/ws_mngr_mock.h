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

using namespace hlasm_plugin::parser_library;

class ws_mngr_mock : public workspace_manager
{
public:
    MOCK_METHOD2(get_workspaces, size_t(ws_id* workspaces, size_t max_size));
    MOCK_METHOD0(get_workspaces_count, size_t());
    MOCK_METHOD2(add_workspace, void(const char* name, const char* uri));
    MOCK_METHOD1(remove_workspace, void(const char* uri));

    MOCK_METHOD4(
        did_open_file, void(const char* document_uri, version_t version, const char* text, size_t text_length));
    MOCK_METHOD4(did_change_file,
        void(const char* document_uri, version_t version, const document_change* changes, size_t ch_size));
    MOCK_METHOD1(did_close_file, void(const char* document_uri));

    MOCK_METHOD1(configuration_changed, void(const lib_config& new_config));

    MOCK_METHOD(position_uri, definition, (const char* document_uri, const position pos), (override));
    MOCK_METHOD(position_uris, references, (const char* document_uri, const position pos), (override));
    MOCK_METHOD(string_array, hover, (const char* document_uri, const position pos), (override));
    MOCK_METHOD(completion_list,
        completion,
        (const char* document_uri, const position pos, const char trigger_char, int trigger_kind),
        (override));
};

#endif // !HLASMPLUGIN_LANGUAGESERVER_TEST_WS_MNGR_MOCK_H
