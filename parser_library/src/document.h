/*
 * Copyright (c) 2022 Broadcom.
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

#ifndef HLASMPLUGIN_PARSERLIBRARY_DOCUMENT_H
#define HLASMPLUGIN_PARSERLIBRARY_DOCUMENT_H

#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace hlasm_plugin::parser_library {

struct replaced_line
{
    std::string m_text;
};

struct original_line
{
    std::string_view m_text;
    size_t m_lineno = 0;
};

class document_line
{
    std::variant<original_line, replaced_line> m_line;

public:
    explicit document_line(original_line l) noexcept
        : m_line(std::move(l))
    {}
    explicit document_line(replaced_line l) noexcept
        : m_line(std::move(l))
    {}

    std::string_view text() const noexcept
    {
        return std::visit([](const auto& s) -> std::string_view { return s.m_text; }, m_line);
    }

    std::optional<size_t> lineno() const noexcept
    {
        if (std::holds_alternative<original_line>(m_line))
            return std::get<original_line>(m_line).m_lineno;
        else
            return std::nullopt;
    }

    bool is_original() const noexcept { return std::holds_alternative<original_line>(m_line); }

    bool same_type(const document_line& d) const noexcept { return m_line.index() == d.m_line.index(); }
};

class document
{
    std::vector<document_line> m_lines;

public:
    document() = default;
    explicit document(std::string_view text);
    explicit document(std::vector<document_line> lines) noexcept
        : m_lines(std::move(lines))
    {}

    auto begin() const { return m_lines.begin(); }

    auto end() const { return m_lines.end(); }

    auto size() const { return m_lines.size(); }

    std::string text() const;

    const auto& at(size_t idx) const { return m_lines.at(idx); }

    void convert_to_replaced();
};

} // namespace hlasm_plugin::parser_library

#endif
