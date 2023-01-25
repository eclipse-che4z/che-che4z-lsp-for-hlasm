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
#include <utility>

#include "protocol.h"

namespace hlasm_plugin::parser_library {
struct fade_message_s
{
    std::string code;
    std::string message;
    std::string uri;
    range r;
    inline static const std::string source = "HLASM Plugin";

    fade_message_s(std::string code, std::string message, std::string uri, range r)
        : code(std::move(code))
        , message(std::move(message))
        , uri(std::move(uri))
        , r(std::move(r)) {};

    static fade_message_s preprocessor_statement(std::string uri, const range& range);
};

} // namespace hlasm_plugin::parser_library

#endif
