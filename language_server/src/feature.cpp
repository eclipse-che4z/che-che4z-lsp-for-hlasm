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

#include "feature.h"

#include <format>

#include "nlohmann/json.hpp"

void nlohmann::adl_serializer<hlasm_plugin::language_server::request_id>::to_json(
    nlohmann::json& j, const hlasm_plugin::language_server::request_id& rid)
{
    if (std::holds_alternative<long>(rid.id))
        j = std::get<long>(rid.id);
    else
        j = std::get<std::string>(rid.id);
}

hlasm_plugin::language_server::request_id
nlohmann::adl_serializer<hlasm_plugin::language_server::request_id>::from_json(const nlohmann::json& j)
{
    if (j.is_number_integer())
        return hlasm_plugin::language_server::request_id(j.get<long>());
    else if (j.is_string())
        return hlasm_plugin::language_server::request_id(j.get<std::string>());
    else
        throw nlohmann::json::other_error::create(501, "Request id must be either integer or string", &j);
}

namespace hlasm_plugin::language_server {

void from_json(const nlohmann::json& j, std::optional<request_id>& rid)
{
    if (j.is_number_integer())
        rid.emplace(j.get<long>());
    else if (j.is_string())
        rid.emplace(j.get<std::string>());
    else
        rid.reset();
}

parser_library::range feature::parse_range(const nlohmann::json& range_json)
{
    return { parse_position(range_json.at("start")), parse_position(range_json.at("end")) };
}

parser_library::position feature::parse_position(const nlohmann::json& position_json)
{
    // TODO: rewrite message parsing
    return { (size_t)position_json.at("line").get<nlohmann::json::number_unsigned_t>(),
        (size_t)position_json.at("character").get<nlohmann::json::number_unsigned_t>() };
}

nlohmann::json feature::range_to_json(const parser_library::range& range)
{
    return nlohmann::json { { "start", position_to_json(range.start) }, { "end", position_to_json(range.end) } };
}

nlohmann::json feature::position_to_json(const parser_library::position& position)
{
    return nlohmann::json { { "line", position.line }, { "character", position.column } };
}

std::string request_id::to_string() const
{
    if (std::holds_alternative<long>(id))
        return std::format("({})", std::get<long>(id));
    else
        return std::format("\"{}\"", std::get<std::string>(id));
}

} // namespace hlasm_plugin::language_server
