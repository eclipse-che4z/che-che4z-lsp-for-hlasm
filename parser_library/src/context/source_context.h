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

#include <memory>
#include <unordered_set>
#include <vector>

#include "copy_member.h"
#include "processing_format.h"
#include "source_snapshot.h"
#include "utils/general_hashers.h"

namespace hlasm_plugin::parser_library::context {

// structure holding information about currently processed source
struct source_context
{
    location current_instruction;

    // location in the file
    size_t begin_index = 0;
    size_t end_index = 0;

    // stack of copy nests
    std::vector<copy_member_invocation> copy_stack;
    // stack of processing states
    std::vector<processing::processing_kind> proc_stack;

    source_context(utils::resource::resource_location source_loc, processing::processing_kind initial);

    source_snapshot create_snapshot() const;
};

enum class file_processing_type : unsigned char
{
    NONE,
    OPENCODE,
    COPY,
    MACRO
};

struct code_scope;

struct processing_frame
{
    processing_frame(position pos,
        const utils::resource::resource_location& resource_loc,
        id_index member,
        file_processing_type proc_type)
        : pos(pos)
        , resource_loc(resource_loc)
        , member_name(member)
        , proc_type(proc_type)
    {}

    position pos;
    utils::resource::resource_location resource_loc;
    id_index member_name;
    file_processing_type proc_type;

    bool operator==(const processing_frame&) const = default;

    location get_location() const { return location(pos, resource_loc); }
};

struct processing_frame_details
{
    processing_frame_details(position pos,
        const utils::resource::resource_location& resource_loc,
        const code_scope& scope,
        file_processing_type proc_type,
        id_index member);

    position pos;
    utils::resource::resource_location resource_loc;
    const code_scope& scope;
    id_index member_name;
    file_processing_type proc_type;
};

using processing_stack_details_t = std::vector<processing_frame_details>;

class processing_frame_tree
{
    struct processing_frame_node
    {
        const processing_frame_node* m_parent;

        processing_frame frame;

        bool operator==(const processing_frame_node&) const = default;

        processing_frame_node(const processing_frame_node* parent,
            position pos,
            const utils::resource::resource_location& resource_loc,
            id_index member,
            file_processing_type proc_type)
            : m_parent(parent)
            , frame(pos, resource_loc, member, proc_type)
        {}
        explicit processing_frame_node()
            : m_parent(nullptr)
            , frame({}, utils::resource::resource_location(), id_index(), file_processing_type::NONE)
        {}
    };

    struct hasher
    {
        size_t operator()(const processing_frame_node& n) const
        {
            constexpr auto hash = []<typename... T>(const T&... v) {
                size_t result = 0;
                ((result = utils::hashers::hash_combine(result, std::hash<T>()(v))), ...);
                return result;
            };
            return hash(n.m_parent, n.frame.member_name, n.frame.resource_loc, n.frame.pos.column, n.frame.pos.line);
        }
    };

    std::unordered_set<processing_frame_node, hasher> m_frames;
    const processing_frame_node* m_root;

public:
    class node_pointer
    {
        const processing_frame_node* m_node;

        explicit node_pointer(const processing_frame_node* node)
            : m_node(node)
        {}

    public:
        node_pointer()
            : m_node(nullptr) {};

        const processing_frame& frame() const { return m_node->frame; }
        node_pointer parent() const { return node_pointer(m_node->m_parent); }

        std::vector<processing_frame> to_vector() const;
        void to_vector(std::vector<processing_frame>&) const;

        bool operator==(node_pointer it) const noexcept { return m_node == it.m_node; }

        bool empty() const noexcept { return !m_node || m_node->m_parent == nullptr; }

        friend class processing_frame_tree;
    };

    node_pointer root() const { return node_pointer(m_root); }

    processing_frame_tree();

    node_pointer step(node_pointer current,
        position pos,
        const utils::resource::resource_location& resource_loc,
        id_index member,
        file_processing_type proc_type);
};

using processing_stack_t = processing_frame_tree::node_pointer;

} // namespace hlasm_plugin::parser_library::context
#endif
