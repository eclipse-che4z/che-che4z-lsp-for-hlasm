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

#ifndef HLASMPLUGIN_PARSERLIBRARY_EXTERNAL_CONFIGURATION_REQUESTS_H
#define HLASMPLUGIN_PARSERLIBRARY_EXTERNAL_CONFIGURATION_REQUESTS_H

#include <string_view>

#include "workspace_manager_response.h"

namespace hlasm_plugin::parser_library {

class external_configuration_requests
{
protected:
    ~external_configuration_requests() = default;

public:
    virtual void read_external_configuration(
        std::string_view url, workspace_manager_response<std::string_view> content) = 0;
};

} // namespace hlasm_plugin::parser_library

#endif
