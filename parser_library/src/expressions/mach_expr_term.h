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

#ifndef HLASMPLUGIN_PARSERLIBRARY_EXPRESSIONS_MACH_EXPR_TERM_H
#define HLASMPLUGIN_PARSERLIBRARY_EXPRESSIONS_MACH_EXPR_TERM_H

#include "context/id_storage.h"
#include "mach_expression.h"

namespace hlasm_plugin::parser_library::expressions {

// Represents a number written in a machine expression.
class mach_expr_constant : public mach_expression
{
    value_t value_;

public:
    mach_expr_constant(std::string value_text, range rng);
    mach_expr_constant(int value, range rng);

    context::dependency_collector get_dependencies(context::dependency_solver& solver) const override;

    value_t evaluate(mach_evaluate_info info) const override;

    void fill_location_counter(context::address addr) override;

    const mach_expression* leftmost_term() const override;

    void apply(mach_expr_visitor& visitor) const override;

    void collect_diags() const override {}
};

// Represents an attribute of a symbol written in machine expressions (e.g. L'SYMBOL)
class mach_expr_data_attr : public mach_expression
{
public:
    mach_expr_data_attr(context::id_index value, context::data_attr_kind attribute, range whole_rng, range symbol_rng);

    context::id_index value;
    context::data_attr_kind attribute;
    range symbol_range;

    context::dependency_collector get_dependencies(context::dependency_solver& solver) const override;

    value_t evaluate(mach_evaluate_info info) const override;

    void fill_location_counter(context::address addr) override;

    const mach_expression* leftmost_term() const override;

    void apply(mach_expr_visitor& visitor) const override;

    void collect_diags() const override {}
};

// Represents an ordinary symbol in machine expressions.
class mach_expr_symbol : public mach_expression
{
public:
    mach_expr_symbol(context::id_index value, range rng);

    context::id_index value;
    mach_expr_data_attr len_expr;

    context::dependency_collector get_dependencies(context::dependency_solver& solver) const override;

    value_t evaluate(mach_evaluate_info info) const override;

    void fill_location_counter(context::address addr) override;

    const mach_expression* leftmost_term() const override;

    void apply(mach_expr_visitor& visitor) const override;

    void collect_diags() const override {}
};

// Represents a location counter written in a machine expression (the character *)
class mach_expr_location_counter : public mach_expression
{
public:
    std::optional<context::address> location_counter;

    mach_expr_location_counter(range rng);

    context::dependency_collector get_dependencies(context::dependency_solver& solver) const override;

    value_t evaluate(mach_evaluate_info info) const override;

    void fill_location_counter(context::address addr) override;

    const mach_expression* leftmost_term() const override;

    void apply(mach_expr_visitor& visitor) const override;

    void collect_diags() const override {}
};

// Represents a self defining term (e.g. X'4A')
class mach_expr_self_def : public mach_expression
{
    value_t value_;

public:
    mach_expr_self_def(std::string option, std::string value, range rng);

    context::dependency_collector get_dependencies(context::dependency_solver& solver) const override;

    value_t evaluate(mach_evaluate_info info) const override;

    void fill_location_counter(context::address addr) override;

    const mach_expression* leftmost_term() const override;

    void apply(mach_expr_visitor& visitor) const override;

    void collect_diags() const override {}
};

// Represents an "empty" term that is used when parsing of a machine expression fails
//(the user writes invalid expression)
class mach_expr_default : public mach_expression
{
public:
    mach_expr_default(range rng);

    context::dependency_collector get_dependencies(context::dependency_solver& solver) const override;

    value_t evaluate(mach_evaluate_info info) const override;

    void fill_location_counter(context::address addr) override;

    const mach_expression* leftmost_term() const override;

    void apply(mach_expr_visitor& visitor) const override;

    void collect_diags() const override;
};

} // namespace hlasm_plugin::parser_library::expressions

#endif