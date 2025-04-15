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

#include "macro_processor.h"

#include <concepts>
#include <memory>
#include <stack>

#include "context/hlasm_context.h"
#include "ebcdic_encoding.h"
#include "parsing/parser_impl.h"
#include "semantics/operand_impls.h"

namespace hlasm_plugin::parser_library::processing {

macro_processor::macro_processor(const analyzing_context& ctx,
    branching_provider& branch_provider,
    parse_lib_provider& lib_provider,
    diagnosable_ctx& diag_ctx)
    : instruction_processor(ctx, branch_provider, lib_provider, diag_ctx)
{}

void macro_processor::process(std::shared_ptr<const processing::resolved_statement> stmt)
{
    const auto next_sysndx = hlasm_ctx.next_sysndx();
    const auto sysndx_limit = hlasm_ctx.sysndx_limit();
    if (next_sysndx > sysndx_limit)
    {
        add_diagnostic(diagnostic_op::error_E072(stmt->stmt_range_ref()));
        return;
    }

    if (const auto& label = stmt->label_ref(); label.type == semantics::label_si_type::ORD)
    {
        auto [valid, id] = hlasm_ctx.try_get_symbol_name(std::get<semantics::ord_symbol_string>(label.value).symbol);
        if (valid && !hlasm_ctx.ord_ctx.get_symbol(id))
        {
            hlasm_ctx.ord_ctx.add_symbol_reference(
                context::symbol(id,
                    context::symbol_value(),
                    context::symbol_attributes(context::symbol_origin::MACH, 'M'_ebcdic),
                    location(),
                    {}),
                lib_info);
        }
    }

    auto [named, symbolic] = get_args(*stmt);
    auto [_, truncated] = hlasm_ctx.enter_macro(stmt->opcode_ref().value, std::move(named), std::move(symbolic));

    if (truncated) // this should never happen in a real code
        add_diagnostic(diagnostic_op::error_E081(stmt->operands_ref().field_range));
}
namespace {
bool is_attribute_consuming(char c)
{
    auto tmp = std::toupper(static_cast<int>(c));
    return tmp == 'O' || tmp == 'S' || tmp == 'I' || tmp == 'L' || tmp == 'T';
}

bool can_attribute_consume(char c)
{
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '=' || c == '$' || c == '_' || c == '#' || c == '@';
}

bool is_valid_string(std::string_view s)
{
    size_t index = 0;
    size_t apostrophes = 0;

    while (true)
    {
        auto ap_id = s.find_first_of('\'', index);
        if (ap_id == std::string_view::npos)
            break;

        if (ap_id > 0 && ap_id + 1 < s.size() && is_attribute_consuming(s[ap_id - 1])
            && can_attribute_consume(s[ap_id + 1]))
        {
            index = ap_id + 1;
            continue;
        }

        index = ap_id + 1;
        apostrophes++;
    }

    return apostrophes % 2 == 0;
}
} // namespace

std::pair<std::unique_ptr<context::macro_param_data_single>, bool> find_single_macro_param(
    std::string_view data, size_t& start)
{
    // always called in nested configuration
    size_t begin = start;

    while (true)
    {
        start = data.find_first_of(",'()", start);

        if (start == std::string_view::npos)
            return { nullptr, false };

        if (data[start] == '(')
        {
            size_t nest = 1;
            while (nest != 0)
            {
                ++start;
                if (start == data.size())
                    return { nullptr, false };

                if (data[start] == '(')
                    ++nest;
                if (data[start] == ')')
                    --nest;
            }
            ++start;
        }
        else if (data[start] == '\'')
        {
            if (start > 0 && start + 1 < data.size() && is_attribute_consuming(data[start - 1])
                && can_attribute_consume(data[start + 1]))
            {
                ++start;
                continue;
            }

            start = data.find_first_of('\'', start + 1);

            if (start == std::string_view::npos)
                return { nullptr, false };

            ++start;
        }
        else
            break;
    }

    auto tmp_start = start;
    bool comma_encountered = data[start] == ',';
    if (comma_encountered)
        ++start;

    return {
        std::make_unique<context::macro_param_data_single>(std::string(data.substr(begin, tmp_start - begin))),
        comma_encountered,
    };
}

context::macro_data_ptr macro_processor::string_to_macrodata(std::string data, diagnostic_adder& add_diags)
{
    if (data.empty())
        return std::make_unique<context::macro_param_data_dummy>();

    if (data.front() != '(' || data.back() != ')')
        return std::make_unique<context::macro_param_data_single>(std::move(data));


    std::stack<size_t> nests;
    std::stack<std::vector<context::macro_data_ptr>> macro_data;

    nests.push(0);
    macro_data.emplace();
    bool empty_op_pending = false;

    if (data.size() > std::numeric_limits<context::A_t>::max())
    {
        // This should not really happen in practice
        add_diags(diagnostic_op::error_E079);
        data.erase(std::numeric_limits<context::A_t>::max());
    }

    while (true)
    {
        auto begin = nests.top();

        if (begin == data.size())
            return std::make_unique<context::macro_param_data_single>(std::move(data));

        if (data[begin] == '(')
        {
            nests.push(begin + 1);
            macro_data.emplace();
            empty_op_pending = false;
        }
        else if (data[begin] == ')')
        {
            ++begin;
            nests.pop();

            auto vec = std::move(macro_data.top());
            macro_data.pop();

            if (std::exchange(empty_op_pending, false))
                vec.push_back(std::make_unique<context::macro_param_data_single>(""));

            if (begin != data.size() && data[begin] != ',' && data[begin] != ')')
            {
                auto [tmp_single, comma] = find_single_macro_param(data, begin);
                empty_op_pending = comma;

                if (tmp_single == nullptr)
                    return std::make_unique<context::macro_param_data_single>(std::move(data));

                auto single = context::macro_param_data_composite(std::move(vec)).get_value() + tmp_single->get_value();

                macro_data.top().emplace_back(std::make_unique<context::macro_param_data_single>(std::move(single)));
            }
            else
                macro_data.top().emplace_back(std::make_unique<context::macro_param_data_composite>(std::move(vec)));

            nests.top() = begin + (begin != data.size() && data[begin] == ',' ? 1 : 0);

            if (nests.size() == 1)
            {
                break;
            }
        }
        else
        {
            auto [op, comma] = find_single_macro_param(data, begin);
            macro_data.top().push_back(std::move(op));
            nests.top() = begin;
            empty_op_pending = comma;

            if (macro_data.top().back() == nullptr)
                return std::make_unique<context::macro_param_data_single>(std::move(data));
        }
    }

    if (nests.top() != data.size())
        return std::make_unique<context::macro_param_data_single>(std::move(data));

    assert(macro_data.size() == 1 && macro_data.top().size() == 1);

    return std::move(macro_data.top().front());
}

macro_arguments macro_processor::get_args(const resolved_statement& statement) const
{
    return {
        .name_param = get_label_args(statement),
        .symbolic_params = get_operand_args(statement),
    };
}

context::macro_data_ptr macro_processor::get_label_args(const resolved_statement& statement) const
{
    const auto& label = statement.label_ref();
    std::string value;
    switch (label.type)
    {
        case semantics::label_si_type::CONC:
            value = semantics::concatenation_point::evaluate(std::get<semantics::concat_chain>(label.value), eval_ctx);
            break;
        case semantics::label_si_type::ORD:
            value = std::get<semantics::ord_symbol_string>(label.value).mixed_case;
            break;
        case semantics::label_si_type::MAC:
            value = std::get<std::string>(label.value);
            break;
        case semantics::label_si_type::VAR:
            value = semantics::var_sym_conc::evaluate(std::get<semantics::vs_ptr>(label.value)->evaluate(eval_ctx));
            break;
        default:
            return context::macro_data_ptr();
    }
    static constexpr size_t label_limit = 63;
    if (value.size() > label_limit)
    {
        if (const auto sub = utils::utf8_substr(value, 0, label_limit); sub.str.size() != value.size())
        {
            value.clear();
            add_diagnostic(diagnostic_op::error_CE011(label.field_range));
        }
    }
    return std::make_unique<context::macro_param_data_single>(std::move(value));
}

context::hlasm_context::name_result is_keyword(const semantics::concat_chain& chain, context::hlasm_context& hlasm_ctx)
{
    using namespace semantics;
    if (!concat_chain_starts_with<char_str_conc, equals_conc>(chain))
        return std::make_pair(false, context::id_index());
    return hlasm_ctx.try_get_symbol_name(std::get<char_str_conc>(chain[0].value).value);
}

namespace {
bool has_keyword_operand(const std::unordered_map<context::id_index, const context::macro_param_base*>& named_params,
    context::id_index arg_name)
{
    const auto named = named_params.find(arg_name);
    return named != named_params.end() && named->second->param_type == context::macro_param_type::KEY_PAR_TYPE;
}

template<
    std::invocable<semantics::concat_chain::const_iterator, semantics::concat_chain::const_iterator, diagnostic_adder&>
        T>
context::macro_data_ptr create_macro_data_inner(semantics::concat_chain::const_iterator begin,
    semantics::concat_chain::const_iterator end,
    T& to_string,
    diagnostic_adder& add_diags,
    bool nested = false)
{
    auto size = end - begin;
    if (size == 0)
        return std::make_unique<context::macro_param_data_dummy>();
    else if (size == 1 && !semantics::concat_chain_matches<semantics::sublist_conc>(begin, end))
        return macro_processor::string_to_macrodata(to_string(begin, end, add_diags), add_diags);
    else if (size > 1)
    {
        if (auto s = to_string(begin, end, add_diags); s.empty())
            return std::make_unique<context::macro_param_data_dummy>();
        else if (s.front() != '(' && (!nested || semantics::concatenation_point::find_var_sym(begin, end) == nullptr))
            return macro_processor::string_to_macrodata(std::move(s), add_diags);
        else if (is_valid_string(s))
            return macro_processor::string_to_macrodata(std::move(s), add_diags);
        else
        {
            add_diags(diagnostic_op::error_S0005);
            return std::make_unique<context::macro_param_data_dummy>();
        }
    }

    const auto& inner_chains = std::get<semantics::sublist_conc>(begin->value).list;

    std::vector<context::macro_data_ptr> sublist;
    sublist.reserve(inner_chains.size());

    for (auto& inner_chain : inner_chains)
    {
        sublist.push_back(create_macro_data_inner(inner_chain.begin(), inner_chain.end(), to_string, add_diags, true));
    }
    return std::make_unique<context::macro_param_data_composite>(std::move(sublist));
}

size_t find_erase_point(std::string_view s, const size_t limit)
{
    size_t i = 0;
    while (true)
    {
        auto l = limit;
        auto j = i;
        for (; j < s.size(); ++j)
        {
            l -= (s[j] & 0xc0) != 0x80;
            if (l == (size_t)-1)
                break;
        }
        if (l == (size_t)-1)
            i = j;
        else if (l == 0 && j == s.size())
            i = j;
        else
            break;
    }

    return i;
}

constexpr size_t macro_argument_length_limit = 4064;

} // namespace

auto macro_processor::make_evaluator() const
{
    return [this, limit_diag_issued = false](semantics::concat_chain::const_iterator b,
               semantics::concat_chain::const_iterator e,
               diagnostic_adder& diags) mutable {
        auto s = semantics::concatenation_point::evaluate(b, e, eval_ctx);
        if (s.size() < macro_argument_length_limit)
            return s;
        const auto erase_to = find_erase_point(s, macro_argument_length_limit);
        if (erase_to == 0)
            return s;
        s.erase(0, erase_to);
        if (!limit_diag_issued)
        {
            limit_diag_issued = true;
            diags(diagnostic_op::error_CE011);
        }
        return s;
    };
}

std::vector<context::macro_arg> macro_processor::get_operand_args(const resolved_statement& statement) const
{
    std::vector<context::macro_arg> args;
    std::vector<context::id_index> keyword_params;

    const auto& ops = statement.operands_ref().value;
    if (ops.empty())
        return args;

    const auto& named_params = hlasm_ctx.get_macro_definition(statement.opcode_ref().value)->named_params();
    args.reserve(ops.size());

    for (const auto& op : ops)
    {
        if (op->type != semantics::operand_type::MAC)
        {
            args.emplace_back(std::make_unique<context::macro_param_data_dummy>());
            continue;
        }

        const auto tmp = op->access_mac();
        assert(tmp);

        const auto [valid_keyword, arg_name] = is_keyword(tmp->chain, hlasm_ctx);

        const auto chain_b = tmp->chain.begin() + 2 * valid_keyword;
        const auto chain_e = tmp->chain.end();

        diagnostic_adder add_diags(eval_ctx.diags, tmp->operand_range);
        if (valid_keyword)
        {
            if (!has_keyword_operand(named_params, arg_name))
            {
                add_diags(diagnostic_op::warning_W014);

                args.emplace_back(std::make_unique<context::macro_param_data_single>(
                    semantics::concatenation_point::to_string(tmp->chain)));

                continue;
            }

            if (std::ranges::find(keyword_params, arg_name) != keyword_params.end())
                add_diags(diagnostic_op::error_E011, "Keyword");
            else
                keyword_params.push_back(arg_name);
        }
        auto evaluator = make_evaluator();
        args.emplace_back(create_macro_data_inner(chain_b, chain_e, evaluator, add_diags), arg_name);
    }

    return args;
}

context::macro_data_ptr macro_processor::create_macro_data(semantics::concat_chain::const_iterator begin,
    semantics::concat_chain::const_iterator end,
    diagnostic_adder& add_diags)
{
    auto tmp = semantics::concatenation_point::find_var_sym(begin, end);
    if (tmp)
    {
        add_diags(diagnostic_op::error_E064);
        return std::make_unique<context::macro_param_data_dummy>();
    }

    static constexpr auto f = [](semantics::concat_chain::const_iterator b,
                                  semantics::concat_chain::const_iterator e,
                                  diagnostic_adder&) { return semantics::concatenation_point::to_string(b, e); };
    return create_macro_data_inner(begin, end, f, add_diags);
}

} // namespace hlasm_plugin::parser_library::processing
