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

#include "concatenation.h"

#include "expressions/conditional_assembly/terms/ca_var_sym.h"
#include "utils/unicode_text.h"

namespace hlasm_plugin::parser_library::semantics {

std::string concatenation_point::evaluate(const concat_chain& chain, const expressions::evaluation_context& eval_ctx)
{
    return evaluate(chain.begin(), chain.end(), eval_ctx);
}

template<bool collect_ranges>
struct concatenation_point_evaluator
{
    std::string& result;
    const expressions::evaluation_context& eval_ctx;
    size_t initial_line {};
    std::span<const size_t> line_limits = {};

    std::vector<std::pair<std::pair<size_t, bool>, range>> ranges = {};
    bool was_var = false;
    size_t utf16_offset = 0;

    void operator()(const char_str_conc& v)
    {
        if constexpr (collect_ranges)
        {
            if (v.conc_range.start.line == v.conc_range.end.line) [[likely]]
            {
                ranges.emplace_back(std::pair(utf16_offset, false), v.conc_range);
                utf16_offset += v.conc_range.end.column - v.conc_range.start.column;
            }
            else
                fill_continued_ranges(v);
        }
        result.append(v.value);
        was_var = false;
    }

    void fill_continued_ranges(const char_str_conc& v) requires(collect_ranges)
    {
        auto p = std::string_view(v.value);
        auto l = v.conc_range.start.line - initial_line;
        const auto le = v.conc_range.end.line - initial_line;
        auto c = v.conc_range.start.column;
        while (!p.empty() && l < le)
        {
            const auto limit = line_limits[l];
            auto fits = limit - c;
            p = utils::skip_chars_utf16(p, fits);
            const range next_range(position(initial_line + l, c), position(initial_line + l, limit));
            ranges.emplace_back(std::pair(utf16_offset, false), next_range);
            utf16_offset += fits;

            ++l;
            c = 15; // TODO: continuation
        }
        const range last_range(position(v.conc_range.end.line, c), v.conc_range.end);
        ranges.emplace_back(std::pair(utf16_offset, false), last_range);
        utf16_offset += v.conc_range.end.column - c;
    }

    void operator()(const var_sym_conc& v)
    {
        auto value = var_sym_conc::evaluate(v.symbol->evaluate(eval_ctx));
        if constexpr (collect_ranges)
        {
            ranges.emplace_back(std::pair(utf16_offset, true), v.symbol->symbol_range);
            utf16_offset += utils::length_utf16_no_validation(value);
        }
        result.append(std::move(value));
        was_var = true;
    }

    void operator()(const dot_conc& v)
    {
        if (!was_var)
        {
            if constexpr (collect_ranges)
            {
                ranges.emplace_back(std::pair(utf16_offset, false), v.conc_range);
                utf16_offset += 1;
            }
            result.push_back('.');
        }
        was_var = false;
    }

    void operator()(const sublist_conc& v)
    {
        assert(!collect_ranges);
        result.push_back('(');
        for (bool first = true; const auto& l : v.list)
        {
            if (first)
                first = false;
            else
                result.push_back(',');
            for (const auto visitor = [this](const auto& e) { operator()(e); }; const auto& c : l)
                std::visit(visitor, c.value);
        }
        result.push_back(')');
        was_var = false;
    }

    void operator()(const equals_conc& v)
    {
        if constexpr (collect_ranges)
        {
            ranges.emplace_back(std::pair(utf16_offset, false), v.conc_range);
            utf16_offset += 1;
        }
        result.push_back('=');
        was_var = false;
    }
};

std::string concatenation_point::evaluate(concat_chain::const_iterator begin,
    concat_chain::const_iterator end,
    const expressions::evaluation_context& eval_ctx)
{
    std::string ret;
    concatenation_point_evaluator<false> evaluator { ret, eval_ctx };

    for (auto it = begin; it != end; ++it)
        std::visit(evaluator, it->value);

    return ret;
}
std::pair<std::string, std::vector<std::pair<std::pair<size_t, bool>, range>>>
concatenation_point::evaluate_with_range_map(const concat_chain& chain,
    const size_t initial_line,
    std::span<const size_t> line_limits,
    const expressions::evaluation_context& eval_ctx)
{
    std::string ret;
    concatenation_point_evaluator<true> evaluator { ret, eval_ctx, initial_line, line_limits };
    // the size estimate is not exact here
    evaluator.ranges.reserve(chain.size() + line_limits.size());

    for (const auto& [value] : chain)
        std::visit(evaluator, value);

    return { std::move(ret), std::move(evaluator.ranges) };
}

void concatenation_point::resolve(diagnostic_op_consumer& diag) const
{
    std::visit([&diag](const auto& v) { v.resolve(diag); }, value);
}

void concatenation_point::clear_concat_chain(concat_chain& chain)
{
    std::erase_if(chain, [](const concatenation_point& p) {
        if (auto* str = std::get_if<char_str_conc>(&p.value); str && str->value.empty())
            return true;
        if (auto* var = std::get_if<var_sym_conc>(&p.value); var && !var->symbol)
            return true;

        return false;
    });
}

std::string concatenation_point::to_string(const concat_chain& chain) { return to_string(chain.begin(), chain.end()); }

std::string concatenation_point::to_string(concat_chain&& chain)
{
    if (chain.empty())
        return {};
    else if (chain.size() == 1 && std::holds_alternative<char_str_conc>(chain.front().value))
        return std::move(std::get<char_str_conc>(chain.front().value).value);
    else
        return to_string(chain.begin(), chain.end());
}

struct concat_point_stringifier
{
    std::string& result;

    void operator()(const char_str_conc& v) const { result.append(v.value); }

    void operator()(const var_sym_conc& v) const
    {
        result.push_back('&');
        if (const auto* created = v.symbol->created())
        {
            result.push_back('(');
            operator()(*created);
            result.push_back(')');
        }
        else
            result.append(v.symbol->named()->to_string_view());
    }

    void operator()(const dot_conc&) const { result.push_back('.'); }

    void operator()(const sublist_conc& v) const
    {
        result.push_back('(');
        for (bool first = true; const auto& l : v.list)
        {
            if (first)
                first = false;
            else
                result.push_back(',');
            operator()(l);
        }
        result.push_back(')');
    }

    void operator()(const equals_conc&) const { result.push_back('='); }

    void operator()(const concat_chain& cc) const
    {
        for (const auto visitor = [this](const auto& e) { operator()(e); }; const auto& c : cc)
            std::visit(visitor, c.value);
    }
};

std::string concatenation_point::to_string(concat_chain::const_iterator begin, concat_chain::const_iterator end)
{
    std::string ret;
    concat_point_stringifier stringifier { ret };

    for (auto it = begin; it != end; ++it)
        std::visit(stringifier, it->value);

    return ret;
}

const var_sym_conc* concatenation_point::find_var_sym(
    concat_chain::const_iterator begin, concat_chain::const_iterator end)
{
    for (auto it = begin; it != end; ++it)
    {
        if (auto* var = std::get_if<var_sym_conc>(&it->value))
            return var;

        if (auto* sublist = std::get_if<sublist_conc>(&it->value))
            for (const auto& entry : sublist->list)
                if (auto tmp = find_var_sym(entry.begin(), entry.end()))
                    return tmp;
    }

    return nullptr;
}

bool concatenation_point::get_undefined_attributed_symbols(
    std::vector<context::id_index>& symbols, const concat_chain& chain, const expressions::evaluation_context& eval_ctx)
{
    bool result = false;
    for (auto it = chain.begin(); it != chain.end(); ++it)
    {
        if (auto* var = std::get_if<var_sym_conc>(&it->value))
            result |= expressions::ca_var_sym::get_undefined_attributed_symbols_vs(symbols, var->symbol, eval_ctx);
        else if (auto* sublist = std::get_if<sublist_conc>(&it->value))
            for (const auto& entry : sublist->list)
                result |= get_undefined_attributed_symbols(symbols, entry, eval_ctx);
    }
    return result;
}

void char_str_conc::resolve(diagnostic_op_consumer&) const {}

void var_sym_conc::resolve(diagnostic_op_consumer& diag) const { symbol->resolve(context::SET_t_enum::A_TYPE, diag); }

std::string var_sym_conc::evaluate(context::SET_t varsym_value)
{
    switch (varsym_value.type())
    {
        case context::SET_t_enum::A_TYPE:
            return std::to_string(std::abs(varsym_value.access_a()));
        case context::SET_t_enum::B_TYPE:
            return varsym_value.access_b() ? "1" : "0";
        case context::SET_t_enum::C_TYPE:
            return std::move(varsym_value.access_c());
        default:
            return "";
    }
}

void dot_conc::resolve(diagnostic_op_consumer&) const {}

void equals_conc::resolve(diagnostic_op_consumer&) const {}

void sublist_conc::resolve(diagnostic_op_consumer& diag) const
{
    for (const auto& l : list)
        for (const auto& e : l)
            e.resolve(diag);
}

concatenation_point::concatenation_point(
    std::in_place_type_t<char_str_conc> t, std::string value, const range& conc_range)
    : value(t, std::move(value), conc_range)
{}
concatenation_point::concatenation_point(std::in_place_type_t<var_sym_conc> t, vs_ptr v)
    : value(t, std::move(v))
{}
concatenation_point::concatenation_point(std::in_place_type_t<dot_conc> t, const range& r)
    : value(t, r)
{}
concatenation_point::concatenation_point(std::in_place_type_t<sublist_conc> t, std::vector<concat_chain> list)
    : value(t, std::move(list))
{}
concatenation_point::concatenation_point(std::in_place_type_t<equals_conc> t, const range& r)
    : value(t, r)
{}

var_sym_conc::var_sym_conc(vs_ptr s)
    : symbol(std::move(s))
{}

sublist_conc::sublist_conc(std::vector<concat_chain> list)
    : list(std::move(list))
{}

} // namespace hlasm_plugin::parser_library::semantics
