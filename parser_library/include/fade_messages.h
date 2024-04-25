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

#ifndef HLASMPLUGIN_PARSERLIBRARY_FADE_MESSAGES_H
#define HLASMPLUGIN_PARSERLIBRARY_FADE_MESSAGES_H

#include <string>
#include <string_view>

#include "range.h"

namespace hlasm_plugin::parser_library {
struct fade_message
{
    std::string code;
    std::string message;
    static constexpr const std::string_view source = "HLASM Plugin";
    std::string uri;
    range r;

    fade_message(std::string code, std::string message, std::string uri, range r);

    static fade_message preprocessor_statement(std::string uri, const range& range);
    static fade_message inactive_statement(std::string uri, const range& range);
    static fade_message unused_macro(std::string uri, const range& range);
};

} // namespace hlasm_plugin::parser_library

#endif
