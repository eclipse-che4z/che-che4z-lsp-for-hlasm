/*
 * Copyright (c) 2022 Broadcom.
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

#ifndef CONTEXT_OPCODE_GENERATION_H
#define CONTEXT_OPCODE_GENERATION_H

#include <compare>
#include <cstddef>

namespace hlasm_plugin::parser_library::context {

class opcode_generation
{
    friend class hlasm_context;

    size_t gen;

    explicit constexpr opcode_generation(size_t g)
        : gen(g)
    {}

    opcode_generation& operator++()
    {
        ++gen;
        return *this;
    }

    opcode_generation operator++(int)
    {
        opcode_generation result = *this;
        ++gen;
        return result;
    }

public:
    auto operator<=>(const opcode_generation&) const = default;

    static const opcode_generation current;
    static const opcode_generation zero;
};
inline const opcode_generation opcode_generation::current((size_t)-1);
inline const opcode_generation opcode_generation::zero(0);

} // namespace hlasm_plugin::parser_library::context

#endif
