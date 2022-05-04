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

#ifndef SEMANTICS_CONCATENATION_H
#define SEMANTICS_CONCATENATION_H

#include <memory>
#include <set>
#include <string>
#include <vector>

#include "context/common_types.h"
#include "context/id_storage.h"
#include "diagnostic_consumer.h"

// this file is a composition of structures that create concat_chain
// concat_chain is used to represent model statement fields

namespace hlasm_plugin::parser_library::expressions {
struct evaluation_context;
}

namespace hlasm_plugin::parser_library::semantics {

enum class concat_type
{
    STR,
    VAR,
    DOT,
    SUB,
    EQU
};

struct char_str_conc;
struct var_sym_conc;
struct dot_conc;
struct equals_conc;
struct sublist_conc;

struct concatenation_point;
using concat_point_ptr = std::unique_ptr<concatenation_point>;
using concat_chain = std::vector<concat_point_ptr>;

// helper stuct for character strings that contain variable symbols
// these points of concatenation when formed into array represent character string in a way that is easily concatenated
// when variable symbols are substituted
struct concatenation_point
{
    // cleans concat_chains of empty strings and badly parsed operands
    static void clear_concat_chain(concat_chain& conc_list);

    static std::string to_string(const concat_chain& chain);
    static std::string to_string(concat_chain::const_iterator begin, concat_chain::const_iterator end);

    static var_sym_conc* contains_var_sym(concat_chain::const_iterator begin, concat_chain::const_iterator end);

    static std::set<context::id_index> get_undefined_attributed_symbols(
        const concat_chain& chain, const expressions::evaluation_context& eval_ctx);

    const concat_type type;

    concatenation_point(const concat_type type);

    char_str_conc* access_str();
    var_sym_conc* access_var();
    dot_conc* access_dot();
    equals_conc* access_equ();
    sublist_conc* access_sub();

    static std::string evaluate(const concat_chain& chain, const expressions::evaluation_context& eval_ctx);
    static std::string evaluate(concat_chain::const_iterator begin,
        concat_chain::const_iterator end,
        const expressions::evaluation_context& eval_ctx);

    virtual std::string evaluate(const expressions::evaluation_context& eval_ctx) const = 0;
    virtual void resolve(diagnostic_op_consumer& diag) = 0;

    virtual ~concatenation_point() = default;
};

} // namespace hlasm_plugin::parser_library::semantics

#endif
