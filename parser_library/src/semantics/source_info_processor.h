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

#ifndef SOURCE_INFO_PROC_INFO
#define SOURCE_INFO_PROC_INFO

#include <memory>
#include <vector>

#include "highlighting_info.h"

namespace hlasm_plugin::parser_library::semantics {

// Stores info that are generated during parsing and do not require access to context (currently only highligting)
class source_info_processor
{
public:
    explicit source_info_processor(bool collect_hl_info);

    // takes vector of highlighting symbols and processes them into highlighting info for further propagation
    void process_hl_symbols(std::vector<token_info> symbols);

    // add one hl symbol to the highlighting info
    void add_hl_symbol(token_info symbol);

    const lines_info& semantic_tokens() const;

    // finishes collected data
    void finish();

private:
    // highlighting information
    semantics::highlighting_info hl_info_;
    // specifies whether to generate highlighting information
    bool collect_hl_info_;
};

} // namespace hlasm_plugin::parser_library::semantics

#endif
