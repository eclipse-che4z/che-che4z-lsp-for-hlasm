/*
 * Copyright (c) 2023 Broadcom.
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

#ifndef HLASMPLUGIN_PARSERLIBRARY_CONTEXT_SPECIAL_INSTRUCTIONS_H
#define HLASMPLUGIN_PARSERLIBRARY_CONTEXT_SPECIAL_INSTRUCTIONS_H

#include <algorithm>

#include "id_storage.h"

namespace hlasm_plugin::parser_library::context {
inline bool instruction_resolved_during_macro_parsing(id_index name)
{
    // List of instructions that are resolved during macro definition - therefore are affected by OPSYN
    static constexpr id_index cached_instr[] {
        id_storage::well_known::COPY,
        id_storage::well_known::ASPACE,
        id_storage::well_known::GBLA,
        id_storage::well_known::GBLB,
        id_storage::well_known::GBLC,
        id_storage::well_known::LCLA,
        id_storage::well_known::LCLB,
        id_storage::well_known::LCLC,
        id_storage::well_known::SETA,
        id_storage::well_known::SETB,
        id_storage::well_known::SETC,
        id_storage::well_known::MEND,
        id_storage::well_known::MACRO,
        id_storage::well_known::MEXIT,
        id_storage::well_known::AIF,
        id_storage::well_known::AREAD,
        id_storage::well_known::ACTR,
        id_storage::well_known::AGO,
        id_storage::well_known::ANOP,
    };

    return std::ranges::find(cached_instr, name) != std::end(cached_instr);
}

} // namespace hlasm_plugin::parser_library::context
#endif
