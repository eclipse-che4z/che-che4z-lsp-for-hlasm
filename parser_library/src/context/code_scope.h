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

#ifndef CONTEXT_CODE_SCOPE_H
#define CONTEXT_CODE_SCOPE_H

#include "macro.h"
#include "utils/time.h"

namespace hlasm_plugin::parser_library::context {
class location_counter;
class set_symbol_base;

// helper struct for HLASM code scopes
// contains locally valid set symbols, sequence symbols and pointer to macro class (if code is in any)
struct code_scope
{
    using set_sym_storage = std::unordered_map<id_index, std::pair<std::shared_ptr<set_symbol_base>, bool>>;

    // local variables of scope
    set_sym_storage variables;
    // gets macro to which this scope belong (nullptr if in open code)
    std::unique_ptr<macro_invocation> this_macro;
    // the ACTR branch counter
    A_t branch_counter = 4096;
    // number of changed branch counters
    size_t branch_counter_change = 0;
    // variable for implementing &SYSM_SEV
    unsigned mnote_max_in_scope = 0;
    // scope creation time
    utils::timestamp time;
    // initial location counter
    location_counter* loctr = nullptr;
    // scope unique value
    unsigned long sysndx = 0;

    bool is_in_macro() const { return !!this_macro; }

    explicit code_scope(std::unique_ptr<macro_invocation> macro_invo)
        : this_macro(std::move(macro_invo))
    {}
    explicit code_scope() = default;
};

} // namespace hlasm_plugin::parser_library::context
#endif
