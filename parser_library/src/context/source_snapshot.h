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

#ifndef CONTEXT_SOURCE_SNAPSHOT_H
#define CONTEXT_SOURCE_SNAPSHOT_H

#include <vector>

#include "id_index.h"
#include "location.h"
#include "statement_id.h"

namespace hlasm_plugin::parser_library::context {

// helper structure representing position in source file
struct source_position
{
    // line in the file
    size_t rewind_target = 0;

    source_position() = default;
    explicit source_position(size_t rewind_target) noexcept
        : rewind_target(rewind_target)
    {}

    bool operator==(const source_position& oth) const noexcept = default;
};

// helper structure representing a copy member invocation
struct copy_frame
{
    id_index copy_member;
    statement_id statement_offset;
    size_t suspended_at;
    size_t pending_resume;

    copy_frame(id_index copy_member, statement_id statement_offset, size_t suspended_at, size_t pending_resume)
        : copy_member(copy_member)
        , statement_offset(statement_offset)
        , suspended_at(suspended_at)
        , pending_resume(pending_resume)
    {}

    bool operator==(const copy_frame& oth) const noexcept = default;

    void adjust_to_beginning() noexcept
    {
        if (suspended_at == (size_t)-1)
            statement_offset.value -= 1;
        else
            pending_resume = (size_t)-1;
    }
};

// snapshot of a source_context structure
struct source_snapshot
{
    location instruction;
    size_t begin_index = 0;
    size_t end_index = 0;
    std::vector<copy_frame> copy_frames;

    source_snapshot() = default;

    source_snapshot(location instruction, size_t begin_index, size_t end_index, std::vector<copy_frame> copy_frames)
        : instruction(std::move(instruction))
        , begin_index(begin_index)
        , end_index(end_index)
        , copy_frames(std::move(copy_frames))
    {}

    bool operator==(const source_snapshot& oth) const
    {
        return begin_index == oth.begin_index && end_index == oth.end_index && copy_frames == oth.copy_frames;
    }
};

} // namespace hlasm_plugin::parser_library::context

#endif
