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
    try
    {
        value_ = std::stoi(value_text);
    }
    catch (std::out_of_range&)
    {
        add_diagnostic(diagnostic_op::error_ME001(get_range()));
    }
}
mach_expr_constant::mach_expr_constant(int value, range rng)
    : mach_expression(rng)
    , value_(value)
{}

context::dependency_collector mach_expr_constant::get_dependencies(context::dependency_solver&) const
{
    return context::dependency_collector();
}

mach_expr_constant::value_t mach_expr_constant::evaluate(context::dependency_solver&) const { return value_; }

const mach_expression* mach_expr_constant::leftmost_term() const { return this; }

void mach_expr_constant::apply(mach_expr_visitor& visitor) const { visitor.visit(*this); }

size_t mach_expr_constant::hash() const
{
    auto result = (size_t)0x38402610af574281;
    if (value_.value_kind() == context::symbol_value_kind::ABS)
        result = hash_combine(result, value_.get_abs());
    return result;
}

//***********  mach_expr_symbol ************
mach_expr_symbol::mach_expr_symbol(context::id_index value, range rng)
    : mach_expression(rng)
    , value(value)
    , len_expr(value, context::data_attr_kind::L, rng, rng)
{}

context::dependency_collector mach_expr_symbol::get_dependencies(context::dependency_solver& solver) const
{
    auto symbol = solver.get_symbol(value);

    if (symbol == nullptr || symbol->kind() == context::symbol_value_kind::UNDEF)
        return value;
    else if (symbol->kind() == context::symbol_value_kind::RELOC)
        return symbol->value().get_reloc();
    else
        return context::dependency_collector();
}

mach_expr_constant::value_t mach_expr_symbol::evaluate(context::dependency_solver& info) const
{
    auto symbol = info.get_symbol(value);

    if (symbol == nullptr || symbol->kind() == context::symbol_value_kind::UNDEF)
        return context::symbol_value();

    return symbol->value();
}

const mach_expression* mach_expr_symbol::leftmost_term() const { return this; }
void mach_expr_symbol::apply(mach_expr_visitor& visitor) const { visitor.visit(*this); }
size_t mach_expr_symbol::hash() const
{
    auto result = (size_t)0xdf510e8c145dd28d;

    if (value)
        result = hash_combine(result, (uintptr_t)value);
    return result;
}
bool mach_expr_symbol::do_is_similar(const mach_expression& expr) const
{
    const auto& e = static_cast<const mach_expr_symbol&>(expr);
    return value == e.value;
}
//***********  mach_expr_self_def ************
bool mach_expr_self_def::do_is_similar(const mach_expression& expr) const
{
    const auto& e = static_cast<const mach_expr_self_def&>(expr);
    return value_.get_abs() == e.value_.get_abs();
}

mach_expr_self_def::mach_expr_self_def(std::string option, std::string value, range rng)
    : mach_expression(rng)
{
    diagnostic_adder add_diagnostic(*this, rng);
    value_ = ca_constant::self_defining_term(option, value, add_diagnostic);
}

context::dependency_collector mach_expr_self_def::get_dependencies(context::dependency_solver&) const
{
    return context::dependency_collector();
}

mach_expr_self_def::value_t mach_expr_self_def::evaluate(context::dependency_solver&) const { return value_; }

const mach_expression* mach_expr_self_def::leftmost_term() const { return this; }

void mach_expr_self_def::apply(mach_expr_visitor& visitor) const { visitor.visit(*this); }

size_t mach_expr_self_def::hash() const { return hash_combine((size_t)0x038d7ea26932b75b, value_.get_abs()); }

bool mach_expr_location_counter::do_is_similar(const mach_expression&) const { return true; }

mach_expr_location_counter::mach_expr_location_counter(range rng)
    : mach_expression(rng)
{}

context::dependency_collector mach_expr_location_counter::get_dependencies(context::dependency_solver& mi) const
{
    auto location_counter = mi.get_loctr();
    if (!location_counter.has_value())
        return context::dependency_collector(true);
    else
        return context::dependency_collector(*location_counter);
}

mach_expression::value_t mach_expr_location_counter::evaluate(context::dependency_solver& mi) const
{
    auto location_counter = mi.get_loctr();
    if (!location_counter.has_value())
        return context::address({ nullptr }, 0, {});
    else
        return *location_counter;
}

const mach_expression* mach_expr_location_counter::leftmost_term() const { return this; }

void mach_expr_location_counter::apply(mach_expr_visitor& visitor) const { visitor.visit(*this); }

size_t mach_expr_location_counter::hash() const { return (size_t)0x0009459ca772d69b; }

bool mach_expr_default::do_is_similar(const mach_expression&) const { return true; }

mach_expr_default::mach_expr_default(range rng)
    : mach_expression(rng)
{}

context::dependency_collector mach_expr_default::get_dependencies(context::dependency_solver&) const
{
    return context::dependency_collector();
}

mach_expression::value_t mach_expr_default::evaluate(context::dependency_solver&) const { return value_t(); }

const mach_expression* mach_expr_default::leftmost_term() const { return this; }

void mach_expr_default::apply(mach_expr_visitor& visitor) const { visitor.visit(*this); }

void mach_expr_default::collect_diags() const {}

size_t mach_expr_default::hash() const { return (size_t)0xd11a22d1aa4016e0; }

bool mach_expr_data_attr::do_is_similar(const mach_expression& expr) const
{
    const auto& e = static_cast<const mach_expr_data_attr&>(expr);
    return value == e.value && attribute == e.attribute && utils::is_similar(lit, e.lit);
}

mach_expr_data_attr::mach_expr_data_attr(
    context::id_index value, context::data_attr_kind attribute, range rng, range symbol_rng)
    : mach_expression(rng)
    , value(value)
    , attribute(attribute)
    , symbol_range(symbol_rng)
{}

mach_expr_data_attr::mach_expr_data_attr(
    std::unique_ptr<mach_expr_literal> value, context::data_attr_kind attribute, range whole_rng, range symbol_rng)
    : mach_expression(whole_rng)
    , value(nullptr)
    , attribute(attribute)
    , symbol_range(symbol_rng)
    , lit(std::move(value))
{}

context::dependency_collector mach_expr_data_attr::get_dependencies(context::dependency_solver& solver) const
{
    if (value)
    {
        auto symbol = solver.get_symbol(value);

        if (symbol == nullptr || !symbol->attributes().is_defined(attribute))
            return context::dependency_collector({ attribute, value });
        else
            return context::dependency_collector();
    }
    else if (lit)
    {
        return lit->get_data_definition()->get_dependencies(solver);
    }

    return context::dependency_collector(true);
}

mach_expression::value_t mach_expr_data_attr::evaluate(context::dependency_solver& solver) const
{
    if (value)
    {
        auto symbol = solver.get_symbol(value);

        if (symbol == nullptr)
        {
            return context::symbol_attributes::default_value(attribute);
        }
        else if ((attribute == context::data_attr_kind::S || attribute == context::data_attr_kind::I)
            && !symbol->attributes().can_have_SI_attr())
        {
            add_diagnostic(diagnostic_op::warning_W011(get_range()));
            return 0;
        }
        else if (symbol->attributes().is_defined(attribute))
        {
            return symbol->attributes().get_attribute_value(attribute);
        }
        else
            return context::symbol_attributes::default_value(attribute);
    }
    else if (lit)
    {
        (void)lit->evaluate(solver);
        if (lit->get_data_definition()->get_dependencies(solver).contains_dependencies())
        {
            // TODO: something should probably be done here
        }
        auto& dd = lit->get_data_definition();
        context::symbol_attributes attrs(context::symbol_origin::DAT,
            ebcdic_encoding::a2e[(unsigned char)dd->get_type_attribute()],
            dd->get_length_attribute(solver),
            dd->get_scale_attribute(solver),
            dd->get_integer_attribute(solver));
        if ((attribute == context::data_attr_kind::S || attribute == context::data_attr_kind::I)
            && !attrs.can_have_SI_attr())
        {
            add_diagnostic(diagnostic_op::warning_W011(get_range()));
            return 0;
        }
        return attrs.get_attribute_value(attribute);
    }
    return context::symbol_attributes::default_value(attribute);
}

const mach_expression* mach_expr_data_attr::leftmost_term() const { return this; }

void mach_expr_data_attr::apply(mach_expr_visitor& visitor) const { visitor.visit(*this); }

size_t mach_expr_data_attr::hash() const
{
    auto result = (size_t)0xa2957a462d908bd2;
    if (value)
        result = hash_combine(result, (uintptr_t)value);
    result = hash_combine(result, (size_t)attribute);
    if (lit)
        result = hash_combine(result, lit->hash());

    return result;
}

bool mach_expr_literal::do_is_similar(const mach_expression& expr) const
{
    const auto& e = static_cast<const mach_expr_literal&>(expr);
    return utils::is_similar(m_data_definition, e.m_data_definition);
}

mach_expr_literal::mach_expr_literal(range rng, data_definition dd, std::string dd_text)
    : mach_expression(rng)
    , m_data_definition(std::make_shared<data_definition>(std::move(dd)))
    , m_dd_text(std::move(dd_text))
{}

context::dependency_collector mach_expr_literal::get_dependencies(context::dependency_solver& solver) const
{
    auto length_deps = m_data_definition->get_length_dependencies(solver);
    // literal size has to be evaluable at the definition point (ASMA151E)
    if (length_deps.has_error || length_deps.contains_dependencies())
        return context::dependency_collector(true);
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

mach_expression::value_t mach_expr_literal::evaluate(context::dependency_solver& solver) const
{
    auto symbol = solver.get_symbol(get_literal_id(solver));

    if (symbol == nullptr || symbol->kind() == context::symbol_value_kind::UNDEF)
        return context::symbol_value();

    return symbol->value();
}

const mach_expression* mach_expr_literal::leftmost_term() const { return this; }

void mach_expr_literal::apply(mach_expr_visitor& visitor) const { visitor.visit(*this); }

void mach_expr_literal::collect_diags() const {}

size_t mach_expr_literal::hash() const { return m_data_definition->hash(); }

const std::shared_ptr<const data_definition>& mach_expr_literal::get_data_definition() const
{
    return m_data_definition;
}

context::id_index mach_expr_literal::get_literal_id(context::dependency_solver& solver) const
{
    return solver.get_literal_id(m_dd_text, m_data_definition, get_range());
}

} // namespace hlasm_plugin::parser_library::expressions
