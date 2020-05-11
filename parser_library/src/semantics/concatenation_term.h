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
