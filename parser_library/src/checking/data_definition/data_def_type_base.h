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

#include <bitset>
#include <cstdint>
#include <map>
#include <memory>
#include <variant>

#include "context/ordinary_assembly/alignment.h"
#include "data_def_fields.h"

namespace hlasm_plugin::parser_library {
class diagnostic_collector;
namespace context {
enum class integer_type : unsigned char;
}
} // namespace hlasm_plugin::parser_library

namespace hlasm_plugin::parser_library::checking {

struct data_definition_common;
struct data_definition_operand;

// DC and DS have subtle differences; this enum is used to template functions accordingly.
enum class data_instr_type
{
    DC,
    DS,
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
    bool even = false;
};
struct n_a
{};
struct no_check
{};
struct ignored
{};
class bound_list
{
    std::bitset<32> m_allowed;

public:
    bound_list(std::initializer_list<unsigned> l) noexcept
    {
        for (unsigned i : l)
            m_allowed.set(i);
    }

    bool allowed(int32_t i) const noexcept { return i >= 0 && (uint32_t)i < m_allowed.size() && m_allowed.test(i); }

    std::string to_diag_list() const;
};
using modifier_spec = std::variant<modifier_bound, n_a, no_check, ignored, bound_list>;

// Implicit length is either fixed number or is derived from (string) nominal value.
struct as_needed
{};
using implicit_length_t = std::variant<uint64_t, as_needed>;

// Type of nominal value that various types of data definition expect.
enum class nominal_value_type : unsigned char
{
    STRING,
    EXPRESSIONS,
    ADDRESS_OR_EXPRESSION
};

// To check in context of this file means to report diagnostics using specified diagnostic_collector
// and return false if there was an error found (if a warning was found, true is returned).

enum class data_definition_type : char
{
    A = 'A',
    B = 'B',
    C = 'C',
    D = 'D',
    E = 'E',
    F = 'F',
    G = 'G',
    H = 'H',
    J = 'J',
    L = 'L',
    P = 'P',
    Q = 'Q',
    R = 'R',
    S = 'S',
    V = 'V',
    X = 'X',
    Y = 'Y',
    Z = 'Z',
};

// Base type for all data definition types and type extensions. Checks the operand and gets its attributes and
// alignment.
class data_def_type
{
public:
    enum class expects_single_symbol_t : bool
    {
        no,
        yes,
    };
    // constructor for types with  the same lengths in DC and DS instruction
    data_def_type(data_definition_type type,
        char extension,
        modifier_spec bit_length_spec,
        modifier_spec length_spec,
        modifier_spec scale_spec,
        modifier_spec exponent_spec,
        nominal_value_type nominal_type,
        context::alignment implicit_alignment,
        implicit_length_t implicit_length,
        context::integer_type int_type_,
        expects_single_symbol_t single_symbol = expects_single_symbol_t::no,
        bool ignores_scale = false);

    // constructor for types with different allowed lengths with DS instruction
    data_def_type(data_definition_type type,
        char extension,
        modifier_spec bit_length_spec,
        modifier_spec length_spec,
        int max_ds_length_spec,
        modifier_spec scale_spec,
        modifier_spec exponent_spec,
        nominal_value_type nominal_type,
        context::alignment implicit_alignment,
        implicit_length_t implicit_length,
        context::integer_type int_type_);

    bool expects_single_symbol() const noexcept { return single_symbol == expects_single_symbol_t::yes; }

    // returns length of the operand in bits
    uint64_t get_length(
        int32_t dupl_factor, int32_t length, bool length_in_bits, const reduced_nominal_value_t& rnv) const;
    // returns the length attribute of operand with specified length modifier and nominal value
    uint32_t get_length_attribute(const data_def_length_t& length, const reduced_nominal_value_t& nominal) const;
    // returns scale attribute of operand with specified scale modifier and nominal value
    int16_t get_scale_attribute(const scale_modifier_t& scale, const reduced_nominal_value_t& nominal) const;
    // Returns type corresponding to specified type and extension.
    static const data_def_type* access_data_def_type(char type, char extension);

    virtual ~data_def_type() = 0;

    [[nodiscard]] constexpr context::integer_type get_int_type() const noexcept { return int_type_; }
    [[nodiscard]] constexpr bool ignores_scale() const noexcept { return ignores_scale_; }

    // When implicit length is "as needed" - it depends on nominal value, returns the implicit length in bytes.
    // All types that have implicit length "as needed" must override this function.
    virtual uint64_t get_nominal_length(const reduced_nominal_value_t& op) const;

    virtual uint32_t get_nominal_length_attribute(const reduced_nominal_value_t& op) const;
    // Gets the value of scale attribute when there is no scale modifier defined by user.
    virtual int16_t get_implicit_scale(const reduced_nominal_value_t& op) const;

    // Data def types override this function to implement type-specific check. check_nominal specifies whether it is
    // safe to access nominal value of operand(has correct type, etc..).
    virtual bool check_impl(const data_definition_common& common,
        const nominal_value_t& nominal,
        const diagnostic_collector& add_diagnostic,
        bool check_nominal) const;

    // Checks if nominal value has the right type and is safe to access. Expects that nominal type is present.
    bool check_nominal_type(
        const nominal_value_t& op, const diagnostic_collector& add_diagnostic, const range& r) const;

    size_t get_number_of_values_in_nominal(const reduced_nominal_value_t& nom) const;

    modifier_spec get_length_spec(data_instr_type instr_type) const;

    modifier_spec get_bit_length_spec(data_instr_type instr_type) const;

    data_definition_type type() const noexcept { return (data_definition_type)type_ext[0]; }
    char extension() const noexcept { return type_ext[1]; }
    std::string_view type_str() const noexcept { return std::string_view(type_ext, 1 + !!type_ext[1]); }

    char type_ext[2];

    nominal_value_type nominal_type;

    modifier_spec bit_length_spec_;
    modifier_spec length_spec_;
    modifier_spec ds_length_spec_;
    modifier_spec ds_bit_length_spec_;
    modifier_spec scale_spec_;
    modifier_spec exponent_spec_;

    context::alignment alignment_;

    implicit_length_t implicit_length_;

    context::integer_type int_type_;
    bool ignores_scale_ = false;
    expects_single_symbol_t single_symbol = expects_single_symbol_t::no;
};

} // namespace hlasm_plugin::parser_library::checking


#endif
