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

#ifndef HLASMPLUGIN_PARSERLIBRARY_EMPTY_PARSE_LIB_PROVIDER_H
#define HLASMPLUGIN_PARSERLIBRARY_EMPTY_PARSE_LIB_PROVIDER_H

#include "parse_lib_provider.h"

namespace hlasm_plugin::parser_library {

// Parse lib provider that does not provide any libraries.
class empty_parse_lib_provider final : public parse_lib_provider
{
public:
    [[nodiscard]] utils::value_task<bool> parse_library(
        std::string, analyzing_context, processing::processing_kind) override;
    bool has_library(std::string_view, utils::resource::resource_location*) override;
    [[nodiscard]] utils::value_task<std::optional<std::pair<std::string, utils::resource::resource_location>>>
        get_library(std::string) override;

    static empty_parse_lib_provider instance;
};


} // namespace hlasm_plugin::parser_library

#endif // HLASMPLUGIN_PARSERLIBRARY_EMPTY_PARSE_LIB_PROVIDER_H
