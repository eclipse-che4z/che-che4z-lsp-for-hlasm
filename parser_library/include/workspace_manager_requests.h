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

#ifndef HLASMPLUGIN_PARSERLIBRARY_WORKSPACE_MANAGER_REQUESTS_H
#define HLASMPLUGIN_PARSERLIBRARY_WORKSPACE_MANAGER_REQUESTS_H

#include "workspace_manager_response.h"


namespace hlasm_plugin::parser_library {

class workspace_manager_requests
{
protected:
    ~workspace_manager_requests() = default;

public:
    virtual void request_workspace_configuration(
        const char* url, workspace_manager_response<sequence<char>> json_text) = 0;
};

} // namespace hlasm_plugin::parser_library

#endif
