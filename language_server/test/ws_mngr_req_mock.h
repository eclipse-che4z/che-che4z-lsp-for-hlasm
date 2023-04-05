/*
 * Copyright (c) 2023 Broadcom.
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

#ifndef HLASMPLUGIN_LANGUAGESERVER_TEST_WS_MNGR_REQ_MOCK_H
#define HLASMPLUGIN_LANGUAGESERVER_TEST_WS_MNGR_REQ_MOCK_H

#include "gmock/gmock.h"

#include "workspace_manager_requests.h"

class workspace_manager_requests_mock : public hlasm_plugin::parser_library::workspace_manager_requests
{
public:
    MOCK_METHOD(void,
        request_workspace_configuration,
        (const char* url,
            hlasm_plugin::parser_library::workspace_manager_response<hlasm_plugin::parser_library::sequence<char>>
                json_text),
        (override));
};

#endif
