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

#ifndef HLASMPARSER_PARSERLIBRARY_PREPROCESSOR_OPTIONS_H
#define HLASMPARSER_PARSERLIBRARY_PREPROCESSOR_OPTIONS_H

#include <variant>

// This file contains assembler compiler options definitions.

namespace hlasm_plugin::parser_library {

struct db2_preprocessor_options
{
    std::string version;

    explicit db2_preprocessor_options(std::string version = {})
        : version(std::move(version))
    {}

    bool operator==(const db2_preprocessor_options&) const = default;
};

struct cics_preprocessor_options
{
    bool prolog;
    bool epilog;
    bool leasm;

    explicit cics_preprocessor_options(bool prolog = true, bool epilog = true, bool leasm = false)
        : prolog(prolog)
        , epilog(epilog)
        , leasm(leasm)
    {}

    bool operator==(const cics_preprocessor_options&) const = default;
};

using preprocessor_options = std::variant<std::monostate, db2_preprocessor_options, cics_preprocessor_options>;

} // namespace hlasm_plugin::parser_library
#endif
