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

#include "nominal_value.h"

#include <unordered_set>

using namespace hlasm_plugin::parser_library::expressions;
using namespace hlasm_plugin::parser_library::context;

nominal_value_string* nominal_value_t::access_string() { return dynamic_cast<nominal_value_string*>(this); }

nominal_value_exprs* nominal_value_t::access_exprs() { return dynamic_cast<nominal_value_exprs*>(this); }

//*********** nominal_value_string ***************
dependency_collector nominal_value_string::get_dependencies(dependency_solver&) const { return dependency_collector(); }

nominal_value_string::nominal_value_string(std::string value, hlasm_plugin::parser_library::range rng)
    : value(std::move(value))
    , value_range(rng)
{ }

//*********** nominal_value_exprs ***************
dependency_collector nominal_value_exprs::get_dependencies(dependency_solver& solver) const
{
    dependency_collector conjunction;
    for (auto& e : exprs)
    {
        dependency_collector list;
        if (std::holds_alternative<mach_expr_ptr>(e))
            list = std::get<mach_expr_ptr>(e)->get_dependencies(solver);
        else if (std::holds_alternative<address_nominal>(e))
            list = std::get<address_nominal>(e).get_dependencies(solver);

        conjunction.undefined_symbols.insert(list.undefined_symbols.begin(), list.undefined_symbols.end());
    }
    return conjunction;
}

nominal_value_exprs::nominal_value_exprs(expr_or_address_list exprs)
    : exprs(std::move(exprs))
{ }



//*********** nominal_value_list ***************
dependency_collector address_nominal::get_dependencies(dependency_solver& solver) const
{
    auto conjunction = base->get_dependencies(solver);

    auto list2 = displacement->get_dependencies(solver);
    conjunction.undefined_symbols.insert(list2.undefined_symbols.begin(), list2.undefined_symbols.end());

    return conjunction;
}

address_nominal::address_nominal()
    : displacement()
    , base()
{ }

address_nominal::address_nominal(mach_expr_ptr displacement, mach_expr_ptr base)
    : displacement(std::move(displacement))
    , base(std::move(base))
{ }
