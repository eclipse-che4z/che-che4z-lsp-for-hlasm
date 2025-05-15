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

#ifndef HLASMPLUGIN_PARSERLIBRARY_EXPRESSIONS_DATA_DEF_H
#define HLASMPLUGIN_PARSERLIBRARY_EXPRESSIONS_DATA_DEF_H

#include <cstdint>

#include "checking/data_definition/data_def_fields.h"
#include "context/id_index.h"
#include "context/ordinary_assembly/alignment.h"
#include "nominal_value.h"

namespace hlasm_plugin::parser_library {
class diagnostic_collector;
} // namespace hlasm_plugin::parser_library

namespace hlasm_plugin::parser_library::checking {
class data_def_type;
enum class data_instr_type;
} // namespace hlasm_plugin::parser_library::checking

namespace hlasm_plugin::parser_library::semantics {
class collector;
}


namespace hlasm_plugin::parser_library::expressions {
class mach_expr_visitor;

// Represents data definition operand as it was written into source code.
// Uses machine expressions to represent all modifiers and nominal value.
struct data_definition final : public context::dependable
{
    enum class length_type : unsigned char
    {
        BYTE,
        BIT
    };
    // Stores type, extension, all the modifiers and nominal value.
    // When pointer is null, it means the user omitted it.
    length_type length_type = length_type::BYTE;
    char type;
    char extension = 0;
    bool references_loctr = false;

    mach_expr_ptr dupl_factor = nullptr;
    mach_expr_ptr program_type = nullptr;
    mach_expr_ptr length = nullptr;
    mach_expr_ptr scale = nullptr;
    mach_expr_ptr exponent = nullptr;
    nominal_value_ptr nominal_value = nullptr;

    range type_range;
    range extension_range;

    // Returns conjunction of all dependencies of all expression in data_definition.
    context::dependency_collector get_dependencies(context::dependency_solver& solver) const override;
    // Returns conjunction of dependencies of length modifier and duplication factor, which are the only ones
    // that matter when determining how much space DC needs.
    context::dependency_collector get_length_dependencies(context::dependency_solver& solver) const;
    // Returns representation of the type of this data definition.
    const checking::data_def_type* access_data_def_type() const;
    context::alignment get_alignment() const;

    char get_type_attribute() const;
    // Expects, that scale does not have unresolved dependencies
    int32_t get_scale_attribute(context::dependency_solver& info, diagnostic_op_consumer& diags) const;
    uint32_t get_length_attribute(context::dependency_solver& info, diagnostic_op_consumer& diags) const;
    int32_t get_integer_attribute(context::dependency_solver& info, diagnostic_op_consumer& diags) const;
    context::symbol_attributes get_symbol_attributes(
        context::dependency_solver& info, diagnostic_op_consumer& diags) const;

    // Returns true, if this data definition has one of the types, that take expressions consisting of only one
    // symbol (like V or R)
    bool expects_single_symbol() const;
    // Returns true, if data definition does not violate the single symbol rule, if the type requires it.
    bool check_single_symbol_ok(diagnostic_op_consumer& diags) const;

    // When any of the evaluated expressions have dependencies, resulting modifier will have data_def_field::present set
    // to false
    checking::dupl_factor_modifier_t evaluate_dupl_factor(
        context::dependency_solver& info, diagnostic_op_consumer& diags) const;
    checking::data_def_length_t evaluate_length(context::dependency_solver& info, diagnostic_op_consumer& diags) const;
    checking::scale_modifier_t evaluate_scale(context::dependency_solver& info, diagnostic_op_consumer& diags) const;
    checking::exponent_modifier_t evaluate_exponent(
        context::dependency_solver& info, diagnostic_op_consumer& diags) const;

    // When any of the evaluated expressions have dependencies, resulting modifier will have
    // data_def_expr::ignored or data_def_address::ignored set to false
    checking::nominal_value_t evaluate_nominal_value(
        context::dependency_solver& info, diagnostic_op_consumer& diags) const;
    checking::reduced_nominal_value_t evaluate_reduced_nominal_value() const;

    long long evaluate_total_length(context::dependency_solver& info,
        checking::data_instr_type checking_rules,
        diagnostic_op_consumer& diags) const;

    void apply(mach_expr_visitor& visitor) const;

    friend bool is_similar(const data_definition& l, const data_definition& r) noexcept;

    size_t hash() const;
};

} // namespace hlasm_plugin::parser_library::expressions

#endif
