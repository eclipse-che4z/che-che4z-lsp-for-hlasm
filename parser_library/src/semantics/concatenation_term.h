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

#ifndef SEMANTICS_CONCATENATION_TERM_H
#define SEMANTICS_CONCATENATION_TERM_H

#include "concatenation.h"
#include "variable_symbol.h"

// this file is a composition of structures that create concat_chain
// concat_chain is used to represent model statement fields

namespace hlasm_plugin {
namespace parser_library {
namespace semantics {

// concatenation point representing character string
struct char_str_conc : public concatenation_point
{
    char_str_conc(std::string value);

    std::string value;

    virtual std::string evaluate(expressions::evaluation_context& eval_ctx) const override;
};

// concatenation point representing variable symbol
struct var_sym_conc : public concatenation_point
{
    var_sym_conc(vs_ptr);

    vs_ptr symbol;

    virtual std::string evaluate(expressions::evaluation_context& eval_ctx) const override;

    static std::string evaluate(context::SET_t varsym_value);
};

// concatenation point representing dot
struct dot_conc : public concatenation_point
{
    dot_conc();

    virtual std::string evaluate(expressions::evaluation_context& eval_ctx) const override;
};

// concatenation point representing equals sign
struct equals_conc : public concatenation_point
{
    equals_conc();

    virtual std::string evaluate(expressions::evaluation_context& eval_ctx) const override;
};

// concatenation point representing macro operand sublist
struct sublist_conc : public concatenation_point
{
    sublist_conc(std::vector<concat_chain> list);

    std::vector<concat_chain> list;

    virtual std::string evaluate(expressions::evaluation_context& eval_ctx) const override;
};

} // namespace semantics
} // namespace parser_library
} // namespace hlasm_plugin
#endif
