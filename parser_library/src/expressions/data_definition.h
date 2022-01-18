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

#include <optional>

#include "checking/data_definition/data_def_fields.h"
#include "checking/diagnostic_collector.h"
#include "context/id_storage.h"
#include "context/ordinary_assembly/alignment.h"
#include "nominal_value.h"

namespace hlasm_plugin::parser_library::checking {
class data_def_type;
}

namespace hlasm_plugin::parser_library::semantics {
class collector;
}


namespace hlasm_plugin::parser_library::expressions {
class mach_expr_visitor;

// Represents data definition operand as it was written into source code.
// Uses machine expressions to represent all modifiers and nominal value.
struct data_definition final : public diagnosable_op_impl, public context::dependable
{
    enum class length_type : unsigned char
    {
        BYTE,
        BIT
    };
    // Stores type, extension, all the modifiers and nominal value.
    // When pointer is null, it means the user omitted it.
    mach_expr_ptr dupl_factor = nullptr;
    char type;
    range type_range;
    char extension = 0;
    range extension_range;
    mach_expr_ptr program_type = nullptr;
    mach_expr_ptr length = nullptr;
    mach_expr_ptr scale = nullptr;
    mach_expr_ptr exponent = nullptr;
    nominal_value_ptr nominal_value = nullptr;

    length_type length_type = length_type::BYTE;
    bool references_loctr = false;

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
    int32_t get_scale_attribute(context::dependency_solver& info) const;
    uint32_t get_length_attribute(context::dependency_solver& info) const;
    int32_t get_integer_attribute(context::dependency_solver& info) const;

    // Returns true, if this data definition has one of the types, that take expressions consisting of only one symbol
    // (like V or R)
    bool expects_single_symbol() const;
    // Returns true, if data definition does not violate the single symbol rule, if the type requires it.
    bool check_single_symbol_ok(const diagnostic_collector& add_diagnostic) const;
    // Returns list of symbol names written to nominal value.
    // Expects that check_single_symbol_ok returned true.
    std::vector<context::id_index> get_single_symbol_names() const;

    void collect_diags() const override;

    // When any of the evaluated expressions have dependencies, resulting modifier will have data_def_field::present set
    // to false
    checking::dupl_factor_modifier_t evaluate_dupl_factor(context::dependency_solver& info) const;
    checking::data_def_length_t evaluate_length(context::dependency_solver& info) const;
    checking::scale_modifier_t evaluate_scale(context::dependency_solver& info) const;
    checking::exponent_modifier_t evaluate_exponent(context::dependency_solver& info) const;

    // When any of the evaluated expressions have dependencies, resulting modifier will have
    // data_def_expr::ignored or data_def_address::ignored set to false
    checking::nominal_value_t evaluate_nominal_value(context::dependency_solver& info) const;

    void apply(mach_expr_visitor& visitor) const;

    friend bool is_similar(const data_definition& l, const data_definition& r) noexcept;

    size_t hash() const;
};

// Guide the ANTLR parser through the process of parsing data definition expression
class data_definition_parser
{
public:
    struct allowed_t
    {
        bool expression;
        bool string;
        bool plus_minus;
        bool dot;
    };

    const allowed_t& allowed() const { return m_allowed; }

    using push_arg = std::variant<std::string_view, mach_expr_ptr>;

    void push(push_arg v, range r);
    void push(nominal_value_ptr n);

    data_definition take_result();

    void set_collector(semantics::collector& collector) { m_collector = &collector; }

private:
    std::optional<std::pair<int, range>> parse_number(std::string_view& v, range& r);
    mach_expr_ptr read_number(push_arg& v, range& r);
    mach_expr_ptr read_number(std::string_view& v, range& r);

    allowed_t m_allowed = { true, true, false, false };
    data_definition m_result;

    semantics::collector* m_collector = nullptr;
    std::optional<position> m_expecting_next;

    enum class state
    {
        duplicating_factor,
        read_type,
        try_reading_program,
        read_program,
        try_reading_bitfield,
        try_reading_length,
        read_length,
        try_reading_scale,
        read_scale,
        try_reading_exponent,
        read_exponent,
        too_much_text,
    } m_state = state::duplicating_factor;
};

} // namespace hlasm_plugin::parser_library::expressions

#endif