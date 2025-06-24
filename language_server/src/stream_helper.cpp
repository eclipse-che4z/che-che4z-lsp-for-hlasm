/*
 * Copyright (c) 2025 Broadcom.
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

#include "stream_helper.h"

#include <array>
#include <ios>
#include <locale>

namespace hlasm_plugin::language_server {

void imbue_stream_newline_is_space(std::ios& stream)
{
    // A struct that can be imbued into std::iostream, so it recognizes
    // only newline and nothing else as a whitespace. The dispatcher
    // expects a stream like that in the constructor.
    static constexpr auto masks = []() {
        std::array<std::ctype<char>::mask, std::ctype<char>::table_size> result {};
        result[(unsigned char)'\n'] = std::ctype_base::space;

        return result;
    }();
    stream.imbue(std::locale(stream.getloc(), new std::ctype<char>(masks.data())));
}

} // namespace hlasm_plugin::language_server
