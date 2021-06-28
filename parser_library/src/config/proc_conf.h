/*
 * Copyright (c) 2021 Broadcom.
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

#ifndef HLASMPARSER_PARSERLIBRARY_CONFIG_PROC_CONF_H
#define HLASMPARSER_PARSERLIBRARY_CONFIG_PROC_CONF_H

#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "nlohmann/json_fwd.hpp"

namespace hlasm_plugin::parser_library::config {

struct library
{
    std::string path;
    std::vector<std::string> macro_extensions;
    bool optional = false;
};
void to_json(nlohmann::json& j, const library& p);
void from_json(const nlohmann::json& j, library& p);

struct assembler_options
{
    std::string sysparm;
    std::string profile;

    bool valid() const noexcept { return sysparm.size() < 256; }
};
void to_json(nlohmann::json& j, const assembler_options& p);
void from_json(const nlohmann::json& j, assembler_options& p);

struct db2_preprocessor
{};
inline bool operator==(const db2_preprocessor&, const db2_preprocessor&) { return true; }
inline bool operator!=(const db2_preprocessor& l, const db2_preprocessor& r) { return !(l == r); }

void to_json(nlohmann::json& j, const db2_preprocessor& p);
void from_json(const nlohmann::json& j, db2_preprocessor& p);

struct processor_group
{
    std::string name;
    std::vector<library> libs;
    assembler_options asm_options;
    std::variant<std::monostate, db2_preprocessor> preprocessor;
};
void to_json(nlohmann::json& j, const processor_group& p);
void from_json(const nlohmann::json& j, processor_group& p);

struct proc_conf
{
    std::vector<processor_group> pgroups;
    std::vector<std::string> macro_extensions;
};
void to_json(nlohmann::json& j, const proc_conf& p);
void from_json(const nlohmann::json& j, proc_conf& p);

} // namespace hlasm_plugin::parser_library::config

#endif // HLASMPARSER_PARSERLIBRARY_CONFIG_PROC_CONF_H
