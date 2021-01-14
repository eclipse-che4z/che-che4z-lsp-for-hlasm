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

// Represents data definition operand as it was written into source code.
// Uses machine expressions to represent all modifiers and nominal value.
struct data_definition : public diagnosable_op_impl, public context::dependable
{
    enum class length_type
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

    inline static const char* expr_placeholder = "&";
    inline static const char* nominal_placeholder = " ";
    // Creates the data definition. format is the string representation of data definition as the user
    // has written it, excapt all expressions are replaced with '&' and nominal value is replaced with " ".
    // The expressions are passed separately as exprs in the same order as '&' appear in the format. begin is
    // the position of first character of data definition. The function filles the passed collector with
    // highlighting information.
    static data_definition create(semantics::collector& coll,
        std::string format,
        mach_expr_list exprs,
        nominal_value_ptr nominal,
        position begin);

    // Returns conjunction of all dependencies of all expression in data_definition.
    virtual context::dependency_collector get_dependencies(context::dependency_solver& solver) const override;
    // Returns conjunction of dependencies of length modifier and duplication factor, which are the only ones
    // that matter when determining how much space DC needs.
    context::dependency_collector get_length_dependencies(context::dependency_solver& solver) const;
    // Returns representation of the type of this data definition.
    const checking::data_def_type* access_data_def_type() const;
    context::alignment get_alignment() const;

    char get_type_attribute() const;
    // Expects, that scale does not have unresolved dependencies
    int32_t get_scale_attribute(expressions::mach_evaluate_info info) const;
    uint32_t get_length_attribute(expressions::mach_evaluate_info info) const;
    int32_t get_integer_attribute(expressions::mach_evaluate_info info) const;

    // Returns true, if this data definition has one of the types, that take expressions consisting of only one symbol
    // (like V or R)
    bool expects_single_symbol() const;
    // Returns true, if data definition does not violate the single symbol rule, if the type requires it.
    bool check_single_symbol_ok(const diagnostic_collector& add_diagnostic) const;
    // Returns list of symbol names written to nominal value.
    // Expects that check_single_symbol_ok returned true.
    std::vector<context::id_index> get_single_symbol_names() const;

    // Assigns location counter to all expressions used to represent this data_definition.
    void assign_location_counter(context::address loctr_value);

    void collect_diags() const override;

    // When any of the evaluated expressions have dependencies, resulting modifier will have data_def_field::present set
    // to false
    checking::dupl_factor_modifier_t evaluate_dupl_factor(expressions::mach_evaluate_info info) const;
    checking::data_def_length_t evaluate_length(expressions::mach_evaluate_info info) const;
    checking::scale_modifier_t evaluate_scale(expressions::mach_evaluate_info info) const;
    checking::exponent_modifier_t evaluate_exponent(expressions::mach_evaluate_info info) const;

    // When any of the evaluated expressions have dependencies, resulting modifier will have
    // data_def_expr::ignored or data_def_address::ignored set to false
    checking::nominal_value_t evaluate_nominal_value(expressions::mach_evaluate_info info) const;

private:
    class parser;
};

// Parses data definition from the form it comes from the grammar, the input comes with expressions and nominal value
// replaced by special characters. On one instance, parse may be called only once.
// If there are errors, they are reported with diagnostics of data_definition. Only those checks needed for valid
// parsing of the data definition are performed here (syntax errors). Checks regarding values of fields are
// implemented in checking.
class data_definition::parser
{
public:
    parser(semantics::collector& coll,
        std::string format,
        mach_expr_list exprs,
        nominal_value_ptr nominal,
        position begin);
    // Parses the data definition specified as parameters of constructor.
    data_definition parse();

    // Parses number from format on position p_ and returns it, if it is valid.
    std::optional<int> parse_number();
    // Returns position behind number that follows.
    size_t get_number_end(size_t begin);
    // Sets the position behind the expression.
    void update_position(const mach_expression& e);
    // Moves the position one character forward.
    // TODO: continuation support: current implementation never moves to the next line
    void update_position_by_one();
    void parse_duplication_factor();
    void parse_modifier();
    // Returns the next expression from the input list.
    mach_expr_ptr move_next_expression();
    // Either returns expression that user specified as a modifier or creates expression from number that he specified
    // Either L(48+12) or L60
    mach_expr_ptr parse_modifier_num_or_expr();
    // Moves the expression to the right modifier field in the result.
    void assign_expr_to_modifier(char modifier, mach_expr_ptr expr);

private:
    semantics::collector& collector_;
    // Input parameters specifying the data definition as grammar parsed it
    std::string format_;
    mach_expr_list exprs_;
    nominal_value_ptr nominal_;

    // Current state of parsing:
    // Current position in text
    position pos_;
    // Current position in format
    size_t p_;
    // Current position in expressions vector
    size_t exprs_i_;
    // Lists currently remaining valid modifiers: (P)rogram type, (L)ength, (S)cale, (E)xponent
    std::string remaining_modifiers_ = "PLSE";
    bool nominal_parsed_ = false;

    data_definition result_;
};

} // namespace hlasm_plugin::parser_library::expressions

#endif