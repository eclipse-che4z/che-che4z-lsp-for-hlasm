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

#include <iterator>
#include <memory>
#include <string>
#include <vector>

#include "antlr4-runtime.h"

#include "context/id_storage.h"
#include "range.h"

// this file is a composition of structures that create concat_chain
// concat_chain is used to represent model statement fields

namespace hlasm_plugin {
namespace parser_library {
namespace semantics {

enum class concat_type
{
    STR,
    VAR,
    DOT,
    SUB,
    EQU
};

struct char_str;
struct var_sym;
struct dot;
struct equals;
struct sublist;

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

    static var_sym* contains_var_sym(const concat_chain& chain);

    static concat_chain clone(const concat_chain& chain);

    const concat_type type;

    concatenation_point(const concat_type type);

    char_str* access_str();
    var_sym* access_var();
    dot* access_dot();
    equals* access_equ();
    sublist* access_sub();

    virtual ~concatenation_point() = default;
};

// concatenation point representing character string
struct char_str : public concatenation_point
{
    char_str(std::string value);

    std::string value;
};

struct basic_var_sym;
struct created_var_sym;

struct var_sym : public concatenation_point
{
    const bool created;

    std::vector<antlr4::ParserRuleContext*> subscript;

    const range symbol_range;

    basic_var_sym* access_basic();
    const basic_var_sym* access_basic() const;
    created_var_sym* access_created();
    const created_var_sym* access_created() const;

protected:
    var_sym(const bool created, std::vector<antlr4::ParserRuleContext*> subscript, const range symbol_range);
};

using vs_ptr = std::unique_ptr<var_sym>;

// concatenation point representing variable symbol
struct basic_var_sym : public var_sym
{
    basic_var_sym(context::id_index name, std::vector<antlr4::ParserRuleContext*> subscript, range symbol_range);

    const context::id_index name;
};

// concatenation point representing created variable symbol
struct created_var_sym : public var_sym
{
    created_var_sym(concat_chain created_name, std::vector<antlr4::ParserRuleContext*> subscript, range symbol_range);

    const concat_chain created_name;
};

// concatenation point representing dot
struct dot : public concatenation_point
{
    dot();
};

// concatenation point representing equals sign
struct equals : public concatenation_point
{
    equals();
};

// concatenation point representing macro operand sublist
struct sublist : public concatenation_point
{
    sublist(std::vector<concat_chain> list);

    std::vector<concat_chain> list;
};

} // namespace semantics
} // namespace parser_library
} // namespace hlasm_plugin
#endif
