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

#include <variant>

#include "macro.h"
#include "utils/time.h"
#include "variables/set_symbol.h"

namespace hlasm_plugin::parser_library::context {
class location_counter;
class set_symbol_base;

struct code_scope_variable
{
    set_symbol_base* ref = nullptr;

    std::variant<std::monostate, set_symbol<A_t>, set_symbol<B_t>, set_symbol<C_t>> data;

    bool global;

    code_scope_variable(set_symbol_base* ref, bool global)
        : ref(ref)
        , global(global)
    {}

    template<typename T>
    code_scope_variable(std::in_place_type_t<T>, id_index name, bool is_scalar, bool global)
        : data(std::in_place_type<set_symbol<T>>, name, is_scalar)
        , global(global)
    {
        ref = &std::get<set_symbol<T>>(data);
    }
};

// helper struct for HLASM code scopes
// contains locally valid set symbols, sequence symbols and pointer to macro class (if code is in any)
struct code_scope
{
    using set_sym_storage = std::unordered_map<id_index, code_scope_variable>;

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
