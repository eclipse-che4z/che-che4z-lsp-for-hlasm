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

#include "assembler_options.h"

#include <span>
#include <string_view>

#include "compiler_options.h"
#include "nlohmann/json.hpp"
#include "utils/projectors.h"
#include "utils/string_operations.h"

namespace hlasm_plugin::parser_library::config {

namespace {
using instr_set_equivalent_pair = std::pair<std::string_view, instruction_set_version>;

constexpr std::array<instr_set_equivalent_pair, 25> instr_set_optable_equivalents = { {
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
    { std::string_view("Z16"), instruction_set_version::Z16 },
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
    { std::string_view("ZSA"), instruction_set_version::Z16 },
} };

static_assert(std::ranges::is_sorted(instr_set_optable_equivalents, {}, &instr_set_equivalent_pair::first));

constexpr std::array<instr_set_equivalent_pair, 58> instr_set_machine_equivalents = { {
    { std::string_view("ARCH-0"), instruction_set_version::XA },
    { std::string_view("ARCH-1"), instruction_set_version::ESA },
    { std::string_view("ARCH-10"), instruction_set_version::Z12 },
    { std::string_view("ARCH-11"), instruction_set_version::Z13 },
    { std::string_view("ARCH-12"), instruction_set_version::Z14 },
    { std::string_view("ARCH-13"), instruction_set_version::Z15 },
    { std::string_view("ARCH-14"), instruction_set_version::Z16 },
    { std::string_view("ARCH-2"), instruction_set_version::ESA },
    { std::string_view("ARCH-3"), instruction_set_version::ESA },
    { std::string_view("ARCH-4"), instruction_set_version::ESA },
    { std::string_view("ARCH-5"), instruction_set_version::ZOP },
    { std::string_view("ARCH-6"), instruction_set_version::YOP },
    { std::string_view("ARCH-7"), instruction_set_version::Z9 },
    { std::string_view("ARCH-8"), instruction_set_version::Z10 },
    { std::string_view("ARCH-9"), instruction_set_version::Z11 },
    { std::string_view("S370"), instruction_set_version::_370 },
    { std::string_view("S370ESA"), instruction_set_version::ESA },
    { std::string_view("S370XA"), instruction_set_version::XA },
    { std::string_view("S390"), instruction_set_version::ESA },
    { std::string_view("S390E"), instruction_set_version::ESA },
    { std::string_view("Z10"), instruction_set_version::Z10 },
    { std::string_view("Z11"), instruction_set_version::Z11 },
    { std::string_view("Z114"), instruction_set_version::Z11 },
    { std::string_view("Z12"), instruction_set_version::Z12 },
    { std::string_view("Z13"), instruction_set_version::Z13 },
    { std::string_view("Z14"), instruction_set_version::Z14 },
    { std::string_view("Z15"), instruction_set_version::Z15 },
    { std::string_view("Z16"), instruction_set_version::Z16 },
    { std::string_view("Z196"), instruction_set_version::Z11 },
    { std::string_view("Z800"), instruction_set_version::ZOP },
    { std::string_view("Z890"), instruction_set_version::YOP },
    { std::string_view("Z9"), instruction_set_version::Z9 },
    { std::string_view("Z900"), instruction_set_version::ZOP },
    { std::string_view("Z990"), instruction_set_version::YOP },
    { std::string_view("ZBC12"), instruction_set_version::Z12 },
    { std::string_view("ZEC12"), instruction_set_version::Z12 },
    { std::string_view("ZS"), instruction_set_version::ZOP },
    { std::string_view("ZS-1"), instruction_set_version::ZOP },
    { std::string_view("ZS-10"), instruction_set_version::Z16 },
    { std::string_view("ZS-2"), instruction_set_version::YOP },
    { std::string_view("ZS-3"), instruction_set_version::Z9 },
    { std::string_view("ZS-4"), instruction_set_version::Z10 },
    { std::string_view("ZS-5"), instruction_set_version::Z11 },
    { std::string_view("ZS-6"), instruction_set_version::Z12 },
    { std::string_view("ZS-7"), instruction_set_version::Z13 },
    { std::string_view("ZS-8"), instruction_set_version::Z14 },
    { std::string_view("ZS-9"), instruction_set_version::Z15 },
    { std::string_view("ZSERIES"), instruction_set_version::ZOP },
    { std::string_view("ZSERIES-1"), instruction_set_version::ZOP },
    { std::string_view("ZSERIES-10"), instruction_set_version::Z16 },
    { std::string_view("ZSERIES-2"), instruction_set_version::YOP },
    { std::string_view("ZSERIES-3"), instruction_set_version::Z9 },
    { std::string_view("ZSERIES-4"), instruction_set_version::Z10 },
    { std::string_view("ZSERIES-5"), instruction_set_version::Z11 },
    { std::string_view("ZSERIES-6"), instruction_set_version::Z12 },
    { std::string_view("ZSERIES-7"), instruction_set_version::Z13 },
    { std::string_view("ZSERIES-8"), instruction_set_version::Z14 },
    { std::string_view("ZSERIES-9"), instruction_set_version::Z15 },
} };

static_assert(std::ranges::is_sorted(instr_set_machine_equivalents, {}, &instr_set_equivalent_pair::first));

bool instr_set_equivalent_valid(
    std::string instr_set_name, std::span<const instr_set_equivalent_pair> equivalents) noexcept
{
    utils::to_upper(instr_set_name);

    return instr_set_name.size() == 0
        || std::ranges::find(equivalents, instr_set_name, utils::first_element) != equivalents.end();
}
} // namespace

bool assembler_options::valid() const noexcept
{
    if (sysparm.has_value() && sysparm.value().size() >= 256)
        return false;
    if (machine.has_value() && optable.has_value())
        return false;
    if (machine.has_value() && !instr_set_equivalent_valid(machine.value(), instr_set_machine_equivalents))
        return false;
    if (optable.has_value() && !instr_set_equivalent_valid(optable.value(), instr_set_optable_equivalents))
        return false;
    return true;
}

namespace {
std::optional<instruction_set_version> find_instruction_set(
    std::string instr_set_name, const std::span<const instr_set_equivalent_pair> equivalents)
{
    utils::to_upper(instr_set_name);

    auto it = std::ranges::lower_bound(equivalents, instr_set_name, {}, utils::first_element);

    if (it == std::end(equivalents) || it->first != instr_set_name)
        return std::nullopt;

    return it->second;
}
} // namespace

void assembler_options::apply_options_to(asm_option& opts) const
{
    if (sysparm.has_value())
        opts.sysparm = sysparm.value();
    if (profile.has_value())
        opts.profile = profile.value();
    if (system_id.has_value())
        opts.system_id = system_id.value();
    if (goff.has_value())
        opts.sysopt_xobject = goff.value();
    if (rent.has_value())
        opts.sysopt_rent = rent.value();
    if (machine.has_value() || optable.has_value()) // Only one of them should have value at this point
    {
        const auto& instr_set_name = machine.has_value() ? machine.value() : optable.value();
        const auto& instr_set_equivalents = machine.has_value()
            ? std::span<const instr_set_equivalent_pair> { instr_set_machine_equivalents }
            : std::span<const instr_set_equivalent_pair> { instr_set_optable_equivalents };

        if (auto translated = find_instruction_set(instr_set_name, instr_set_equivalents); translated.has_value())
        {
            opts.instr_set = translated.value();
        }
    }
}

bool assembler_options::has_value() const noexcept
{
    return sysparm.has_value() || profile.has_value() || machine.has_value() || optable.has_value()
        || system_id.has_value() || goff.has_value() || rent.has_value();
}

template<typename T>
void optional_to_json(nlohmann::json& j, std::string_view name, const std::optional<T>& o)
{
    if (o.has_value())
        j[name] = o.value();
}

template<typename T>
void optional_from_json(const nlohmann::json& j, std::string_view name, std::optional<T>& o)
{
    if (auto it = j.find(name); it != j.end())
    {
        if (o.has_value())
            throw nlohmann::json::other_error::create(
                501, std::string("Parameter ").append(name).append(" redefined"), &j);

        o = it->get<T>();
    }
}

void to_json(nlohmann::json& j, const assembler_options& p)
{
    j = nlohmann::json::object();
    optional_to_json(j, "GOFF", p.goff); // GOFF and XOBJECTS are synonyms
    optional_to_json(j, "MACHINE", p.machine);
    optional_to_json(j, "OPTABLE", p.optable);
    optional_to_json(j, "PROFILE", p.profile);
    optional_to_json(j, "RENT", p.rent);
    optional_to_json(j, "SYSPARM", p.sysparm);
    optional_to_json(j, "SYSTEM_ID", p.system_id);
}

void from_json(const nlohmann::json& j, assembler_options& p)
{
    if (!j.is_object())
        throw nlohmann::json::other_error::create(501, "asm_options must be an object.", &j);

    optional_from_json(j, "GOFF", p.goff);
    optional_from_json(j, "MACHINE", p.machine);
    optional_from_json(j, "OPTABLE", p.optable);
    optional_from_json(j, "PROFILE", p.profile);
    optional_from_json(j, "RENT", p.rent);
    optional_from_json(j, "SYSPARM", p.sysparm);
    optional_from_json(j, "SYSTEM_ID", p.system_id);
    optional_from_json(j, "XOBJECT", p.goff);
}
} // namespace hlasm_plugin::parser_library::config
