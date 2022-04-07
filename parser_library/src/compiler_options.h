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

#ifndef HLASMPARSER_PARSERLIBRARY_COMPILER_OPTIONS_H
#define HLASMPARSER_PARSERLIBRARY_COMPILER_OPTIONS_H

#include <string>

// This file contains assembler compiler options definitions.

namespace hlasm_plugin::parser_library {
struct asm_option
{
    std::string sysparm;
    std::string profile;

    static const std::string system_id_default;
    std::string system_id = system_id_default;

    static constexpr unsigned int sysopt_rent_default = 0;
    unsigned int sysopt_rent = sysopt_rent_default;

    long long statement_count_limit = 10'000'000;
};
} // namespace hlasm_plugin::parser_library
#endif
