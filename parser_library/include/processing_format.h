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

#ifndef PROCESSING_PROCESSING_FORMAT_H
#define PROCESSING_PROCESSING_FORMAT_H

#include <stdint.h>

namespace hlasm_plugin::parser_library::processing {

enum class processing_kind : uint8_t
{
    ORDINARY,
    LOOKAHEAD,
    MACRO,
    COPY
};

enum class processing_form : uint8_t
{
    UNKNOWN,
    MACH,
    ASM_GENERIC_ORD,
    ASM_GENERIC_TEXT,
    ASM_ALIAS,
    ASM_END,
    ASM_USING,
    MAC,
    CA_GENERIC,
    CA_VARDEF,
    CA_BRANCH,
    DAT,
    IGNORED,
    DEFERRED,
};

enum class operand_occurrence : uint8_t
{
    PRESENT,
    ABSENT
};

// structure respresenting in which fashion should be statement processed
struct processing_format
{
    constexpr processing_format(processing_kind kind,
        processing_form form,
        operand_occurrence occurrence = operand_occurrence::PRESENT) noexcept
        : kind(kind)
        , form(form)
        , occurrence(occurrence)
    {}

    bool operator==(const processing_format& oth) const noexcept = default;

    processing_kind kind;
    processing_form form;
    operand_occurrence occurrence;
};

} // namespace hlasm_plugin::parser_library::processing
#endif
