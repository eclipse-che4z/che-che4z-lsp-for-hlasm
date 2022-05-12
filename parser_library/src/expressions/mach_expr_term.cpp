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

#include "mach_expr_term.h"

#include <charconv>
#include <stdexcept>

#include "checking/checker_helper.h"
#include "conditional_assembly/terms/ca_constant.h"
#include "ebcdic_encoding.h"
#include "mach_expr_visitor.h"
#include "utils/similar.h"

namespace hlasm_plugin::parser_library::expressions {
//***********  mach_expr_constant ************

bool mach_expr_constant::do_is_similar(const mach_expression& expr) const
{
    const auto& e = static_cast<const mach_expr_constant&>(expr);
    if (value_.value_kind() != e.value_.value_kind())
        return false;
    if (value_.value_kind() == context::symbol_value_kind::ABS)
        return value_.get_abs() == e.value_.get_abs();

    return true;
}

mach_expr_constant::mach_expr_constant(std::string value_text, range rng)
    : mach_expression(rng)
{
    int32_t v = 0;

    const auto* b = value_text.data();
    const auto* e = b + value_text.size();

    if (auto ec = std::from_chars(b, e, v); ec.ec == std::errc {} && ec.ptr == e)
        value_ = value_t(v);
}
mach_expr_constant::mach_expr_constant(int value, range rng)
    : mach_expression(rng)
    , value_(value)
{}

context::dependency_collector mach_expr_constant::get_dependencies(context::dependency_solver&) const
{
    return context::dependency_collector();
}

mach_expr_constant::value_t mach_expr_constant::evaluate(
    context::dependency_solver&, diagnostic_op_consumer& diags) const
{
    if (value_.value_kind() == context::symbol_value_kind::UNDEF)
        diags.add_diagnostic(diagnostic_op::error_ME001(get_range()));
    return value_;
}

const mach_expression* mach_expr_constant::leftmost_term() const { return this; }

void mach_expr_constant::apply(mach_expr_visitor& visitor) const { visitor.visit(*this); }

size_t mach_expr_constant::hash() const
{
    auto result = (size_t)0x38402610af574281;
    if (value_.value_kind() == context::symbol_value_kind::ABS)
        result = hash_combine(result, value_.get_abs());
    return result;
}

mach_expr_ptr mach_expr_constant::clone() const
{
    return std::make_unique<mach_expr_constant>(value_.get_abs(), get_range());
}

//***********  mach_expr_symbol ************
mach_expr_symbol::mach_expr_symbol(context::id_index value, context::id_index qualifier, range rng)
    : mach_expression(rng)
    , value(value)
    , qualifier(qualifier)
{}

context::dependency_collector mach_expr_symbol::get_dependencies(context::dependency_solver& solver) const
{
    auto symbol = solver.get_symbol(value);

    if (symbol == nullptr || symbol->kind() == context::symbol_value_kind::UNDEF)
        return value;
    else if (symbol->kind() == context::symbol_value_kind::ABS && qualifier)
    {
        return context::dependency_collector::error();
    }
    else if (symbol->kind() == context::symbol_value_kind::RELOC)
    {
        auto reloc_value = symbol->value().get_reloc();
        if (qualifier)
        {
            if (!reloc_value.is_simple())
                return context::dependency_collector::error();
            reloc_value.bases().front().first.qualifier = qualifier;
        }
        return reloc_value;
    }
    else
        return context::dependency_collector();
}

mach_expr_constant::value_t mach_expr_symbol::evaluate(
    context::dependency_solver& solver, diagnostic_op_consumer& diags) const
{
    auto symbol = solver.get_symbol(value);

    if (symbol == nullptr || symbol->kind() == context::symbol_value_kind::UNDEF)
        return context::symbol_value();

    if (symbol->kind() == context::symbol_value_kind::ABS)
    {
        if (qualifier)
        {
            diags.add_diagnostic(diagnostic_op::error_ME004(get_range()));
        }
        return symbol->value();
    }
    else if (symbol->kind() == context::symbol_value_kind::RELOC)
    {
        if (!qualifier)
            return symbol->value();
        auto reloc_value = symbol->value().get_reloc();
        if (reloc_value.is_simple())
        {
            reloc_value.bases().front().first.qualifier = qualifier;
        }
        else
        {
            diags.add_diagnostic(diagnostic_op::error_ME006(get_range()));
        }
        return reloc_value;
    }

    assert(false);
    return context::symbol_value();
}

const mach_expression* mach_expr_symbol::leftmost_term() const { return this; }
void mach_expr_symbol::apply(mach_expr_visitor& visitor) const { visitor.visit(*this); }
size_t mach_expr_symbol::hash() const
{
    auto result = (size_t)0xdf510e8c145dd28d;

    result = hash_combine(result, (uintptr_t)value);
    if (qualifier)
        result = hash_combine(result, (uintptr_t)qualifier);
    return result;
}
mach_expr_ptr mach_expr_symbol::clone() const
{
    return std::make_unique<mach_expr_symbol>(value, qualifier, get_range());
}
bool mach_expr_symbol::do_is_similar(const mach_expression& expr) const
{
    const auto& e = static_cast<const mach_expr_symbol&>(expr);
    return value == e.value && qualifier == e.qualifier;
}
bool mach_expr_location_counter::do_is_similar(const mach_expression&) const { return true; }

mach_expr_location_counter::mach_expr_location_counter(range rng)
    : mach_expression(rng)
{}

context::dependency_collector mach_expr_location_counter::get_dependencies(context::dependency_solver& mi) const
{
    auto location_counter = mi.get_loctr();
    if (!location_counter.has_value())
        return context::dependency_collector::error();
    else
        return context::dependency_collector(*location_counter);
}

mach_expression::value_t mach_expr_location_counter::evaluate(
    context::dependency_solver& mi, diagnostic_op_consumer&) const
{
    auto location_counter = mi.get_loctr();
    if (!location_counter.has_value())
        return context::address(context::address::base {}, 0, {});
    else
        return *location_counter;
}

const mach_expression* mach_expr_location_counter::leftmost_term() const { return this; }

void mach_expr_location_counter::apply(mach_expr_visitor& visitor) const { visitor.visit(*this); }

size_t mach_expr_location_counter::hash() const { return (size_t)0x0009459ca772d69b; }

mach_expr_ptr mach_expr_location_counter::clone() const
{
    return std::make_unique<mach_expr_location_counter>(get_range());
}

bool mach_expr_default::do_is_similar(const mach_expression&) const { return true; }

mach_expr_default::mach_expr_default(range rng)
    : mach_expression(rng)
{}

context::dependency_collector mach_expr_default::get_dependencies(context::dependency_solver&) const
{
    return context::dependency_collector();
}

mach_expression::value_t mach_expr_default::evaluate(context::dependency_solver&, diagnostic_op_consumer&) const
{
    return value_t();
}

const mach_expression* mach_expr_default::leftmost_term() const { return this; }

void mach_expr_default::apply(mach_expr_visitor& visitor) const { visitor.visit(*this); }

size_t mach_expr_default::hash() const { return (size_t)0xd11a22d1aa4016e0; }

mach_expr_ptr mach_expr_default::clone() const { return std::make_unique<mach_expr_default>(get_range()); }

bool mach_expr_data_attr::do_is_similar(const mach_expression& expr) const
{
    const auto& e = static_cast<const mach_expr_data_attr&>(expr);
    return value == e.value && qualifier == e.qualifier && attribute == e.attribute;
}

mach_expr_data_attr::mach_expr_data_attr(context::id_index value,
    context::id_index qualifier,
    context::data_attr_kind attribute,
    range rng,
    range symbol_rng)
    : mach_expression(rng)
    , value(value)
    , qualifier(qualifier)
    , attribute(attribute)
    , symbol_range(symbol_rng)
{}

context::dependency_collector mach_expr_data_attr::get_dependencies(context::dependency_solver& solver) const
{
    auto symbol = solver.get_symbol(value);

    if (symbol == nullptr || !symbol->attributes().is_defined(attribute))
        return context::dependency_collector({ attribute, value });
    else
        return context::dependency_collector();
}

mach_expression::value_t mach_expr_data_attr::evaluate(
    context::dependency_solver& solver, diagnostic_op_consumer& diags) const
{
    auto symbol = solver.get_symbol(value);

    if (symbol == nullptr)
    {
        return context::symbol_attributes::default_value(attribute);
    }
    else if ((attribute == context::data_attr_kind::S || attribute == context::data_attr_kind::I)
        && !symbol->attributes().can_have_SI_attr())
    {
        diags.add_diagnostic(diagnostic_op::warning_W011(get_range()));
        return 0;
    }
    else if (symbol->attributes().is_defined(attribute))
    {
        return symbol->attributes().get_attribute_value(attribute);
    }
    else
        return context::symbol_attributes::default_value(attribute);
}

const mach_expression* mach_expr_data_attr::leftmost_term() const { return this; }

void mach_expr_data_attr::apply(mach_expr_visitor& visitor) const { visitor.visit(*this); }

size_t mach_expr_data_attr::hash() const
{
    auto result = (size_t)0xa2957a462d908bd2;
    result = hash_combine(result, (uintptr_t)value);
    if (qualifier)
        result = hash_combine(result, (uintptr_t)qualifier);
    result = hash_combine(result, (size_t)attribute);

    return result;
}

mach_expr_ptr mach_expr_data_attr::clone() const
{
    return std::make_unique<mach_expr_data_attr>(value, qualifier, attribute, get_range(), symbol_range);
}

bool mach_expr_data_attr_literal::do_is_similar(const mach_expression& expr) const
{
    const auto& e = static_cast<const mach_expr_data_attr_literal&>(expr);
    return attribute == e.attribute && utils::is_similar(lit, e.lit);
}

mach_expr_data_attr_literal::mach_expr_data_attr_literal(
    std::unique_ptr<mach_expr_literal> value, context::data_attr_kind attribute, range whole_rng, range symbol_rng)
    : mach_expression(whole_rng)
    , attribute(attribute)
    , symbol_range(symbol_rng)
    , lit(std::move(value))
{}

context::dependency_collector mach_expr_data_attr_literal::get_dependencies(context::dependency_solver& solver) const
{
    return lit->get_data_definition().get_dependencies(solver);
}

mach_expression::value_t mach_expr_data_attr_literal::evaluate(
    context::dependency_solver& solver, diagnostic_op_consumer& diags) const
{
    context::symbol_attributes attrs = lit->get_data_definition().get_symbol_attributes(solver, diags);
    if ((attribute == context::data_attr_kind::S || attribute == context::data_attr_kind::I)
        && !attrs.can_have_SI_attr())
    {
        diags.add_diagnostic(diagnostic_op::warning_W011(get_range()));
        return 0;
    }
    return attrs.get_attribute_value(attribute);
}

const mach_expression* mach_expr_data_attr_literal::leftmost_term() const { return this; }

void mach_expr_data_attr_literal::apply(mach_expr_visitor& visitor) const { visitor.visit(*this); }

size_t mach_expr_data_attr_literal::hash() const
{
    auto result = (size_t)0x7DD0367F88459860;
    result = hash_combine(result, lit->hash());
    result = hash_combine(result, (size_t)attribute);

    return result;
}

mach_expr_ptr mach_expr_data_attr_literal::clone() const
{
    return std::make_unique<mach_expr_data_attr_literal>(
        std::unique_ptr<mach_expr_literal>(static_cast<mach_expr_literal*>(lit->clone().release())),
        attribute,
        get_range(),
        symbol_range);
}

bool mach_expr_literal::do_is_similar(const mach_expression& expr) const
{
    const auto& e = static_cast<const mach_expr_literal&>(expr);
    return utils::is_similar(m_literal_data->get_dd(), e.m_literal_data->get_dd());
}

mach_expr_literal::mach_expr_literal(range rng, semantics::literal_si lit)
    : mach_expression(rng)
    , m_literal_data(std::move(lit))
{}

context::dependency_collector mach_expr_literal::get_dependencies(context::dependency_solver& solver) const
{
    auto length_deps = m_literal_data->get_dd().get_length_dependencies(solver);
    // literal size has to be evaluable at the definition point (ASMA151E)
    if (length_deps.has_error || length_deps.contains_dependencies())
        return context::dependency_collector::error();
    else
    {
        auto symbol_id = get_literal_id(solver);
        auto symbol = solver.get_symbol(symbol_id);

        if (symbol == nullptr || symbol->kind() == context::symbol_value_kind::UNDEF)
            return symbol_id;
        else if (symbol->kind() == context::symbol_value_kind::RELOC)
            return symbol->value().get_reloc();
        else
            return context::dependency_collector();
    }
}

mach_expression::value_t mach_expr_literal::evaluate(context::dependency_solver& solver, diagnostic_op_consumer&) const
{
    auto symbol = solver.get_symbol(get_literal_id(solver));

    if (symbol == nullptr || symbol->kind() == context::symbol_value_kind::UNDEF)
        return context::symbol_value();

    return symbol->value();
}

const mach_expression* mach_expr_literal::leftmost_term() const { return this; }

void mach_expr_literal::apply(mach_expr_visitor& visitor) const { visitor.visit(*this); }

size_t mach_expr_literal::hash() const { return m_literal_data->get_dd().hash(); }

mach_expr_ptr mach_expr_literal::clone() const
{
    return std::make_unique<mach_expr_literal>(get_range(), m_literal_data);
}

const data_definition& mach_expr_literal::get_data_definition() const { return m_literal_data->get_dd(); }

context::id_index mach_expr_literal::get_literal_id(context::dependency_solver& solver) const
{
    return solver.get_literal_id(
        std::shared_ptr<const expressions::data_definition>(m_literal_data, &m_literal_data->get_dd()));
}

} // namespace hlasm_plugin::parser_library::expressions
