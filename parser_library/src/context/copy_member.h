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

#ifndef CONTEXT_COPY_MEMBER_H
#define CONTEXT_COPY_MEMBER_H

#include "id_index.h"
#include "location.h"
#include "statement_cache.h"
#include "statement_id.h"

namespace hlasm_plugin::parser_library::context {

struct copy_member;

// structure represents COPY member in HLASM macro library
struct copy_member
{
    // member identifier
    const id_index name;
    // block of statements defining the member
    cached_block cached_definition;
    // location of the definition
    const location definition_location;

    copy_member(id_index name, statement_block definition, location definition_location)
        : name(name)
        , cached_definition(std::make_move_iterator(definition.begin()), std::make_move_iterator(definition.end()))
        , definition_location(std::move(definition_location))
    {}
};

using copy_member_ptr = std::shared_ptr<copy_member>;

// structure represents invocation of COPY member in HLASM macro library
struct copy_member_invocation
{
    copy_member_ptr copy_member_definition;
    statement_id current_statement;

    static constexpr size_t not_suspended = (size_t)-1;
    size_t suspended_at = not_suspended;

    explicit copy_member_invocation(copy_member_ptr copy_member)
        : copy_member_definition(std::move(copy_member))
    {}

    id_index name() const { return copy_member_definition->name; }
    cached_block* cached_definition() const { return &copy_member_definition->cached_definition; }
    const location* definition_location() const { return &copy_member_definition->definition_location; }

    position current_statement_position() const
    {
        if (current_statement != statement_id())
            return cached_definition()->at(current_statement.value).get_base()->statement_position();
        else
            return {};
    }

    bool suspended() const { return suspended_at != not_suspended; }
    void resume() { suspended_at = not_suspended; }
    void suspend(size_t line) { suspended_at = line; }
};

} // namespace hlasm_plugin::parser_library::context
#endif
