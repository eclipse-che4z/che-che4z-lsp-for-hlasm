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

#include "antlr4-runtime.h"

#include "parser_library_export.h"

namespace hlasm_plugin::parser_library::lexing {
struct logical_line;
/*
    custom ANTLRInputStream
    supports input rewinding, appending and resetting
*/
class input_source final : public antlr4::ANTLRInputStream
{
public:
    input_source(const std::string& input);

    void append(const UTF32String& str);
    void append(std::string_view str);
    using antlr4::ANTLRInputStream::reset;
    void reset(std::string_view str);
    void reset(const logical_line& l);

    input_source(const input_source&) = delete;
    input_source& operator=(const input_source&) = delete;
    input_source& operator=(input_source&&) = delete;
    input_source(input_source&&) = delete;

    std::string getText(const antlr4::misc::Interval& interval) override;
};

} // namespace hlasm_plugin::parser_library::lexing

#endif
