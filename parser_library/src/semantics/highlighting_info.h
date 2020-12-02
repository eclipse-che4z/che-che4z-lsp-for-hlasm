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

#ifndef HIGHLIGHTING_INFO
#define HIGHLIGHTING_INFO

#include <map>
#include <set>
#include <string>
#include <vector>

#include "protocol.h"

namespace hlasm_plugin {
namespace parser_library {
namespace semantics {

// representation of document along with its version as used in LSP
struct versioned_document
{
    versioned_document()
        : version(0) {};
    versioned_document(std::string uri)
        : uri(std::move(uri))
        , version(0)
    {}
    std::string uri;
    version_t version;
};

// vector of tokens
using lines_info = std::set<token_info>;

// representation about the changes in continuation
struct continuation_info
{
    // end of line position
    // might vary thanks to DBCS for each line
    std::vector<position> continuation_positions;
    // continue and continuation columns might very thanks to ICTL
    size_t continue_column = 15;
    size_t continuation_column = 71;
};

// representation of the information related to the server-side highlighting
struct highlighting_info
{
    highlighting_info() {};
    highlighting_info(std::string document_uri)
        : document(std::move(document_uri))
    {}

    highlighting_info(versioned_document document, lines_info lines)
        : document(std::move(document))
        , lines(std::move(lines)) {};
    // document that the information concerns
    versioned_document document;
    // tokens in each line
    lines_info lines;
    // continuation information is needed for highlighting as well
    continuation_info cont_info;
};

} // namespace semantics
} // namespace parser_library
} // namespace hlasm_plugin

#endif
