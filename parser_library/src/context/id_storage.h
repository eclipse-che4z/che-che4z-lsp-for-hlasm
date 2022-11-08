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

#include <algorithm>
#include <array>
#include <cassert>
#include <compare>
#include <cstring>
#include <optional>
#include <string>
#include <unordered_set>

namespace hlasm_plugin::parser_library::context {


class id_index
{
    static constexpr size_t buffer_size = sizeof(const std::string*) < 8 ? 16 : 2 * sizeof(const std::string*);
    alignas(const std::string*) unsigned char m_buffer[buffer_size] = {}; // check CWG2489

    explicit id_index(const std::string* value) noexcept
    {
        new (m_buffer) const std::string*(value);
        m_buffer[buffer_size - 1] = 0x80u;
    }

    explicit constexpr id_index(std::string_view s) noexcept
    {
        assert(s.size() < buffer_size);
        if (std::is_constant_evaluated())
            std::copy(s.begin(), s.end(), m_buffer);
        else
            std::memcpy(m_buffer, s.data(), s.size());
        m_buffer[buffer_size - 1] = (char)s.size();
    }

    friend class id_storage;
    friend class literal_pool;

public:
    constexpr id_index() noexcept = default;
    template<size_t n>
    explicit consteval id_index(const char (&s)[n]) requires(n <= buffer_size)
        : id_index(std::string_view(s, n - 1))
    {
        assert(s[n - 1] == 0
            && std::string_view(s, n - 1).find_first_of("abcdefghijklmnopqrstuvwxyz") == std::string_view::npos);
    }

    constexpr auto operator<=>(const id_index& o) const noexcept
    {
        // compilers seem still a bit weirded out by the "= default";
        if (std::is_constant_evaluated())
        {
            for (size_t i = 0; i < buffer_size; ++i)
            {
                if (auto r = m_buffer[i] <=> o.m_buffer[i]; r != std::strong_ordering::equal)
                    return r;
            }
            return std::strong_ordering::equal;
        }
        else
        {
            auto r = std::memcmp(m_buffer, o.m_buffer, buffer_size);
            if (r == 0)
                return std::strong_ordering::equal;
            else if (r < 0)
                return std::strong_ordering::less;
            else
                return std::strong_ordering::greater;
        }
    }

    constexpr bool operator==(const id_index& o) const noexcept
    {
        return std::equal(std::begin(m_buffer), std::end(m_buffer), std::begin(o.m_buffer), std::end(o.m_buffer));
    }

    std::string_view to_string_view() const noexcept
    {
        return (m_buffer[buffer_size - 1] & 0x80u)
            ? *reinterpret_cast<const std::string* const&>(m_buffer)
            : std::string_view(reinterpret_cast<const char*>(m_buffer), m_buffer[buffer_size - 1]);
    }
    std::string to_string() const { return std::string(to_string_view()); }

    constexpr bool empty() const noexcept { return m_buffer[buffer_size - 1] == 0; }

    auto hash() const noexcept
    {
        return std::hash<std::string_view>()(std::string_view(reinterpret_cast<const char*>(m_buffer), buffer_size));
    }
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

    static id_index small_id(std::string_view value);

public:
    size_t size() const;
    bool empty() const;

    std::optional<id_index> find(std::string_view val) const;

    id_index add(std::string_view value);
    id_index add(std::string&& value);

    struct well_known
    {
        static constexpr id_index COPY = id_index("COPY");
        static constexpr id_index SETA = id_index("SETA");
        static constexpr id_index SETB = id_index("SETB");
        static constexpr id_index SETC = id_index("SETC");
        static constexpr id_index GBLA = id_index("GBLA");
        static constexpr id_index GBLB = id_index("GBLB");
        static constexpr id_index GBLC = id_index("GBLC");
        static constexpr id_index LCLA = id_index("LCLA");
        static constexpr id_index LCLB = id_index("LCLB");
        static constexpr id_index LCLC = id_index("LCLC");
        static constexpr id_index MACRO = id_index("MACRO");
        static constexpr id_index MEND = id_index("MEND");
        static constexpr id_index MEXIT = id_index("MEXIT");
        static constexpr id_index MHELP = id_index("MHELP");
        static constexpr id_index ASPACE = id_index("ASPACE");
        static constexpr id_index AIF = id_index("AIF");
        static constexpr id_index AIFB = id_index("AIFB");
        static constexpr id_index AGO = id_index("AGO");
        static constexpr id_index AGOB = id_index("AGOB");
        static constexpr id_index ACTR = id_index("ACTR");
        static constexpr id_index AREAD = id_index("AREAD");
        static constexpr id_index ALIAS = id_index("ALIAS");
        static constexpr id_index END = id_index("END");
        static constexpr id_index SYSLIST = id_index("SYSLIST");
        static constexpr id_index ANOP = id_index("ANOP");
        static constexpr id_index AEJECT = id_index("AEJECT");
    };
};

} // namespace hlasm_plugin::parser_library::context

#endif
