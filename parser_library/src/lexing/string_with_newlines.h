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

#ifndef HLASMPLUGIN_PARSERLIBRARY_LEXING_STRING_WITH_NEWLINES_H
#define HLASMPLUGIN_PARSERLIBRARY_LEXING_STRING_WITH_NEWLINES_H

#include <string>
#include <string_view>

namespace hlasm_plugin::parser_library::lexing {

struct u8string_with_newlines
{
    std::string text;

    u8string_with_newlines() = default;
    explicit u8string_with_newlines(std::string t)
        : text(std::move(t))
    {}
};

struct u8string_view_with_newlines
{
    static constexpr const char8_t EOL = (char8_t)-1;

    std::string_view text;

    explicit constexpr u8string_view_with_newlines(u8string_with_newlines&& t)
        : text(t.text)
    {}
    constexpr u8string_view_with_newlines(const u8string_with_newlines& t)
        : text(t.text)
    {}
    explicit constexpr u8string_view_with_newlines(std::string_view t)
        : text(t)
    {}
};
} // namespace hlasm_plugin::parser_library::lexing

#endif
