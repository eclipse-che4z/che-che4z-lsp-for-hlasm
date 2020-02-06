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

namespace hlasm_plugin::parser_library::expressions
{
struct nominal_value_string;
struct nominal_value_exprs;

//Class representing nominal value of data definition. Can be list of expressions
//and addresses(address has the form of D(B), where D and B are expressions) or string.
struct nominal_value_t : public context::dependable
{
	nominal_value_string* access_string();
	nominal_value_exprs* access_exprs();

	virtual ~nominal_value_t() = default;
};

using nominal_value_ptr = std::unique_ptr<nominal_value_t>;

struct nominal_value_string final : public nominal_value_t
{
	virtual context::dependency_collector get_dependencies(context::dependency_solver& solver) const override;

	nominal_value_string(std::string value, range rng);
	std::string value;
	range value_range;
};

struct address_nominal : public context::dependable
{
	virtual context::dependency_collector get_dependencies(context::dependency_solver& solver) const override;
	address_nominal();
	address_nominal(mach_expr_ptr displacement, mach_expr_ptr base);
	mach_expr_ptr displacement;
	mach_expr_ptr base;
};

using expr_or_address = std::variant<mach_expr_ptr, address_nominal>;
using expr_or_address_list = std::vector<expr_or_address>;

struct nominal_value_exprs final : public nominal_value_t
{
	virtual context::dependency_collector get_dependencies(context::dependency_solver& solver) const override;

	nominal_value_exprs(expr_or_address_list exprs);
	expr_or_address_list exprs;
};


}
#endif