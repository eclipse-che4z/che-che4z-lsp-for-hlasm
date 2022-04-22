/*
 * Copyright (c) 2022 Broadcom.
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

#ifndef HLASMPARSER_PARSERLIBRARY_INSTRUCTION_SET_VERSION_H
#define HLASMPARSER_PARSERLIBRARY_INSTRUCTION_SET_VERSION_H

#include <algorithm>
#include <array>
#include <string>

// Available instruction sets versions
namespace hlasm_plugin::parser_library {
enum class instruction_set_version
{
    ZOP = 1,
    YOP,
    Z9,
    Z10,
    Z11,
    Z12,
    Z13,
    Z14,
    Z15,
    ESA,
    XA,
    _370,
    DOS,
    UNI
};

constexpr std::array<std::pair<std::string_view, instruction_set_version>, 23> instr_set_version_equivalents = { {
    { std::string_view("370"), instruction_set_version::_370 },
    { std::string_view("DOS"), instruction_set_version::DOS },
    { std::string_view("ESA"), instruction_set_version::ESA },
    { std::string_view("UNI"), instruction_set_version::UNI },
    { std::string_view("XA"), instruction_set_version::XA },
    { std::string_view("YOP"), instruction_set_version::YOP },
    { std::string_view("Z10"), instruction_set_version::Z10 },
    { std::string_view("Z11"), instruction_set_version::Z11 },
    { std::string_view("Z12"), instruction_set_version::Z12 },
    { std::string_view("Z13"), instruction_set_version::Z13 },
    { std::string_view("Z14"), instruction_set_version::Z14 },
    { std::string_view("Z15"), instruction_set_version::Z15 },
    { std::string_view("Z9"), instruction_set_version::Z9 },
    { std::string_view("ZOP"), instruction_set_version::ZOP },
    { std::string_view("ZS1"), instruction_set_version::ZOP },
    { std::string_view("ZS2"), instruction_set_version::YOP },
    { std::string_view("ZS3"), instruction_set_version::Z9 },
    { std::string_view("ZS4"), instruction_set_version::Z10 },
    { std::string_view("ZS5"), instruction_set_version::Z11 },
    { std::string_view("ZS6"), instruction_set_version::Z12 },
    { std::string_view("ZS7"), instruction_set_version::Z13 },
    { std::string_view("ZS8"), instruction_set_version::Z14 },
    { std::string_view("ZS9"), instruction_set_version::Z15 },
} };

#if __cpp_lib_ranges
static_assert(std::ranges::is_sorted(
    instr_set_version_equivalents, {}, &std::pair<std::string_view, instruction_set_version>::first));
#else
static_assert(std::is_sorted(std::begin(instr_set_version_equivalents),
    std::end(instr_set_version_equivalents),
    [](const auto& l, const auto& r) { return l.first < r.first; }));
#endif

} // namespace hlasm_plugin::parser_library

#endif