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
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include "analyzing_context.h"
#include "context/id_storage.h"
#include "processing/processing_format.h"
#include "utils/resource_location.h"

namespace hlasm_plugin::parser_library::workspaces {

using parse_result = bool;

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
    virtual parse_result parse_library(std::string_view library, analyzing_context ctx, library_data data) = 0;

    virtual bool has_library(std::string_view library, utils::resource::resource_location* url) const = 0;

    virtual std::optional<std::pair<std::string, utils::resource::resource_location>> get_library(
        std::string_view library) const = 0;

protected:
    ~parse_lib_provider() = default;
};

// Parse lib provider that does not provide any libraries.
class empty_parse_lib_provider final : public parse_lib_provider
{
public:
    parse_result parse_library(std::string_view, analyzing_context, library_data) override { return false; };
    bool has_library(std::string_view, utils::resource::resource_location*) const override { return false; };
    std::optional<std::pair<std::string, utils::resource::resource_location>> get_library(
        std::string_view) const override
    {
        return std::nullopt;
    }

    static empty_parse_lib_provider instance;
};


} // namespace hlasm_plugin::parser_library::workspaces

#endif // HLASMPLUGIN_PARSERLIBRARY_PARSE_LIB_PROVIDER_H
