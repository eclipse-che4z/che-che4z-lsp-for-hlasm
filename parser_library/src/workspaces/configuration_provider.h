/*
 * Copyright (c) 2024 Broadcom.
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

#ifndef HLASMPLUGIN_PARSERLIBRARY_CONFIGURATION_PROVIDER_H
#define HLASMPLUGIN_PARSERLIBRARY_CONFIGURATION_PROVIDER_H

#include <cstdint>
#include <memory>
#include <vector>

#include "compiler_options.h"
#include "preprocessor_options.h"
#include "tagged_index.h"
#include "utils/resource_location.h"
#include "utils/task.h"

namespace hlasm_plugin::parser_library::workspaces {

class library;
class processor_group;

struct analyzer_configuration
{
    std::vector<std::shared_ptr<workspaces::library>> libraries;
    asm_option opts;
    std::vector<preprocessor_options> pp_opts;
    utils::resource::resource_location alternative_config_url;
    std::int64_t dig_suppress_limit;
};

struct opcode_suggestion_data
{
    asm_option opts;
    processor_group* proc_grp;
};

class configuration_provider
{
protected:
    ~configuration_provider() = default;

public:
    [[nodiscard]] virtual utils::value_task<
        std::pair<analyzer_configuration, index_t<processor_group, unsigned long long>>>
    get_analyzer_configuration(utils::resource::resource_location url) = 0;
    [[nodiscard]] virtual opcode_suggestion_data get_opcode_suggestion_data(
        const utils::resource::resource_location& url) = 0;
};

} // namespace hlasm_plugin::parser_library::workspaces

#endif
