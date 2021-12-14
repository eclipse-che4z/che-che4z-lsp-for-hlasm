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
{};

struct cics_preprocessor_options
{};

using preprocessor_options = std::variant<std::monostate, db2_preprocessor_options, cics_preprocessor_options>;

} // namespace hlasm_plugin::parser_library
#endif
