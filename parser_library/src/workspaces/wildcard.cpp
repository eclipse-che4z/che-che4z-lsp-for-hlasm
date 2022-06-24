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

#include "wildcard.h"

#include "utils/platform.h"

namespace hlasm_plugin::parser_library::workspaces {
namespace {
// used for wildcard to regex conversions
const std::regex escape("(\\(|\\[|\\{|\\\\|\\^|\\-|\\=|\\$|\\!|\\||\\]|\\}|\\)|\\.)");
const std::regex question("\\?");
const std::regex nongreedy("(\\*|\\+)");
const std::regex slash("\\\\");
} // namespace

std::regex wildcard2regex(std::string wildcard)
{
    // change of double backslash to forward slash
    wildcard = std::regex_replace(wildcard, slash, "/");
    wildcard = std::regex_replace(wildcard, escape, "\\$1");
    wildcard = std::regex_replace(wildcard, question, ".");
    wildcard = std::regex_replace(wildcard, nongreedy, ".$1?");
    return std::regex(wildcard);
}

} // namespace hlasm_plugin::parser_library::workspaces