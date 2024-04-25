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

#include "fade_messages.h"

namespace hlasm_plugin::parser_library {

fade_message fade_message::preprocessor_statement(std::string uri, const range& range)
{
    return fade_message("PREP", "Statement processed by a preprocessor", std::move(uri), range);
}

fade_message fade_message::inactive_statement(std::string uri, const range& range)
{
    return fade_message("INACT", "Inactive statement (based on opened files)", std::move(uri), range);
}

fade_message fade_message::unused_macro(std::string uri, const range& range)
{
    return fade_message("MAC_UNUSED", "Macro defined but not used (based on opened files)", std::move(uri), range);
}

fade_message::fade_message(std::string code, std::string message, std::string uri, range r)
    : code(std::move(code))
    , message(std::move(message))
    , uri(std::move(uri))
    , r(std::move(r))
{}

} // namespace hlasm_plugin::parser_library
