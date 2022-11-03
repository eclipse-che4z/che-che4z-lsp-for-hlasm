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

#include "id_storage.h"

#include "common_types.h"

using namespace hlasm_plugin::parser_library::context;

hlasm_plugin::parser_library::context::id_storage::id_storage()
    : well_known(lit_)
{}

size_t id_storage::size() const { return lit_.size(); }

bool id_storage::empty() const { return lit_.empty(); }

std::optional<id_index> id_storage::find(std::string val) const
{
    if (val.empty())
        return id_index();

    to_upper(val);

    if (auto tmp = lit_.find(val); tmp != lit_.end())
        return id_index(&*tmp);
    else
        return std::nullopt;
}

id_index id_storage::add(std::string value)
{
    if (value.empty())
        return id_index();
    to_upper(value);
    return id_index(&*lit_.insert(std::move(value)).first);
}

id_storage::well_known_strings::well_known_strings(std::unordered_set<std::string>& ptr)
    : COPY(&*ptr.emplace("COPY").first)
    , SETA(&*ptr.emplace("SETA").first)
    , SETB(&*ptr.emplace("SETB").first)
    , SETC(&*ptr.emplace("SETC").first)
    , GBLA(&*ptr.emplace("GBLA").first)
    , GBLB(&*ptr.emplace("GBLB").first)
    , GBLC(&*ptr.emplace("GBLC").first)
    , LCLA(&*ptr.emplace("LCLA").first)
    , LCLB(&*ptr.emplace("LCLB").first)
    , LCLC(&*ptr.emplace("LCLC").first)
    , MACRO(&*ptr.emplace("MACRO").first)
    , MEND(&*ptr.emplace("MEND").first)
    , MEXIT(&*ptr.emplace("MEXIT").first)
    , MHELP(&*ptr.emplace("MHELP").first)
    , ASPACE(&*ptr.emplace("ASPACE").first)
    , AIF(&*ptr.emplace("AIF").first)
    , AIFB(&*ptr.emplace("AIFB").first)
    , AGO(&*ptr.emplace("AGO").first)
    , AGOB(&*ptr.emplace("AGOB").first)
    , ACTR(&*ptr.emplace("ACTR").first)
    , AREAD(&*ptr.emplace("AREAD").first)
    , ALIAS(&*ptr.emplace("ALIAS").first)
    , END(&*ptr.emplace("END").first)
    , SYSLIST(&*ptr.emplace("SYSLIST").first)
{}
