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

#include "utils/general_hashers.h"
#include "utils/merge_sorted.h"
#include "utils/similar.h"

using namespace hlasm_plugin::parser_library::expressions;
using namespace hlasm_plugin::parser_library::context;

using hlasm_plugin::utils::hashers::hash_combine;

nominal_value_string* nominal_value_t::access_string() { return dynamic_cast<nominal_value_string*>(this); }

nominal_value_exprs* nominal_value_t::access_exprs() { return dynamic_cast<nominal_value_exprs*>(this); }

const nominal_value_string* nominal_value_t::access_string() const
{
    return dynamic_cast<const nominal_value_string*>(this);
}

const nominal_value_exprs* nominal_value_t::access_exprs() const
{
    return dynamic_cast<const nominal_value_exprs*>(this);
}

//*********** nominal_value_string ***************
dependency_collector nominal_value_string::get_dependencies(dependency_solver&) const { return dependency_collector(); }

nominal_value_string::nominal_value_string(std::string value, range rng)
    : nominal_value_t(rng)
    , value(std::move(value))
{}

size_t hlasm_plugin::parser_library::expressions::nominal_value_string::hash() const
{
    return hash_combine((size_t)0xfc655abdb5880969, std::hash<std::string> {}(value));
}

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

        utils::merge_sorted(
            conjunction.undefined_symbolics,
            list.undefined_symbolics,
            [](const auto& l, const auto& r) { return l.name <=> r.name; },
            [](auto& l, const auto& r) { l.flags |= r.flags; });
    }
    return conjunction;
}

nominal_value_exprs::nominal_value_exprs(expr_or_address_list exprs, range rng)
    : nominal_value_t(rng)
    , exprs(std::move(exprs))
{}

size_t nominal_value_exprs::hash() const
{
    auto result = (size_t)0x0c260c0f86f63b4e;
    for (const auto& e : exprs)
    {
        result = hash_combine(result,
            std::visit(
                []<typename V>(const V& v) {
                    if constexpr (std::is_same_v<V, mach_expr_ptr>)
                        return v->hash();
                    else
                        return v.hash();
                },
                e));
    }
    return result;
}



//*********** nominal_value_list ***************
dependency_collector address_nominal::get_dependencies(dependency_solver& solver) const
{
    auto conjunction = base->get_dependencies(solver);

    auto list2 = displacement->get_dependencies(solver);

    utils::merge_sorted(
        conjunction.undefined_symbolics,
        list2.undefined_symbolics,
        [](const auto& l, const auto& r) { return l.name <=> r.name; },
        [](auto& l, const auto& r) { l.flags |= r.flags; });

    return conjunction;
}

address_nominal::address_nominal(mach_expr_ptr displacement, mach_expr_ptr base, range r)
    : displacement(std::move(displacement))
    , base(std::move(base))
    , total(r)
{}

size_t address_nominal::hash() const
{
    auto result = (size_t)0x2aa82a8e5d77ef6c;

    if (displacement)
        result = hash_combine(result, displacement->hash());
    if (base)
        result = hash_combine(result, base->hash());

    return result;
}

namespace hlasm_plugin::parser_library::expressions {

bool is_similar(const nominal_value_t& l, const nominal_value_t& r)
{
    return utils::is_similar(l.access_string(), r.access_string())
        && utils::is_similar(l.access_exprs(), r.access_exprs());
}

bool is_similar(const address_nominal& l, const address_nominal& r)
{
    return utils::is_similar(l, r, &address_nominal::displacement, &address_nominal::base);
}

bool is_similar(const nominal_value_exprs& left, const nominal_value_exprs& right)
{
    if (left.exprs.size() != right.exprs.size())
        return false;

    for (size_t i = 0; i < left.exprs.size(); ++i)
    {
        if (!std::visit(
                []<typename L, typename R>(const L& l, const R& r) {
                    if constexpr (std::is_same_v<L, R>)
                        return utils::is_similar(l, r);
                    else
                        return false;
                },
                left.exprs[i],
                right.exprs[i]))
            return false;
    }
    return true;
}

} // namespace hlasm_plugin::parser_library::expressions
