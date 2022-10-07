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
#include "variables/set_symbol.h"
#include "variables/system_variable.h"

namespace hlasm_plugin::parser_library::context {

// helper struct for HLASM code scopes
// contains locally valid set symbols, sequence symbols and pointer to macro class (if code is in any)
struct code_scope
{
    using set_sym_storage = std::unordered_map<id_index, set_sym_ptr>;
    using sys_sym_storage = std::unordered_map<id_index, sys_sym_ptr>;

    // local variables of scope
    set_sym_storage variables;
    // local system variables of scope
    sys_sym_storage system_variables;
    // local sequence symbols of scope
    label_storage sequence_symbols;
    // gets macro to which this scope belong (nullptr if in open code)
    macro_invo_ptr this_macro;
    // the ACTR branch counter
    A_t branch_counter = 4096;
    // number of changed branch counters
    size_t branch_counter_change = 0;
    // variable for implementing &SYSM_SEV
    unsigned mnote_max_in_scope = 0;

    bool is_in_macro() const { return !!this_macro; }

    code_scope(macro_invo_ptr macro_invo, macro_def_ptr macro_def)
        : this_macro(std::move(macro_invo))
        , this_macro_def_(std::move(macro_def))
    {}
    code_scope() = default;

private:
    macro_def_ptr this_macro_def_;
};

} // namespace hlasm_plugin::parser_library::context
#endif
