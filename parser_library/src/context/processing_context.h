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

#ifndef CONTEXT_PROCESSING_CONTEXT_H
#define CONTEXT_PROCESSING_CONTEXT_H

#include "copy_member.h"
#include "processing/processing_format.h"
#include "source_snapshot.h"

namespace hlasm_plugin {
namespace parser_library {
namespace context {

// structure holding information about currently processed source
struct source_context
{
    location current_instruction;

    // location in the file
    size_t begin_index;
    size_t end_index;
    size_t end_line;

    // stack of copy nests
    std::vector<copy_member_invocation> copy_stack;

    source_context(std::string source_name);

    source_snapshot create_snapshot() const;
};

// structure holding information about current processing kind
struct processing_context
{
    // current processing kind
    processing::processing_kind proc_kind;

    // flag whether this context has owner relation to the current source
    bool owns_source;

    processing_context(processing::processing_kind proc_kind, bool owns_source);
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
    processing_frame(location proc_location, const code_scope& scope, file_processing_type proc_type);

    location proc_location;
    const code_scope& scope;
    file_processing_type proc_type;
};

using processing_stack_t = std::vector<processing_frame>;

} // namespace context
} // namespace parser_library
} // namespace hlasm_plugin
#endif
