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
#include <cassert>

namespace hlasm_plugin::parser_library::workspace {

std::regex wildcard2regex(const std::string& wildcard)
{
    auto regex_str = wildcard;
#ifdef _WIN32
    // change of forward slash to double backslash on windows
    regex_str = std::regex_replace(regex_str, slash, "\\");
#endif
    regex_str = std::regex_replace(regex_str, escape, "\\$1");
    regex_str = std::regex_replace(regex_str, question, ".");
    regex_str = std::regex_replace(regex_str, nongreedy, ".$1?");
    return	std::regex(regex_str);
}

}