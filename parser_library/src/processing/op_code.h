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

#ifndef PROCESSING_OP_CODE_H
#define PROCESSING_OP_CODE_H

#include <utility>

#include "context/id_index.h"
#include "context/instruction_type.h"
#include "processing_format.h"

namespace hlasm_plugin::parser_library::context {
class macro_definition;
} // namespace hlasm_plugin::parser_library::context

namespace hlasm_plugin::parser_library::processing {

// structure holding resolved operation code of the instruction (solving OPSYNs and so on)
struct op_code
{
    op_code()
        : type(context::instruction_type::UNDEF)
    {}
    op_code(context::id_index value, context::instruction_type type, const context::macro_definition* mac_def)
        : value(value)
        , mac_def(mac_def)
        , type(type)
    {}

    context::id_index value;
    const context::macro_definition* mac_def = nullptr;
    context::instruction_type type;
};

using processing_status = std::pair<processing_format, op_code>;

class processing_status_cache_key
{
    processing_form form;
    operand_occurrence occurrence;
    unsigned char is_alias : 1, loctr_len : 7;
    unsigned char rel_addr;

    explicit processing_status_cache_key(
        const processing_status& s, std::pair<unsigned char, unsigned char> details) noexcept;

public:
    friend bool operator==(processing_status_cache_key l, processing_status_cache_key r) = default;

    explicit processing_status_cache_key(const processing_status& s) noexcept;

    static unsigned char generate_loctr_len(std::string_view id) noexcept;
};

} // namespace hlasm_plugin::parser_library::processing
#endif
