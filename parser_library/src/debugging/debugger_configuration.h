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

#ifndef HLASMPLUGIN_PARSERLIBRARY_DEBUGGING_DEBUGGER_CONFIGURATION_H
#define HLASMPLUGIN_PARSERLIBRARY_DEBUGGING_DEBUGGER_CONFIGURATION_H

#include <memory>
#include <vector>

#include "compiler_options.h"
#include "preprocessor_options.h"
#include "utils/resource_location.h"
#include "workspaces/file_manager.h"
#include "workspaces/library.h"

namespace hlasm_plugin::parser_library::debugging {

struct debugger_configuration
{
    workspaces::file_manager* fm = nullptr;
    std::vector<std::shared_ptr<workspaces::library>> libraries;
    utils::resource::resource_location workspace_uri = utils::resource::resource_location();
    asm_option opts;
    std::vector<preprocessor_options> pp_opts;
};

} // namespace hlasm_plugin::parser_library::debugging

#endif
