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

    const_pointer add(std::string value);

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
        const std::string* MHELP;
        const std::string* ASPACE;
        const std::string* AIF;
        const std::string* AGO;
        const std::string* ACTR;
        const std::string* AREAD;
        const std::string* ALIAS;
        const std::string* END;
        well_known_strings(std::unordered_set<std::string>& ptr);

    } const well_known;
};

using id_index = id_storage::const_pointer;

} // namespace hlasm_plugin::parser_library::context


#endif
