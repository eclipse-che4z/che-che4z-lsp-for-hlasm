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

#ifndef LSP_INFO_PROC_INFO
#define LSP_INFO_PROC_INFO

#include <memory>
#include <regex>
#include <vector>

#include "context/hlasm_context.h"
#include "highlighting_info.h"

namespace hlasm_plugin {
namespace parser_library {
namespace semantics {

// lsp info processor processes lsp symbols from parser into symbol definitions and their occurencies used for responses
// to lsp requests
class lsp_info_processor
{
public:
    lsp_info_processor(std::string file, const std::string& text, context::hlasm_context* ctx, bool collect_hl_info);

    // name of file this processor is currently used
    const std::string* file_name;
    // common value of an empty string
    const std::string* empty_string;

    // takes vector of highlighting symbols and processes them into highlighting info for further propagation
    void process_hl_symbols(std::vector<token_info> symbols);

    const lines_info& semantic_tokens() const;

    // add one hl symbol to the highlighting info
    void add_hl_symbol(token_info symbol);

    // finishes collected data
    void finish();

private:
    // highlighting information
    semantics::highlighting_info hl_info_;
    // specifies whether to generate highlighting information
    bool collect_hl_info_;
};
} // namespace semantics
} // namespace parser_library
} // namespace hlasm_plugin
#endif