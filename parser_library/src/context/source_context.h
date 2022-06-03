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

#ifndef CONTEXT_SOURCE_CONTEXT_H
#define CONTEXT_SOURCE_CONTEXT_H

#include "copy_member.h"
#include "processing/processing_format.h"
#include "source_snapshot.h"

namespace hlasm_plugin::parser_library::context {

// structure holding information about currently processed source
struct source_context
{
    location current_instruction;

    // location in the file
    size_t begin_index = 0;
    size_t end_index = 0;
    size_t end_line = 0;

    // stack of copy nests
    std::vector<copy_member_invocation> copy_stack;
    // stack of processing states
    std::vector<processing::processing_kind> proc_stack;

    source_context(utils::resource::resource_location source_loc, processing::processing_kind initial);

    source_snapshot create_snapshot() const;
};

enum class file_processing_type
{
    OPENCODE,
    COPY,
    MACRO
};

struct code_scope;

struct processing_frame
{
    processing_frame(location proc_location, const code_scope& scope, file_processing_type proc_type, id_index member);

    location proc_location;
    const code_scope& scope;
    file_processing_type proc_type;
    id_index member_name;
};

using processing_stack_t = std::vector<processing_frame>;

} // namespace hlasm_plugin::parser_library::context
#endif
