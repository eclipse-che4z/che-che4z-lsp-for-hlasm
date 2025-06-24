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

#include "context/id_index.h"
#include "mach_expression.h"
#include "semantics/statement_fields.h"

namespace hlasm_plugin::parser_library::expressions {

// Represents a number written in a machine expression.
class mach_expr_constant final : public mach_expression
{
    int32_t value_;

    bool do_is_similar(const mach_expression& expr) const override;

public:
    mach_expr_constant(int32_t value, range rng);

    context::dependency_collector get_dependencies(context::dependency_solver& solver) const override;

    value_t evaluate(context::dependency_solver& info, diagnostic_op_consumer& diags) const override;

    const mach_expression* leftmost_term() const override;

    void apply(mach_expr_visitor& visitor) const override;

    size_t hash() const override;

    mach_expr_ptr clone() const override;
};

// Represents a literal expression (e.g. =C'text')
class mach_expr_literal final : public mach_expression
{
    bool do_is_similar(const mach_expression& expr) const override;

    semantics::literal_si m_literal_data;

public:
    mach_expr_literal(range rng, semantics::literal_si dd);

    context::dependency_collector get_dependencies(context::dependency_solver& solver) const override;

    value_t evaluate(context::dependency_solver& info, diagnostic_op_consumer& diags) const override;

    const mach_expression* leftmost_term() const override;

    void apply(mach_expr_visitor& visitor) const override;

    size_t hash() const override;

    mach_expr_ptr clone() const override;

    const data_definition& get_data_definition() const;

    context::id_index get_literal_id(context::dependency_solver& solver) const;

    void referenced_by_reladdr() const { m_literal_data->set_referenced_by_reladdr(); }
};

// Represents an attribute of a symbol written in machine expressions (e.g. L'SYMBOL)
class mach_expr_data_attr final : public mach_expression
{
    bool do_is_similar(const mach_expression& expr) const override;

public:
    mach_expr_data_attr(context::id_index value,
        context::id_index qualifier,
        context::data_attr_kind attribute,
        range whole_rng,
        range symbol_rng);

    context::id_index value;
    context::id_index qualifier;
    context::data_attr_kind attribute;
    range symbol_range;

    context::dependency_collector get_dependencies(context::dependency_solver& solver) const override;

    value_t evaluate(context::dependency_solver& info, diagnostic_op_consumer& diags) const override;

    const mach_expression* leftmost_term() const override;

    void apply(mach_expr_visitor& visitor) const override;

    size_t hash() const override;

    mach_expr_ptr clone() const override;
};

// Represents an attribute of a symbol written in machine expressions (e.g. L'SYMBOL)
class mach_expr_data_attr_literal final : public mach_expression
{
    bool do_is_similar(const mach_expression& expr) const override;

public:
    mach_expr_data_attr_literal(
        std::unique_ptr<mach_expr_literal> value, context::data_attr_kind attribute, range whole_rng, range symbol_rng);

    context::data_attr_kind attribute;
    range symbol_range;
    std::unique_ptr<mach_expr_literal> lit;

    context::dependency_collector get_dependencies(context::dependency_solver& solver) const override;

    value_t evaluate(context::dependency_solver& info, diagnostic_op_consumer& diags) const override;

    const mach_expression* leftmost_term() const override;

    void apply(mach_expr_visitor& visitor) const override;

    size_t hash() const override;

    mach_expr_ptr clone() const override;
};

// Represents an ordinary symbol in machine expressions.
class mach_expr_symbol final : public mach_expression
{
    bool do_is_similar(const mach_expression& expr) const override;

public:
    mach_expr_symbol(context::id_index value, context::id_index qualifier, range rng);

    context::id_index value;
    context::id_index qualifier;

    context::dependency_collector get_dependencies(context::dependency_solver& solver) const override;

    value_t evaluate(context::dependency_solver& info, diagnostic_op_consumer& diags) const override;

    const mach_expression* leftmost_term() const override;

    void apply(mach_expr_visitor& visitor) const override;

    size_t hash() const override;

    mach_expr_ptr clone() const override;
};

// Represents a location counter written in a machine expression (the character *)
class mach_expr_location_counter final : public mach_expression
{
    bool do_is_similar(const mach_expression& expr) const override;

public:
    mach_expr_location_counter(range rng);

    context::dependency_collector get_dependencies(context::dependency_solver& solver) const override;

    value_t evaluate(context::dependency_solver& info, diagnostic_op_consumer& diags) const override;

    const mach_expression* leftmost_term() const override;

    void apply(mach_expr_visitor& visitor) const override;

    size_t hash() const override;

    mach_expr_ptr clone() const override;
};

// Represents an "empty" term that is used when parsing of a machine expression fails
//(the user writes invalid expression)
class mach_expr_default final : public mach_expression
{
    bool do_is_similar(const mach_expression& expr) const override;

public:
    mach_expr_default(range rng);

    context::dependency_collector get_dependencies(context::dependency_solver& solver) const override;

    value_t evaluate(context::dependency_solver& info, diagnostic_op_consumer& diags) const override;

    const mach_expression* leftmost_term() const override;

    void apply(mach_expr_visitor& visitor) const override;

    size_t hash() const override;

    mach_expr_ptr clone() const override;
};

} // namespace hlasm_plugin::parser_library::expressions

#endif
