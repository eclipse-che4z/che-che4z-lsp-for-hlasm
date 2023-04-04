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

#ifndef HLASMPLUGIN_PARSERLIBRARY_FILE_H
#define HLASMPLUGIN_PARSERLIBRARY_FILE_H

#include <string>
#include <string_view>
#include <vector>

#include "protocol.h"
#include "utils/resource_location.h"

namespace hlasm_plugin::parser_library::workspaces {

// Interface that represents both file opened in LSP
// as well as a file opened by parser library from the disk.
class file
{
public:
    virtual const utils::resource::resource_location& get_location() const = 0;
    // Gets contents of file either by loading from disk or from LSP.
    virtual const std::string& get_text() const = 0;
    // Returns whether file is open by LSP.
    virtual bool get_lsp_editing() const = 0;
    // Internal unique version
    virtual version_t get_version() const = 0;
    // file is in error state
    virtual bool error() const = 0;
    // Tests if the file is up-to-date
    virtual bool up_to_date() const = 0;

protected:
    ~file() = default;
};

// append offsets of newlines to output
void find_newlines(std::vector<size_t>& output, std::string_view text);
// Generates vector of offsets to the beginning of individual lines
std::vector<size_t> create_line_indices(std::string_view text);
void create_line_indices(std::vector<size_t>& output, std::string_view text);
// Returns the location in text that corresponds to utf-16 based location
// The position may point beyond the last character -> returns text.size()
size_t index_from_position(std::string_view text, const std::vector<size_t>& line_indices, position pos);
// apply incremental change
void apply_text_diff(std::string& text, std::vector<size_t>& lines, range r, std::string_view replacement);


} // namespace hlasm_plugin::parser_library::workspaces

#endif
