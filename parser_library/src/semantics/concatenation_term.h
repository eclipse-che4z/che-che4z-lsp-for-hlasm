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

#include <iterator>
#include <memory>
#include <string>
#include <vector>

#include "antlr4-runtime.h"

#include "context/id_storage.h"
#include "range.h"
#include "concatenation.h"

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
};

struct basic_var_sym_conc;
struct created_var_sym_conc;

struct var_sym_conc : public concatenation_point
{
    const bool created;

    std::vector<antlr4::ParserRuleContext*> subscript;

    const range symbol_range;

    basic_var_sym_conc* access_basic();
    const basic_var_sym_conc* access_basic() const;
    created_var_sym_conc* access_created();
    const created_var_sym_conc* access_created() const;

protected:
    var_sym_conc(const bool created, std::vector<antlr4::ParserRuleContext*> subscript, const range symbol_range);
};

using vs_ptr = std::unique_ptr<var_sym_conc>;

// concatenation point representing variable symbol
struct basic_var_sym_conc : public var_sym_conc
{
    basic_var_sym_conc(context::id_index name, std::vector<antlr4::ParserRuleContext*> subscript, range symbol_range);

    const context::id_index name;
};

// concatenation point representing created variable symbol
struct created_var_sym_conc : public var_sym_conc
{
    created_var_sym_conc(concat_chain created_name, std::vector<antlr4::ParserRuleContext*> subscript, range symbol_range);

    const concat_chain created_name;
};

// concatenation point representing dot
struct dot_conc : public concatenation_point
{
    dot_conc();
};

// concatenation point representing equals sign
struct equals_conc : public concatenation_point
{
    equals_conc();
};

// concatenation point representing macro operand sublist
struct sublist_conc : public concatenation_point
{
    sublist_conc(std::vector<concat_chain> list);

    std::vector<concat_chain> list;
};

} // namespace semantics
} // namespace parser_library
} // namespace hlasm_plugin
#endif
