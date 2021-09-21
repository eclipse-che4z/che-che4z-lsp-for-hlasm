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

#include "processing_context.h"

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::context;

source_context::source_context(std::string source_name)
    : current_instruction(position(), std::move(source_name))
{}

source_snapshot source_context::create_snapshot() const
{
    std::vector<copy_frame> copy_frames;

    for (auto& member : copy_stack)
        copy_frames.emplace_back(member.name(), member.current_statement);

    if (!copy_frames.empty())
        --copy_frames.back().statement_offset;

    return source_snapshot { current_instruction, begin_index, end_index, end_line, std::move(copy_frames) };
}

processing_context::processing_context(processing::processing_kind proc_kind, bool owns_source)
    : proc_kind(proc_kind)
    , owns_source(owns_source)
{}

processing_frame::processing_frame(
    location proc_location, const code_scope& scope, file_processing_type proc_type, id_index member)
    : proc_location(std::move(proc_location))
    , scope(scope)
    , proc_type(std::move(proc_type))
    , member_name(member)
{}
