/*
 * Copyright (c) 2023 Broadcom.
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

#ifndef HLASMPLUGIN_PARSERLIBRARY_WORKSPACES_CONFIGURATION_DATATYPES
#define HLASMPLUGIN_PARSERLIBRARY_WORKSPACES_CONFIGURATION_DATATYPES

#include <compare>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>

#include "config/assembler_options.h"
#include "tagged_index.h"
#include "utils/resource_location.h"
#include "workspaces/processor_group.h"

namespace hlasm_plugin::parser_library::workspaces {
using program_id = utils::resource::resource_location;

struct basic_conf
{
    std::string name;

    auto operator<=>(const basic_conf&) const = default;

    size_t hash() const noexcept { return std::hash<std::string_view>()(name); }
};

struct b4g_conf
{
    std::string name;
    utils::resource::resource_location bridge_json_uri;

    auto operator<=>(const b4g_conf&) const = default;

    size_t hash() const noexcept
    {
        return std::hash<std::string_view>()(name) ^ std::hash<utils::resource::resource_location>()(bridge_json_uri);
    }
};

struct external_conf
{
    std::shared_ptr<const std::string> definition;

    bool operator==(const external_conf& o) const noexcept { return *definition == *o.definition; }
    auto operator<=>(const external_conf& o) const noexcept { return *definition <=> *o.definition; }

    bool operator==(std::string_view o) const noexcept { return *definition == o; }
    auto operator<=>(std::string_view o) const noexcept { return *definition <=> o; }

    size_t hash() const noexcept { return std::hash<std::string_view>()(*definition); }
};

using proc_grp_id = std::variant<basic_conf, b4g_conf, external_conf>;

template<typename T>
struct tagged_string_view
{
    std::string_view value;
};

struct proc_grp_id_hasher
{
    using is_transparent = void;

    size_t operator()(const proc_grp_id& pgid) const noexcept
    {
        return std::visit([](const auto& x) { return x.hash(); }, pgid);
    }

    size_t operator()(const tagged_string_view<external_conf>& external_conf_candidate) const noexcept
    {
        return std::hash<std::string_view>()(external_conf_candidate.value);
    }
};
struct proc_grp_id_equal
{
    using is_transparent = void;

    bool operator()(const proc_grp_id& l, const proc_grp_id& r) const noexcept { return l == r; }
    bool operator()(const proc_grp_id& l, const tagged_string_view<external_conf>& r) const noexcept
    {
        return std::holds_alternative<external_conf>(l) && *std::get<external_conf>(l).definition == r.value;
    }
    bool operator()(const tagged_string_view<external_conf>& l, const proc_grp_id& r) const noexcept
    {
        return std::holds_alternative<external_conf>(r) && l.value == *std::get<external_conf>(r).definition;
    }
};

using proc_groups_map = std::unordered_map<proc_grp_id,
    std::pair<processor_group, index_t<processor_group, unsigned long long>>,
    proc_grp_id_hasher,
    proc_grp_id_equal>;

// represents pair program => processor group - saves
// information that a program uses certain processor group
struct program
{
    program(program_id prog_id, proc_grp_id pgroup, config::assembler_options asm_opts, bool external)
        : prog_id(std::move(prog_id))
        , pgroup(std::move(pgroup))
        , asm_opts(std::move(asm_opts))
        , external(external)
    {}

    program_id prog_id;
    proc_grp_id pgroup;
    config::assembler_options asm_opts;
    bool external;
};

} // namespace hlasm_plugin::parser_library::workspaces

#endif
