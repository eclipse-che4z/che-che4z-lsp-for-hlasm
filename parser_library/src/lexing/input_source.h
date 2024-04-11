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

#ifndef HLASMPLUGIN_PARSER_HLASMINPUTSOURCE_H
#define HLASMPLUGIN_PARSER_HLASMINPUTSOURCE_H

#include <string>
#include <string_view>

#include "ANTLRInputStream.h"

#include "logical_line.h"
#include "utils/unicode_text.h"

namespace hlasm_plugin::parser_library::lexing {
/*
custom ANTLRInputStream
supports input rewinding, appending and resetting
*/
class input_source final : public antlr4::ANTLRInputStream
{
public:
    struct char_substitution
    {
        uint8_t server : 1;
        uint8_t client : 1;

        char_substitution& operator|=(const char_substitution& other)
        {
            server |= other.server;
            client |= other.client;
            return *this;
        }
    };

    input_source() = default;
    explicit input_source(std::string_view input); // for testing only

    char_substitution append(std::u32string_view str);
    char_substitution append(std::string_view str);
    char_substitution new_input(std::string_view str);
    char_substitution new_input(
        const logical_line<utils::utf8_iterator<std::string_view::iterator, utils::utf8_utf16_counter>>& l);

    input_source(const input_source&) = delete;
    input_source& operator=(const input_source&) = delete;
    input_source& operator=(input_source&&) = delete;
    input_source(input_source&&) = delete;

    std::string getText(const antlr4::misc::Interval& interval) override;
};

} // namespace hlasm_plugin::parser_library::lexing

#endif
