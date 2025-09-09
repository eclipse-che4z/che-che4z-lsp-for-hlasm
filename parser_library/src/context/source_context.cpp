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

#include "source_context.h"

#include <cassert>
#include <memory>

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::context;

source_context::source_context(utils::resource::resource_location source_loc, processing::processing_kind initial)
    : current_instruction(position(), std::move(source_loc))
    , proc_stack(1, initial)
{}

source_snapshot source_context::create_snapshot() const
{
    std::vector<copy_frame> copy_frames;

    for (auto& member : copy_stack)
        copy_frames.emplace_back(member.name(), member.current_statement);

    return source_snapshot { current_instruction, begin_index, end_index, std::move(copy_frames) };
}

processing_frame_details::processing_frame_details(const processing_frame& frame, const code_scope& scope)
    : pos(frame.pos)
    , resource_loc(frame.resource_loc)
    , scope_ptr(&scope)
    , member_name(frame.member_name)
    , proc_type(frame.proc_type)
{}

processing_frame_tree::processing_frame_tree()
    : m_root(std::to_address(m_frames.emplace().first))
{}

processing_frame_tree::node_pointer processing_frame_tree::step(node_pointer current,
    position pos,
    const utils::resource::resource_location& resource_loc,
    id_index member,
    file_processing_type proc_type)
{
    assert(current.m_node);

    const auto [it, _] = m_frames.insert({ current.m_node, pos, resource_loc, member, proc_type });
    return node_pointer(std::to_address(it));
}


std::vector<processing_frame> processing_frame_tree::node_pointer::to_vector() const
{
    std::vector<processing_frame> result;
    to_vector(result);
    return result;
}
void processing_frame_tree::node_pointer::to_vector(std::vector<processing_frame>& result) const
{
    result.clear();

    for (auto it = *this; !it.empty(); it = it.parent())
        result.emplace_back(it.frame());

    std::ranges::reverse(result);
}
