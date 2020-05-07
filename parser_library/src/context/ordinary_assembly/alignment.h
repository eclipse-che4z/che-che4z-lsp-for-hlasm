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

#ifndef CONTEXT_ALIGNMENT_H
#define CONTEXT_ALIGNMENT_H

#include <cstddef>

namespace hlasm_plugin::parser_library::context {

// structure representing required alignment of storage area
// boundary represents actual alignment
// byte is offset that is added to aligned location
struct alignment
{
    size_t byte;
    size_t boundary;

    bool operator==(const alignment& oth) const { return boundary == oth.boundary && byte == oth.byte; }
};

// enumeration of common alignments
static constexpr alignment no_align { 0, 1 };
static constexpr alignment halfword { 0, 2 };
static constexpr alignment fullword { 0, 4 };
static constexpr alignment doubleword { 0, 8 };
static constexpr alignment quadword { 0, 16 };

} // namespace hlasm_plugin::parser_library::context

#endif
