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

#include "document.h"

#include <numeric>

namespace hlasm_plugin::parser_library {
document::document(std::string_view text)
{
    if (text.empty())
    {
        m_lines.emplace_back(original_line());
        return;
    }
    size_t line_no = 0;
    while (!text.empty())
    {
        auto p = text.find_first_of("\r\n");
        if (p == std::string_view::npos)
            break;
        if (text.substr(p, 2) == "\r\n")
            ++p;

        m_lines.emplace_back(original_line { text.substr(0, p + 1), line_no });

        text.remove_prefix(p + 1);
        ++line_no;
    }
    if (!text.empty())
        m_lines.emplace_back(original_line { text, line_no });
}

std::string document::text() const
{
    return std::accumulate(m_lines.begin(), m_lines.end(), std::string(), [](std::string&& result, const auto& l) {
        auto t = l.text();
        result.append(t);
        if (t.empty() || t.back() != '\n')
            result.push_back('\n');
        return std::move(result);
    });
}

void document::convert_to_replaced()
{
    for (auto& line : m_lines)
    {
        if (line.is_original())
        {
            line = document_line(replaced_line { std::string(line.text()) });
        }
    }
}

} // namespace hlasm_plugin::parser_library
