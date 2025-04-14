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
    bool was_var = false;
    std::vector<std::pair<std::pair<size_t, bool>, range>> ranges;
    size_t utf16_offset = 0;

    void operator()(const char_str_conc& v)
    {
        if constexpr (collect_ranges)
        {
            ranges.emplace_back(std::pair(utf16_offset, false), v.conc_range);
            utf16_offset += utils::length_utf16_no_validation(v.value);
        }
        result.append(v.value);
        was_var = false;
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
concatenation_point::evaluate_with_range_map(const concat_chain& chain, const expressions::evaluation_context& eval_ctx)
{
    return evaluate_with_range_map(chain.begin(), chain.end(), eval_ctx);
}
std::pair<std::string, std::vector<std::pair<std::pair<size_t, bool>, range>>>
concatenation_point::evaluate_with_range_map(concat_chain::const_iterator begin,
    concat_chain::const_iterator end,
    const expressions::evaluation_context& eval_ctx)
{
    std::string ret;
    concatenation_point_evaluator<true> evaluator { ret, eval_ctx };
    evaluator.ranges.reserve(std::ranges::distance(begin, end));

    for (auto it = begin; it != end; ++it)
        std::visit(evaluator, it->value);

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
        if (v.symbol->created)
        {
            result.push_back('(');
            operator()(v.symbol->access_created()->created_name);
            result.push_back(')');
        }
        else
            result.append(v.symbol->access_basic()->name.to_string_view());
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

} // namespace hlasm_plugin::parser_library::semantics
