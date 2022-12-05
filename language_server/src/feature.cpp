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

namespace hlasm_plugin::language_server {

parser_library::range feature::parse_range(const json& range_json)
{
    return { parse_position(range_json["start"]), parse_position(range_json["end"]) };
}

parser_library::position feature::parse_position(const json& position_json)
{
    // TODO: rewrite message parsing
    return { (size_t)position_json["line"].get<nlohmann::json::number_unsigned_t>(),
        (size_t)position_json["character"].get<nlohmann::json::number_unsigned_t>() };
}

json feature::range_to_json(const parser_library::range& range)
{
    return json { { "start", position_to_json(range.start) }, { "end", position_to_json(range.end) } };
}

json feature::position_to_json(const parser_library::position& position)
{
    return json { { "line", position.line }, { "character", position.column } };
}

} // namespace hlasm_plugin::language_server
