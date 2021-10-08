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

const std::string id_storage::empty_string_("");

const id_storage::const_pointer id_storage::empty_id = &id_storage::empty_string_;

hlasm_plugin::parser_library::context::id_storage::id_storage()
    : well_known(lit_)
{}

size_t id_storage::size() const { return lit_.size(); }

id_storage::const_iterator id_storage::begin() const { return lit_.begin(); }

id_storage::const_iterator id_storage::end() const { return lit_.end(); }

bool id_storage::empty() const { return lit_.empty(); }

id_storage::const_pointer id_storage::find(std::string val) const
{
    to_upper(val);

    if (val.empty())
        return empty_id;

    const_iterator tmp = lit_.find(val);

    return tmp == lit_.end() ? nullptr : &*tmp;
}

id_storage::const_pointer id_storage::add(std::string value, bool is_uri)
{
    if (value.empty())
        return empty_id;
    if (!is_uri)
        to_upper(value);
    return &*lit_.insert(std::move(value)).first;
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
    , AGO(&*ptr.emplace("AGO").first)
    , ACTR(&*ptr.emplace("ACTR").first)
    , AREAD(&*ptr.emplace("AREAD").first)
    , ALIAS(&*ptr.emplace("ALIAS").first)
{}
