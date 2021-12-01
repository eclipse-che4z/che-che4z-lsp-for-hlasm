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
#include "expressions/data_definition.h"
#include "mach_expression.h"

namespace hlasm_plugin::parser_library::expressions {

// Represents a number written in a machine expression.
class mach_expr_constant final : public mach_expression
{
    value_t value_;

    bool do_is_similar(const mach_expression& expr) const override;

public:
    mach_expr_constant(std::string value_text, range rng);
    mach_expr_constant(int value, range rng);

    context::dependency_collector get_dependencies(context::dependency_solver& solver) const override;

    value_t evaluate(context::dependency_solver& info) const override;

    const mach_expression* leftmost_term() const override;

    void apply(mach_expr_visitor& visitor) const override;

    void collect_diags() const override {}

    size_t hash() const override;
};

// Represents an literal expression (e.g. =C'text')
class mach_expr_literal final : public mach_expression
{
    bool do_is_similar(const mach_expression& expr) const override;

    struct literal_data
    {
        const data_definition dd;
        bool referenced_by_reladdr = false;

        explicit literal_data(data_definition dd)
            : dd(std::move(dd))
        {}
    };

    std::shared_ptr<literal_data> m_literal_data;
    std::string m_dd_text;

public:
    mach_expr_literal(range rng, data_definition dd, std::string text);

    context::dependency_collector get_dependencies(context::dependency_solver& solver) const override;

    value_t evaluate(context::dependency_solver& info) const override;

    const mach_expression* leftmost_term() const override;

    void apply(mach_expr_visitor& visitor) const override;

    void collect_diags() const override;

    size_t hash() const override;

    const data_definition& get_data_definition() const;

    const std::string& get_data_definition_text() const { return m_dd_text; }

    context::id_index get_literal_id(context::dependency_solver& solver) const;

    void referenced_by_reladdr() const { m_literal_data->referenced_by_reladdr = true; }
};

// Represents an attribute of a symbol written in machine expressions (e.g. L'SYMBOL)
class mach_expr_data_attr final : public mach_expression
{
    bool do_is_similar(const mach_expression& expr) const override;

public:
    mach_expr_data_attr(context::id_index value, context::data_attr_kind attribute, range whole_rng, range symbol_rng);
    mach_expr_data_attr(
        std::unique_ptr<mach_expr_literal> value, context::data_attr_kind attribute, range whole_rng, range symbol_rng);

    context::id_index value;
    context::data_attr_kind attribute;
    range symbol_range;
    std::unique_ptr<mach_expr_literal> lit;

    context::dependency_collector get_dependencies(context::dependency_solver& solver) const override;

    value_t evaluate(context::dependency_solver& info) const override;

    const mach_expression* leftmost_term() const override;

    void apply(mach_expr_visitor& visitor) const override;

    void collect_diags() const override;

    size_t hash() const override;
};

// Represents an ordinary symbol in machine expressions.
class mach_expr_symbol final : public mach_expression
{
    bool do_is_similar(const mach_expression& expr) const override;

public:
    mach_expr_symbol(context::id_index value, range rng);

    context::id_index value;
    mach_expr_data_attr len_expr;

    context::dependency_collector get_dependencies(context::dependency_solver& solver) const override;

    value_t evaluate(context::dependency_solver& info) const override;

    const mach_expression* leftmost_term() const override;

    void apply(mach_expr_visitor& visitor) const override;

    void collect_diags() const override {}

    size_t hash() const override;
};

// Represents a location counter written in a machine expression (the character *)
class mach_expr_location_counter final : public mach_expression
{
    bool do_is_similar(const mach_expression& expr) const override;

public:
    mach_expr_location_counter(range rng);

    context::dependency_collector get_dependencies(context::dependency_solver& solver) const override;

    value_t evaluate(context::dependency_solver& info) const override;

    const mach_expression* leftmost_term() const override;

    void apply(mach_expr_visitor& visitor) const override;

    void collect_diags() const override {}

    size_t hash() const override;
};

// Represents a self defining term (e.g. X'4A')
class mach_expr_self_def final : public mach_expression
{
    bool do_is_similar(const mach_expression& expr) const override;

    value_t value_;

public:
    mach_expr_self_def(std::string option, std::string value, range rng);

    context::dependency_collector get_dependencies(context::dependency_solver& solver) const override;

    value_t evaluate(context::dependency_solver& info) const override;

    const mach_expression* leftmost_term() const override;

    void apply(mach_expr_visitor& visitor) const override;

    void collect_diags() const override {}

    size_t hash() const override;
};

// Represents an "empty" term that is used when parsing of a machine expression fails
//(the user writes invalid expression)
class mach_expr_default final : public mach_expression
{
    bool do_is_similar(const mach_expression& expr) const override;

public:
    mach_expr_default(range rng);

    context::dependency_collector get_dependencies(context::dependency_solver& solver) const override;

    value_t evaluate(context::dependency_solver& info) const override;

    const mach_expression* leftmost_term() const override;

    void apply(mach_expr_visitor& visitor) const override;

    void collect_diags() const override;

    size_t hash() const override;
};

} // namespace hlasm_plugin::parser_library::expressions

#endif