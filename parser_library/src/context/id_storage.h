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

#ifndef CONTEXT_LITERAL_STORAGE_H
#define CONTEXT_LITERAL_STORAGE_H

#include <string>
#include <unordered_set>

namespace hlasm_plugin::parser_library::context {


// storage for identifiers
// changes strings of identifiers to indexes of this storage class for easier and unified work
class id_storage
{
private:
    std::unordered_set<std::string> lit_;
    static const std::string empty_string_;

public:
    id_storage();
    using const_pointer = const std::string*;
    using const_iterator = typename std::unordered_set<std::string>::const_iterator;

    // represents value of empty identifier
    static const const_pointer empty_id;

    size_t size() const;
    const_iterator begin() const;
    const_iterator end() const;
    bool empty() const;

    const_pointer find(std::string val) const;

    const_pointer add(std::string value, bool is_uri = false);

    struct well_known_strings
    {
        const std::string* COPY;
        const std::string* SETA;
        const std::string* SETB;
        const std::string* SETC;
        const std::string* GBLA;
        const std::string* GBLB;
        const std::string* GBLC;
        const std::string* LCLA;
        const std::string* LCLB;
        const std::string* LCLC;
        const std::string* MACRO;
        const std::string* MEND;
        const std::string* MEXIT;
        const std::string* ASPACE;
        const std::string* AIF;
        const std::string* AGO;
        const std::string* ACTR;
        const std::string* AREAD;
        const std::string* empty;
        well_known_strings(std::unordered_set<std::string>& ptr);

    } const well_known;
};

using id_index = id_storage::const_pointer;

class id_set : std::vector<id_index>
{
    using base = std::vector<id_index>;

public:
    void emplace(id_index id)
    {
        auto i = std::lower_bound(base::begin(), base::end(), id);
        if (i == base::end() || *i != id)
            insert(i, id);
    }

    auto begin() const { return base::begin(); }
    auto end() const { return base::end(); }
    auto find(id_index id) const { return std::lower_bound(begin(), end(), id); }

    using base::empty;
    using base::size;

    using base::clear;

    void erase(id_index id)
    {
        auto i = std::lower_bound(base::begin(), base::end(), id);
        if (i != base::end() && *i == id)
            base::erase(i);
    }
};

} // namespace hlasm_plugin::parser_library::context


#endif
