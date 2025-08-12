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

#ifndef HLASMPLUGIN_PARSERLIBRARY_EXPRESSIONS_NOMINAL_VALUE_H
#define HLASMPLUGIN_PARSERLIBRARY_EXPRESSIONS_NOMINAL_VALUE_H

#include "mach_expression.h"

namespace hlasm_plugin::parser_library::expressions {
struct nominal_value_string;
struct nominal_value_exprs;

// Class representing nominal value of data definition as it was written int the source
// code. Can be list of expressions and addresses(address has the form of D(B), where
// D and B are expressions) or string.
struct nominal_value_t : public context::dependable
{
    explicit nominal_value_t(range rng) noexcept
        : value_range(rng)
    {}
    range value_range;

    nominal_value_string* access_string();
    nominal_value_exprs* access_exprs();

    const nominal_value_string* access_string() const;
    const nominal_value_exprs* access_exprs() const;

    virtual ~nominal_value_t() = default;

    virtual size_t hash() const = 0;

    friend bool is_similar(const nominal_value_t& l, const nominal_value_t& r);
};

using nominal_value_ptr = std::unique_ptr<nominal_value_t>;

struct nominal_value_string final : public nominal_value_t
{
    context::dependency_collector get_dependencies(context::dependency_solver& solver) const override;

    nominal_value_string(std::string value, range rng);
    std::string value;

    friend bool is_similar(const nominal_value_string& l, const nominal_value_string& r) { return l.value == r.value; }

    size_t hash() const override;
};

// Represents address in the form D(B)
struct address_nominal final : public context::dependable
{
    context::dependency_collector get_dependencies(context::dependency_solver& solver) const override;
    address_nominal(mach_expr_ptr displacement, mach_expr_ptr base, range r);
    mach_expr_ptr displacement;
    mach_expr_ptr base;
    range total;

    friend bool is_similar(const address_nominal& l, const address_nominal& r);

    size_t hash() const;
};

using expr_or_address = std::variant<mach_expr_ptr, address_nominal>;
using expr_or_address_list = std::vector<expr_or_address>;

struct nominal_value_exprs final : public nominal_value_t
{
    context::dependency_collector get_dependencies(context::dependency_solver& solver) const override;

    nominal_value_exprs(expr_or_address_list exprs, range rng);
    expr_or_address_list exprs;

    friend bool is_similar(const nominal_value_exprs& l, const nominal_value_exprs& r);

    size_t hash() const override;
};


} // namespace hlasm_plugin::parser_library::expressions
#endif
