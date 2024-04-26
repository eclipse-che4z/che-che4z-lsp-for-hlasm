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

#ifndef HLASMPLUGIN_PARSERLIBRARY_PARSE_LIB_PROVIDER_H
#define HLASMPLUGIN_PARSERLIBRARY_PARSE_LIB_PROVIDER_H

#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include "processing_format.h"
#include "utils/resource_location.h"
#include "utils/task.h"

namespace hlasm_plugin::parser_library {
struct analyzing_context;

// Interface that the analyzer uses to parse macros and COPY files in separate files (libraries).
class parse_lib_provider
{
public:
    // Parses library with specified name and saves it into context.
    // Library data passes information whether COPY or macro is going to be parsed.
    [[nodiscard]] virtual utils::value_task<bool> parse_library(
        std::string library, analyzing_context ctx, processing::processing_kind proc_kind) = 0;

    virtual bool has_library(std::string_view library, utils::resource::resource_location* url) = 0;

    [[nodiscard]] virtual utils::value_task<std::optional<std::pair<std::string, utils::resource::resource_location>>>
    get_library(std::string library) = 0;

protected:
    ~parse_lib_provider() = default;
};

} // namespace hlasm_plugin::parser_library

#endif // HLASMPLUGIN_PARSERLIBRARY_PARSE_LIB_PROVIDER_H
