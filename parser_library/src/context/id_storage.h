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

#include <compare>
#include <optional>
#include <string>
#include <unordered_set>

namespace hlasm_plugin::parser_library::context {


class id_index
{
    const std::string* m_value = nullptr;

    explicit constexpr id_index(const std::string* value) noexcept
        : m_value(value)
    {}

    friend class id_storage;
    friend class literal_pool;

public:
    constexpr id_index() noexcept = default;

    constexpr auto operator<=>(const id_index&) const noexcept = default;

    std::string_view to_string_view() const noexcept { return m_value ? *m_value : std::string_view(); }
    std::string to_string() const { return m_value ? *m_value : std::string(); }

    constexpr bool empty() const noexcept { return m_value == nullptr; }

    auto hash() const noexcept { return std::hash<const std::string*>()(m_value); }
};
} // namespace hlasm_plugin::parser_library::context

template<>
struct std::hash<hlasm_plugin::parser_library::context::id_index>
{
    std::size_t operator()(const hlasm_plugin::parser_library::context::id_index& id) const noexcept
    {
        return id.hash();
    }
};

namespace hlasm_plugin::parser_library::context {
// storage for identifiers
// changes strings of identifiers to indexes of this storage class for easier and unified work
class id_storage
{
    std::unordered_set<std::string> lit_;

public:
    id_storage();

    size_t size() const;
    bool empty() const;

    std::optional<id_index> find(std::string val) const;

    id_index add(std::string value);

    struct well_known_strings
    {
        id_index COPY;
        id_index SETA;
        id_index SETB;
        id_index SETC;
        id_index GBLA;
        id_index GBLB;
        id_index GBLC;
        id_index LCLA;
        id_index LCLB;
        id_index LCLC;
        id_index MACRO;
        id_index MEND;
        id_index MEXIT;
        id_index MHELP;
        id_index ASPACE;
        id_index AIF;
        id_index AIFB;
        id_index AGO;
        id_index AGOB;
        id_index ACTR;
        id_index AREAD;
        id_index ALIAS;
        id_index END;
        id_index SYSLIST;
        well_known_strings(std::unordered_set<std::string>& ptr);

    } const well_known;
};

} // namespace hlasm_plugin::parser_library::context

#endif
