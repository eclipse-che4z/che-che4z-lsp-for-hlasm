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

#include <array>
#include <bitset>
#include <cstdint>
#include <numeric>
#include <variant>

#include "context/ordinary_assembly/alignment.h"
#include "data_def_fields.h"

namespace hlasm_plugin::parser_library {
class diagnostic_collector;
struct diagnostic_op;
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
// ignored: the modifier is ignored during assembly, a warning is registered if present.
// bound_list: a list of allowed lengths
struct modifier_bound
{
    int min;
    int max;
    bool even = false;
};
struct n_a
{};
struct ignored
{};
class bound_list
{
    std::bitset<32> m_allowed;

public:
    constexpr bound_list(std::initializer_list<unsigned> l) noexcept
        : m_allowed(std::accumulate(l.begin(), l.end(), 0ULL, [](auto acc, auto v) { return acc | 1ULL << v; }))
    {
        // C++23: for (unsigned i : l) m_allowed.set(i);
    }

    bool allowed(int32_t i) const noexcept { return i >= 0 && (uint32_t)i < m_allowed.size() && m_allowed.test(i); }

    std::string to_diag_list() const;

    bool operator==(const bound_list&) const = default;
};
using modifier_spec = std::variant<modifier_bound, n_a, ignored, bound_list>;

// Implicit length is either fixed number or is derived from (string) nominal value.
struct as_needed
{
    struct impl_t
    {
        uint64_t (*get_nominal_length)(std::string_view) noexcept;
        uint32_t (*get_nominal_length_attribute)(std::string_view) noexcept;
        uint32_t empty_length;
        uint32_t error_length = 0;
    };

    [[nodiscard]] uint64_t get_nominal_length(std::string_view nom) const noexcept
    {
        return impl->get_nominal_length(nom);
    }
    [[nodiscard]] uint32_t get_nominal_length_attribute(std::string_view nom) const noexcept
    {
        return impl->get_nominal_length_attribute(nom);
    }
    [[nodiscard]] uint32_t get_empty_length() const noexcept { return impl->empty_length; }
    [[nodiscard]] uint32_t get_error_length() const noexcept { return impl->error_length; }

    explicit constexpr as_needed(const impl_t& impl) noexcept
        : impl(&impl)
    {}
    as_needed(impl_t&& impl) noexcept = delete;

private:
    const impl_t* impl;
};
using implicit_length_t = std::variant<uint64_t, as_needed>;

extern constinit const as_needed::impl_t B_nominal_extras;
extern constinit const as_needed::impl_t CA_CE_nominal_extras;
extern constinit const as_needed::impl_t CU_nominal_extras;
extern constinit const as_needed::impl_t G_nominal_extras;
extern constinit const as_needed::impl_t X_nominal_extras;
extern constinit const as_needed::impl_t P_nominal_extras;
extern constinit const as_needed::impl_t Z_nominal_extras;

using nominal_diag_func = diagnostic_op (*)(const range&, std::string_view);
nominal_diag_func check_nominal_H_F_FD(std::string_view nom) noexcept;
nominal_diag_func check_nominal_P_Z(std::string_view nom) noexcept;
nominal_diag_func check_nominal_E_D_L(std::string_view nom, char extension) noexcept;

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
    // constructor for types with  the same lengths in DC and DS instruction
    consteval data_def_type(data_definition_type type,
        char extension,
        modifier_spec bit_length_spec,
        modifier_spec length_spec,
        modifier_spec scale_spec,
        modifier_spec exponent_spec,
        context::alignment implicit_alignment,
        implicit_length_t implicit_length,
        context::integer_type int_type_,
        bool ignores_scale = false) noexcept;

    // constructor for types with different allowed lengths with DS instruction
    consteval data_def_type(data_definition_type type,
        char extension,
        modifier_spec bit_length_spec,
        modifier_spec length_spec,
        int max_ds_length_spec,
        modifier_spec scale_spec,
        modifier_spec exponent_spec,
        context::alignment implicit_alignment,
        implicit_length_t implicit_length,
        context::integer_type int_type_) noexcept;

    // returns length of the operand in bits
    uint64_t get_length(
        int32_t dupl_factor, int32_t length, bool length_in_bits, const reduced_nominal_value_t& rnv) const;
    // returns the length attribute of operand with specified length modifier and nominal value
    uint32_t get_length_attribute(const data_def_length_t& length, const reduced_nominal_value_t& nominal) const;
    // returns scale attribute of operand with specified scale modifier and nominal value
    int16_t get_scale_attribute(const scale_modifier_t& scale, const reduced_nominal_value_t& nominal) const;
    // Returns type corresponding to specified type and extension.
    static const data_def_type* access_data_def_type(char type, char extension);

    [[nodiscard]] constexpr context::integer_type get_int_type() const noexcept { return int_type_; }
    [[nodiscard]] constexpr bool ignores_scale() const noexcept { return ignores_scale_; }

    // Gets the value of scale attribute when there is no scale modifier defined by user.
    int16_t get_implicit_scale(const reduced_nominal_value_t& op) const;

    size_t get_number_of_values_in_nominal(const reduced_nominal_value_t& nom) const;

    modifier_spec get_length_spec(data_instr_type instr_type) const;

    modifier_spec get_bit_length_spec(data_instr_type instr_type) const;

    data_definition_type type() const noexcept { return (data_definition_type)type_ext[0]; }
    char extension() const noexcept { return type_ext[1]; }
    std::string_view type_str() const noexcept { return std::string_view(type_ext.data(), 1 + !!type_ext[1]); }

    std::array<char, 2> type_ext;

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
};

} // namespace hlasm_plugin::parser_library::checking


#endif
