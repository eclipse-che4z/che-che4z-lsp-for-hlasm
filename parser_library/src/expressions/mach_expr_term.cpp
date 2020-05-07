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

#include "arithmetic_expression.h"
#include "checking/checker_helper.h"

using namespace hlasm_plugin::parser_library::expressions;
using namespace hlasm_plugin::parser_library;

//***********  mach_expr_constant ************
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

mach_expr_constant::value_t mach_expr_constant::evaluate(mach_evaluate_info) const { return value_; }

void mach_expr_constant::fill_location_counter(context::address) {}

const mach_expression* mach_expr_constant::leftmost_term() const { return this; }



//***********  mach_expr_symbol ************
mach_expr_symbol::mach_expr_symbol(context::id_index value, range rng)
    : mach_expression(rng)
    , value(value)
    , len_expr(value, context::data_attr_kind::L, rng)
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

mach_expr_constant::value_t mach_expr_symbol::evaluate(mach_evaluate_info info) const
{
    auto symbol = info.get_symbol(value);

    if (symbol == nullptr || symbol->kind() == context::symbol_value_kind::UNDEF)
        return context::symbol_value();

    return symbol->value();
}
void mach_expr_symbol::fill_location_counter(context::address) {}
const mach_expression* mach_expr_symbol::leftmost_term() const { return this; }
//***********  mach_expr_self_def ************
mach_expr_self_def::mach_expr_self_def(std::string option, std::string value, range rng)
    : mach_expression(rng)
{
    auto ae = arithmetic_expression::from_string(
        std::move(option), std::move(value), false); // could generate diagnostic + DBCS
    ae->diag->diag_range = rng;
    if (ae->has_error())
        add_diagnostic(*ae->diag);
    else
        value_ = ae->get_numeric_value();
}

context::dependency_collector mach_expr_self_def::get_dependencies(context::dependency_solver&) const
{
    return context::dependency_collector();
}

mach_expr_self_def::value_t mach_expr_self_def::evaluate(mach_evaluate_info) const { return value_; }

void mach_expr_self_def::fill_location_counter(context::address) {}

const mach_expression* mach_expr_self_def::leftmost_term() const { return this; }

mach_expr_location_counter::mach_expr_location_counter(range rng)
    : mach_expression(rng)
{}

context::dependency_collector mach_expr_location_counter::get_dependencies(context::dependency_solver&) const
{
    if (!location_counter.has_value())
        return context::dependency_collector(true);
    else
        return context::dependency_collector(*location_counter);
}

mach_expression::value_t mach_expr_location_counter::evaluate(mach_evaluate_info) const
{
    if (!location_counter.has_value())
        return context::address({ nullptr }, 0, {});
    else
        return *location_counter;
}

void mach_expr_location_counter::fill_location_counter(context::address addr) { location_counter = std::move(addr); }

const mach_expression* mach_expr_location_counter::leftmost_term() const { return this; }

mach_expr_default::mach_expr_default(range rng)
    : mach_expression(rng)
{}

context::dependency_collector mach_expr_default::get_dependencies(context::dependency_solver&) const
{
    return context::dependency_collector();
}

mach_expression::value_t mach_expr_default::evaluate(mach_evaluate_info) const { return value_t(); }

void mach_expr_default::fill_location_counter(context::address) {}

const mach_expression* mach_expr_default::leftmost_term() const { return this; }

void mach_expr_default::collect_diags() const {}

mach_expr_data_attr::mach_expr_data_attr(context::id_index value, context::data_attr_kind attribute, range rng)
    : mach_expression(rng)
    , value(value)
    , attribute(attribute)
{}

context::dependency_collector mach_expr_data_attr::get_dependencies(context::dependency_solver& solver) const
{
    auto symbol = solver.get_symbol(value);

    if (symbol == nullptr || !symbol->attributes().is_defined(attribute))
    {
        return context::dependency_collector({ attribute, value });
    }
    else
        return context::dependency_collector();
}

mach_expression::value_t mach_expr_data_attr::evaluate(mach_evaluate_info info) const
{
    auto symbol = info.get_symbol(value);

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

void mach_expr_data_attr::fill_location_counter(context::address) {}

const mach_expression* mach_expr_data_attr::leftmost_term() const { return this; }
