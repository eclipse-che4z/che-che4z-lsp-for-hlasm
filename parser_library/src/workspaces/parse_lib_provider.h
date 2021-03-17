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

#include "analyzing_context.h"

namespace hlasm_plugin::parser_library::workspaces {

using parse_result = bool;

struct library_data
{
    processing::processing_kind proc_kind;
    context::id_index library_member;
};
// Interface that the analyzer uses to parse macros and COPY files in separate files (libraries).
class parse_lib_provider
{
public:
    // Parses library with specified name and saves it into context.
    // Library data passes information whether COPY or macro is going to be parsed.
    virtual parse_result parse_library(const std::string& library, analyzing_context ctx, const library_data data) = 0;

    virtual bool has_library(const std::string& library, const std::string& program) const = 0;

    virtual const asm_option& get_asm_options(const std::string&) = 0;

    virtual ~parse_lib_provider() = default;
};

// Parse lib provider that does not provide any libraries.
class empty_parse_lib_provider : public parse_lib_provider
{
    asm_option asm_opts;

public:
    virtual parse_result parse_library(const std::string&, analyzing_context, const library_data) override
    {
        return false;
    };
    virtual bool has_library(const std::string&, const std::string&) const override { return false; };

    const asm_option& get_asm_options(const std::string&) override { return asm_opts; };
    static empty_parse_lib_provider instance;
};


} // namespace hlasm_plugin::parser_library::workspaces

#endif // HLASMPLUGIN_PARSERLIBRARY_PARSE_LIB_PROVIDER_H