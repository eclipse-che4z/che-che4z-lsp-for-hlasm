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

#ifndef LSP_TEXT_DATA_REF_T_H
#define LSP_TEXT_DATA_REF_T_H

#include <string>
#include <vector>

#include "protocol.h"

namespace hlasm_plugin::parser_library::lsp {

struct text_data_ref_t
{
    const std::string& text;
    std::vector<size_t> line_indices;

    text_data_ref_t();
    explicit text_data_ref_t(const std::string& text);

    // Returns a specified line from the text, zero-based indexed.
    // If the line does not exist, returns empty string view
    std::string_view get_line(size_t line) const;

    // Returns a string_view beginning at specified line and ending at specified column
    std::string_view get_line_beginning(position pos) const;

    // Returns a string_view beginning at specified position and ending at specified position
    std::string_view get_range_content(range pos) const;

    // Returns a character before the specified position
    // If the position points to the first character, returns '\0'
    char get_character_before(position pos) const;

private:
    static std::string empty_text;
};

} // namespace hlasm_plugin::parser_library::lsp

#endif
