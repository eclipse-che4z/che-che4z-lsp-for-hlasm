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

#ifndef HLASMPLUGIN_PARSERLIBRARY_MACH_EXPRESSION_H
#define HLASMPLUGIN_PARSERLIBRARY_MACH_EXPRESSION_H

#include <memory>
#include <string>

#include "context/ordinary_assembly/dependable.h"
#include "diagnosable_impl.h"


namespace hlasm_plugin::parser_library::expressions {

class mach_expr_visitor;
class mach_expression;
using mach_expr_ptr = std::unique_ptr<mach_expression>;
using mach_expr_list = std::vector<mach_expr_ptr>;

// Represents in machine expression written in source code.
// This interface represents one node (operator or term) in
// a machine expression, the root operator represents the
// whole machine expression.
// It can be evaluated using context::dependency_solver& that provides
// information about values of ordinary symbols.
class mach_expression : public diagnosable_op_impl, public context::resolvable
{
    virtual bool do_is_similar(const mach_expression& expr) const = 0;

public:
    using value_t = context::symbol_value;

    context::symbol_value resolve(context::dependency_solver& solver) const override;

    virtual value_t evaluate(context::dependency_solver& info) const = 0;

    virtual const mach_expression* leftmost_term() const = 0;

    virtual void apply(mach_expr_visitor& visitor) const = 0;

    virtual size_t hash() const = 0;

    range get_range() const;
    virtual ~mach_expression() {}

    static mach_expr_ptr assign_expr(mach_expr_ptr expr, range expr_range);

    bool is_similar(const mach_expression& expr) const;

protected:
    mach_expression(range rng);

private:
    range expr_range_;
};

inline size_t hash_combine(std::size_t old, std::size_t next)
{
    return old ^ (next + 0x9e3779b9 + (old << 6) + (old >> 2));
}

} // namespace hlasm_plugin::parser_library::expressions

#endif
