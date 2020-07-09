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

#ifndef HLASMPLUGIN_PARSERLIBRARY_CHECKING_DATA_DEF_TYPE_BASE_H
#define HLASMPLUGIN_PARSERLIBRARY_CHECKING_DATA_DEF_TYPE_BASE_H

#include <cstdint>
#include <functional>
#include <map>
#include <set>
#include <variant>

#include "checking/diagnostic_collector.h"
#include "context/ordinary_assembly/alignment.h"
#include "data_def_fields.h"
namespace hlasm_plugin::parser_library::checking {

class data_definition_operand;

// DC and DS have subtle differences; this enum is used to template functions accordingly.
enum class data_instr_type
{
    DC,
    DS
};

// Modifier spec specifies allowed values for modifiers.
// modifier_bound: specifies inclusive range of allowed values.
// n_a: the modifier cannot be specified, a diagnostic is registered otherwise.
// no_check: the modifier is not checked at all, typically derived data_def_type checks it in other way.
// ignored: the modifier is ignored during assembly, a warning is registered if present.
struct modifier_bound
{
    int min;
    int max;
};
struct n_a
{};
struct no_check
{};
struct ignored
{};
using modifier_spec = std::variant<modifier_bound, n_a, no_check, ignored>;

// Implicit length is either fixed number or is derived from (string) nominal value.
struct as_needed
{};
using implicit_length_t = std::variant<uint64_t, as_needed>;

static const std::set<char> type_extensions({ 'A', 'E', 'U', 'H', 'B', 'D', 'Q', 'Y' });

// Type of nominal value that various types of data definition expect.
enum class nominal_value_type
{
    STRING,
    EXPRESSIONS,
    ADDRESS_OR_EXPRESSION
};

// To check in context of this file means to report diagnostics using specified diagnostic_collector
// and return false if there was an error found (if a warning was found, true is returned).

// Base type for all data definition types and type extensions. Checks the operand and gets its attributes and
// alignment.
class data_def_type
{
public:
    // constructor for types with  the same lengths in DC and DS instruction
    data_def_type(char type,
        char extension,
        modifier_spec bit_length_spec,
        modifier_spec length_spec,
        modifier_spec scale_spec,
        modifier_spec exponent_spec,
        nominal_value_type nominal_type,
        context::alignment implicit_alignment,
        implicit_length_t implicit_length);

    // constructor for types with different allowed lengths with DS instruction
    data_def_type(char type,
        char extension,
        modifier_spec bit_length_spec,
        modifier_spec length_spec,
        int max_ds_length_spec,
        modifier_spec scale_spec,
        modifier_spec exponent_spec,
        nominal_value_type nominal_type,
        context::alignment implicit_alignment,
        implicit_length_t implicit_length);


    // Checks data def operand, returns false when there was an error. Adds found diagnostics using specified diagnostic
    // collector.
    template<data_instr_type instr_type>
    bool check(const data_definition_operand& op, const diagnostic_collector& add_diagnostic) const;

    bool check_DC(const data_definition_operand& op, const diagnostic_collector& add_diagnostic) const;
    bool check_DS(const data_definition_operand& op, const diagnostic_collector& add_diagnostic) const;

    virtual bool expects_single_symbol() const;

    context::alignment get_alignment(bool length_present) const;
    // returns length of the operand in bits
    uint64_t get_length(const data_definition_operand& op) const;
    // returns the length attribute of operand with specified length modifier and nominal value
    uint32_t get_length_attribute(const data_def_length_t& length, const nominal_value_t& nominal) const;
    // returns scale attribute of operand with specified scale modifier and nominal value
    int16_t get_scale_attribute(const scale_modifier_t& scale, const nominal_value_t& nominal) const;
    // returns length of operand with specified scale modifier and nominal value
    int32_t get_integer_attribute(
        const data_def_length_t& length, const scale_modifier_t& scale, const nominal_value_t& nominal) const;
    // Returns type corresponding to specified type and extension.
    static const data_def_type* access_data_def_type(char type, char extension);

    static const std::map<char, std::set<char>> types_extensions;

    virtual ~data_def_type() = 0;

protected:
    char type;
    char extension;
    std::string type_str;

    // When implicit length is "as needed" - it depends on nominal value, returns the implicit length in bytes.
    // All types that have implicit length "as needed" must override this function.
    virtual uint64_t get_nominal_length(const nominal_value_t& op) const;

    virtual uint32_t get_nominal_length_attribute(const nominal_value_t& op) const;
    // Gets the value of scale attribute when there is no scale modifier defined by user.
    virtual int16_t get_implicit_scale(const nominal_value_t& op) const;

    virtual int32_t get_integer_attribute_impl(uint32_t length, int32_t scale) const;

private:
    // Checks properties of data def operand that all types have in common - modifiers, duplication factor.
    template<data_instr_type type>
    std::pair<bool, bool> check_base(
        const data_definition_operand& op, const diagnostic_collector& add_diagnostic) const;

    // Data def types override this function to implement type-specific check. check_nominal specifies whether it is
    // safe to access nominal value of operand(has correct type, etc..).
    virtual bool check(
        const data_definition_operand& op, const diagnostic_collector& add_diagnostic, bool check_nominal) const;

    // Concatenates the two characters and returns resulting string.
    static std::string init_type_str(char type, char extension);
    // Checks duplication factor.
    bool check_dupl_factor(const data_definition_operand& op, const diagnostic_collector& add_diagnostic) const;

    // Checks the length modifier.
    template<data_instr_type type>
    bool check_length(const data_definition_operand& op, const diagnostic_collector& add_diagnostic) const;

    // Checks whether the nominal value is present when it is mandatory. Returns two booleans: the first one specifies
    // whether there was an error, the second one specifies whether the nominal value is present and needs to be checked
    // further.
    template<data_instr_type type>
    std::pair<bool, bool> check_nominal_present(
        const data_definition_operand& op, const diagnostic_collector& add_diagnostic) const;

    // Checks if nominal value has the right type and is safe to access. Expects that nominal type is present.
    bool check_nominal_type(const data_definition_operand& op, const diagnostic_collector& add_diagnostic) const;

    size_t get_number_of_values_in_nominal(const nominal_value_t& nom) const;

    template<data_instr_type type>
    modifier_spec get_length_spec() const;

    template<data_instr_type type>
    modifier_spec get_bit_length_spec() const;

    nominal_value_type nominal_type;

    modifier_spec bit_length_spec_;
    modifier_spec length_spec_;
    modifier_spec ds_length_spec_;
    modifier_spec ds_bit_length_spec_;
    modifier_spec scale_spec_;
    modifier_spec exponent_spec_;

    context::alignment alignment_;

    implicit_length_t implicit_length_;
};

} // namespace hlasm_plugin::parser_library::checking


#endif
