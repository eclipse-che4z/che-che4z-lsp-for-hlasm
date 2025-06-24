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
struct instruction_si;
} // namespace semantics
} // namespace hlasm_plugin::parser_library

namespace hlasm_plugin::parser_library::processing {
using stmt_lines_range = std::pair<size_t, size_t>;

struct line_detail
{
    size_t count = 0;
    bool contains_statement = false;
    bool macro_body = false;
    bool macro_prototype = false;

    line_detail merge(const line_detail& other) const
    {
        return line_detail {
            .count = count + other.count,
            .contains_statement = contains_statement || other.contains_statement,
            .macro_body = macro_body || other.macro_body,
            .macro_prototype = macro_prototype || other.macro_prototype,
        };
    }

    line_detail& add(size_t other_count, bool is_macro)
    {
        count += other_count;
        contains_statement = true;
        macro_body |= is_macro;
        return *this;
    }
};

struct hit_count_entry
{
    std::vector<line_detail> details;
    bool has_sections = false;

    hit_count_entry& merge(const hit_count_entry& other)
    {
        has_sections |= other.has_sections;

        const auto [e, o, _] = std::ranges::transform(details, other.details, details.begin(), &line_detail::merge);
        details.insert(e, o, other.details.end());

        return *this;
    }

    bool contains_prototype(size_t i) const noexcept { return i < details.size() && details[i].macro_prototype; }

    void emplace_prototype(size_t line)
    {
        update_max_and_resize(line);
        details[line].macro_prototype = true;
    }

    void add(size_t line, size_t count, bool is_macro)
    {
        update_max_and_resize(line);
        details[line].add(count, is_macro);
    }

    void add(const stmt_lines_range& lines_range, size_t count, bool is_macro)
    {
        const auto& [start_line, end_line] = lines_range;

        update_max_and_resize(end_line);
        for (size_t line = start_line; line <= end_line; ++line)
            details[line].add(count, is_macro);
    }

private:
    void update_max_and_resize(size_t line)
    {
        if (line >= details.size())
            details.resize(line + 1);
    }
};

using hit_count_map = std::unordered_map<utils::resource::resource_location, hit_count_entry>;

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

    statement_type get_stmt_type(const semantics::instruction_si& instr, const op_code* op);
};

} // namespace hlasm_plugin::parser_library::processing

#endif
