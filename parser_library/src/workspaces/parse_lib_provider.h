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

#include <compare>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include "analyzing_context.h"
#include "context/id_index.h"
#include "processing/processing_format.h"
#include "utils/resource_location.h"
#include "utils/task.h"

namespace hlasm_plugin::parser_library::workspaces {

struct library_data
{
    processing::processing_kind proc_kind;
    context::id_index library_member;

    constexpr auto operator<=>(const library_data&) const = default;
};

// Interface that the analyzer uses to parse macros and COPY files in separate files (libraries).
class parse_lib_provider
{
public:
    // Parses library with specified name and saves it into context.
    // Library data passes information whether COPY or macro is going to be parsed.
    [[nodiscard]] virtual utils::value_task<bool> parse_library(
        std::string library, analyzing_context ctx, library_data data) = 0;

    virtual bool has_library(std::string_view library, utils::resource::resource_location* url) = 0;

    [[nodiscard]] virtual utils::value_task<std::optional<std::pair<std::string, utils::resource::resource_location>>>
    get_library(std::string library) = 0;

protected:
    ~parse_lib_provider() = default;
};

// Parse lib provider that does not provide any libraries.
class empty_parse_lib_provider final : public parse_lib_provider
{
public:
    [[nodiscard]] utils::value_task<bool> parse_library(std::string, analyzing_context, library_data) override;
    bool has_library(std::string_view, utils::resource::resource_location*) override;
    [[nodiscard]] utils::value_task<std::optional<std::pair<std::string, utils::resource::resource_location>>>
        get_library(std::string) override;

    static empty_parse_lib_provider instance;
};


} // namespace hlasm_plugin::parser_library::workspaces

#endif // HLASMPLUGIN_PARSERLIBRARY_PARSE_LIB_PROVIDER_H
