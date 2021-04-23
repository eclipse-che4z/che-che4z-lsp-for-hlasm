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

#include "id_storage.h"
#include "location.h"
#include "statement_cache.h"

namespace hlasm_plugin {
namespace parser_library {
namespace context {

struct copy_member;
using copy_member_ptr = std::shared_ptr<copy_member>;

// structure represents COPY member in HLASM macro library
struct copy_member
{
    // member idenifier
    const id_index name;
    // block of statements defining the member
    cached_block cached_definition;
    // location of the definition
    const location definition_location;

    copy_member(id_index name, statement_block definition, location definition_location)
        : name(name)
        , definition_location(std::move(definition_location))
    {
        
        for (auto&& stmt : definition)
            cached_definition.emplace_back(std::move(stmt));
    }

    //copy_member_invocation enter() { return copy_member_invocation(name, cached_definition, definition_location); }
};

// structure represents invocation of COPY member in HLASM macro library
struct copy_member_invocation
{
    const id_index name;
    cached_block& cached_definition;
    const location& definition_location;
    const copy_member_ptr copy_member_definition;
    int current_statement;

    copy_member_invocation(copy_member_ptr copy_member)
        : name(copy_member->name)
        , cached_definition(copy_member->cached_definition)
        , definition_location(copy_member->definition_location)
        , copy_member_definition(std::move(copy_member))
        , current_statement(-1)
    {}
};


} // namespace context
} // namespace parser_library
} // namespace hlasm_plugin
#endif
