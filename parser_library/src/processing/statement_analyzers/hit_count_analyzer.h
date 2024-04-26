/*
 * Copyright (c) 2023 Broadcom.
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

#ifndef PROCESSING_HIT_COUNT_ANALYZER_H
#define PROCESSING_HIT_COUNT_ANALYZER_H

#include <algorithm>
#include <functional>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "processing/statement_providers/statement_provider_kind.h"
#include "processing_format.h"
#include "statement_analyzer.h"
#include "utils/resource_location.h"

namespace hlasm_plugin::parser_library {
namespace context {
class hlasm_context;
} // namespace context

namespace semantics {
struct core_statement;
} // namespace semantics
} // namespace hlasm_plugin::parser_library

namespace hlasm_plugin::parser_library::processing {
using stmt_lines_range = std::pair<size_t, size_t>;

struct line_detail
{
    size_t count = 0;
    bool contains_statement = false;
    bool macro_definition = false;

    line_detail& operator+=(const line_detail& other)
    {
        count += other.count;
        contains_statement |= other.contains_statement;
        macro_definition |= other.macro_definition;
        return *this;
    }

    line_detail& add(size_t other_count, bool is_macro)
    {
        count += other_count;
        contains_statement = true;
        macro_definition |= is_macro;
        return *this;
    }
};

struct line_hits
{
    std::vector<line_detail> line_details;
    size_t max_lineno = 0;

    line_hits& merge(const line_hits& other)
    {
        max_lineno = std::max(max_lineno, other.max_lineno);

        if (!other.line_details.empty())
        {
            auto& other_line_hits = other.line_details;
            auto& our_line_hits = line_details;

            if (other_line_hits.size() > our_line_hits.size())
                our_line_hits.resize(other_line_hits.size());

            for (size_t i = 0; i <= other.max_lineno; ++i)
                our_line_hits[i] += other_line_hits[i];
        }

        return *this;
    }

    void add(size_t line, size_t count, bool is_macro)
    {
        update_max_and_resize(line);
        line_details[line].add(count, is_macro);
    }

    void add(const stmt_lines_range& lines_range, size_t count, bool is_macro)
    {
        const auto& [start_line, end_line] = lines_range;

        update_max_and_resize(end_line);
        for (auto i = start_line; i <= end_line; ++i)
            line_details[i].add(count, is_macro);
    }

private:
    void update_max_and_resize(size_t line)
    {
        max_lineno = std::max(max_lineno, line);

        if (line_details.size() <= line)
            line_details.resize(2 * line + 1000);
    }
};

struct hit_count_entry
{
    bool has_sections = false;
    line_hits hits;
    std::vector<bool> macro_definition_lines;

    hit_count_entry& merge(const hit_count_entry& other)
    {
        has_sections |= other.has_sections;

        hits.merge(other.hits);

        const auto& other_mac_def_lines = other.macro_definition_lines;
        const auto common = std::min(macro_definition_lines.size(), other_mac_def_lines.size());
        const auto middle = other_mac_def_lines.begin() + common;
        std::transform(other_mac_def_lines.begin(),
            middle,
            macro_definition_lines.begin(),
            macro_definition_lines.begin(),
            std::logical_or<bool>());
        macro_definition_lines.insert(macro_definition_lines.end(), middle, other_mac_def_lines.end());

        return *this;
    }

    bool contains_line(size_t i) const noexcept
    {
        return i < macro_definition_lines.size() && macro_definition_lines[i];
    }

    void emplace_line(size_t i)
    {
        if (i >= macro_definition_lines.size())
            macro_definition_lines.resize(i + 1);
        macro_definition_lines[i] = true;
    }
};

using hit_count_map =
    std::unordered_map<utils::resource::resource_location, hit_count_entry, utils::resource::resource_location_hasher>;

class hit_count_analyzer final : public statement_analyzer
{
public:
    explicit hit_count_analyzer(context::hlasm_context& ctx);
    ~hit_count_analyzer() = default;

    bool analyze(const context::hlasm_statement& statement,
        statement_provider_kind prov_kind,
        processing_kind proc_kind,
        bool) override;

    void analyze_aread_line(const utils::resource::resource_location& rl, size_t lineno, std::string_view) override;

    hit_count_map take_hit_count_map();

private:
    enum class statement_type
    {
        REGULAR,
        MACRO_INIT,
        MACRO_NAME,
        MACRO_BODY,
        MACRO_END
    };

    context::hlasm_context& m_ctx;
    hit_count_map m_hit_count_map;
    statement_type m_next_stmt_type = statement_type::REGULAR;
    size_t m_macro_level = 0;

    const utils::resource::resource_location& get_current_stmt_rl(processing_kind proc_kind) const;
    hit_count_entry& get_hc_entry_reference(const utils::resource::resource_location& rl);

    statement_type get_stmt_type(const semantics::core_statement& statement);
};

} // namespace hlasm_plugin::parser_library::processing

#endif
