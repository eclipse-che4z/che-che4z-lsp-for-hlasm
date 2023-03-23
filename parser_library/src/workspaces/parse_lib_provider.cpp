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

#include "parse_lib_provider.h"

#include <cassert>

namespace hlasm_plugin::parser_library::workspaces {


void empty_parse_lib_provider::parse_library(
    std::string_view, analyzing_context, library_data, std::function<void(bool)> callback)
{
    assert(callback);
    callback(false);
};
bool empty_parse_lib_provider::has_library(std::string_view, utils::resource::resource_location*) { return false; };
void empty_parse_lib_provider::get_library(std::string_view,
    std::function<void(std::optional<std::pair<std::string, utils::resource::resource_location>>)> callback)
{
    assert(callback);
    callback(std::nullopt);
}

empty_parse_lib_provider empty_parse_lib_provider::instance;

} // namespace hlasm_plugin::parser_library::workspaces
