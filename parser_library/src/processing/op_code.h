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

#include "context/id_storage.h"
#include "context/instruction_type.h"
#include "processing_format.h"

namespace hlasm_plugin::parser_library::processing {

// structure holding resolved operation code of the instruction (solving OPSYNs and so on)
struct op_code
{
    op_code()
        : value(context::id_storage::empty_id)
        , type(context::instruction_type::UNDEF)
    {}
    op_code(context::id_index value, context::instruction_type type)
        : value(value)
        , type(type)
    {}

    context::id_index value;
    context::instruction_type type;
};

using processing_status = std::pair<processing_format, op_code>;

class processing_status_cache_key
{
    processing_form form;
    operand_occurence occurence;
    bool is_alias;

public:
    friend bool operator==(processing_status_cache_key l, processing_status_cache_key r)
    {
        return l.form == r.form && l.occurence == r.occurence && l.is_alias == r.is_alias;
    }
    friend bool operator!=(processing_status_cache_key l, processing_status_cache_key r) { return !(l == r); }

    explicit processing_status_cache_key(const processing_status& s)
        : form(s.first.form)
        , occurence(s.first.occurence)
        , is_alias(s.second.type == context::instruction_type::ASM && s.second.value && *s.second.value == "ALIAS")
    {}
};

} // namespace hlasm_plugin::parser_library::processing
#endif
