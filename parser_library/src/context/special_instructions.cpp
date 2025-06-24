/*
 * Copyright (c) 2025 Broadcom.
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

#include "special_instructions.h"

#include <algorithm>

#include "id_index.h"
#include "well_known.h"

namespace hlasm_plugin::parser_library::context {
bool instruction_resolved_during_macro_parsing(id_index name)
{
    // List of instructions that are resolved during macro definition - therefore are affected by OPSYN
    static constexpr id_index cached_instr[] {
        well_known::COPY,
        well_known::ASPACE,
        well_known::GBLA,
        well_known::GBLB,
        well_known::GBLC,
        well_known::LCLA,
        well_known::LCLB,
        well_known::LCLC,
        well_known::SETA,
        well_known::SETB,
        well_known::SETC,
        well_known::MEND,
        well_known::MACRO,
        well_known::MEXIT,
        well_known::AIF,
        well_known::AREAD,
        well_known::ACTR,
        well_known::AGO,
        well_known::ANOP,
    };

    return std::ranges::find(cached_instr, name) != std::end(cached_instr);
}

} // namespace hlasm_plugin::parser_library::context
