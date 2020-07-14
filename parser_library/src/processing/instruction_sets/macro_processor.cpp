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

#include <functional>
#include <memory>

#include "processing/context_manager.h"

namespace hlasm_plugin::parser_library::processing {

macro_processor::macro_processor(context::hlasm_context& hlasm_ctx,
    attribute_provider& attr_provider,
    branching_provider& branch_provider,
    workspaces::parse_lib_provider& lib_provider)
    : instruction_processor(hlasm_ctx, attr_provider, branch_provider, lib_provider)
{}

void macro_processor::process(context::shared_stmt_ptr stmt)
{
    auto args = get_args(*stmt->access_resolved());

    hlasm_ctx.enter_macro(
        stmt->access_resolved()->opcode_ref().value, std::move(args.name_param), std::move(args.symbolic_params));
}


void macro_processor::process(context::unique_stmt_ptr stmt)
{
    auto args = get_args(*stmt->access_resolved());

    hlasm_ctx.enter_macro(
        stmt->access_resolved()->opcode_ref().value, std::move(args.name_param), std::move(args.symbolic_params));
}

bool is_data_def(char c)
{
    c = (char)toupper(c);
    return c == 'L' || c == 'I' || c == 'S' || c == 'T' || c == 'D' || c == 'O' || c == 'N' || c == 'K';
}

std::unique_ptr<context::macro_param_data_single> find_single_macro_param(const std::string& data, size_t& start)
{
    size_t begin = start;

    while (true)
    {
        start = data.find_first_of(",'()", start);

        if (start == std::string::npos)
            return nullptr;

        if (data[start] == '(')
        {
            size_t nest = 1;
            while (nest != 0)
            {
                ++start;
                if (start == data.size())
                    return nullptr;

                if (data[start] == '(')
                    ++nest;
                if (data[start] == ')')
                    --nest;
            }
            ++start;
        }
        else if (data[start] == '\'')
        {
            if (start > 0 && is_data_def(data[start - 1]))
            {
                ++start;
                continue;
            }

            start = data.find_first_of('\'', start + 1);

            if (start == std::string::npos)
                return nullptr;

            ++start;
        }
        else
            break;
    }

    auto tmp_start = start;
    if (data[start] == ',')
        ++start;

    return std::make_unique<context::macro_param_data_single>(data.substr(begin, tmp_start - begin));
}

context::macro_data_ptr macro_processor::string_to_macrodata(std::string data)
{
    if (data.empty())
        return std::make_unique<context::macro_param_data_dummy>();

    if (data.front() != '(' || data.back() != ')')
        return std::make_unique<context::macro_param_data_single>(std::move(data));


    std::stack<size_t> nests;
    std::stack<std::vector<context::macro_data_ptr>> macro_data;

    nests.push(0);
    macro_data.emplace();

    while (true)
    {
        auto begin = nests.top();

        if (begin == data.size())
            return std::make_unique<context::macro_param_data_single>(std::move(data));

        if (data[begin] == '(')
        {
            nests.push(begin + 1);
            macro_data.emplace();
        }
        else if (data[begin] == ')')
        {
            ++begin;
            nests.pop();

            auto vec = std::move(macro_data.top());
            macro_data.pop();

            if (begin != data.size() && data[begin] != ',' && data[begin] != ')')
            {
                auto tmp_single = find_single_macro_param(data, begin);

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
            macro_data.top().push_back(find_single_macro_param(data, begin));
            nests.top() = begin;

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
    macro_arguments args;

    args.name_param = get_label_args(statement);

    args.symbolic_params = get_operand_args(statement);

    return args;
}

context::macro_data_ptr macro_processor::get_label_args(const resolved_statement& statement) const
{
    switch (statement.label_ref().type)
    {
        case semantics::label_si_type::CONC:
            return std::make_unique<context::macro_param_data_single>(semantics::concatenation_point::evaluate(
                std::get<semantics::concat_chain>(statement.label_ref().value), eval_ctx));
        case semantics::label_si_type::ORD:
        case semantics::label_si_type::MAC:
            return std::make_unique<context::macro_param_data_single>(
                std::get<std::string>(statement.label_ref().value));
        case semantics::label_si_type::VAR:
            return std::make_unique<context::macro_param_data_single>(semantics::var_sym_conc::evaluate(
                std::get<semantics::vs_ptr>(statement.label_ref().value)->evaluate(eval_ctx)));
        default:
            return context::macro_data_ptr();
    }
}

bool is_keyword(const semantics::concat_chain& chain, const context_manager& mngr)
{
    return chain.size() >= 2 && chain[0]->type == semantics::concat_type::STR
        && chain[1]->type == semantics::concat_type::EQU
        && mngr.try_get_symbol_name(chain[0]->access_str()->value).first;
}

std::vector<context::macro_arg> macro_processor::get_operand_args(const resolved_statement& statement) const
{
    context_manager mngr(hlasm_ctx);
    std::vector<context::macro_arg> args;
    std::vector<context::id_index> keyword_params;

    for (const auto& op : statement.operands_ref().value)
    {
        if (op->type == semantics::operand_type::EMPTY)
        {
            args.push_back({ std::make_unique<context::macro_param_data_dummy>(), nullptr });
            continue;
        }

        auto tmp = op->access_mac();
        assert(tmp);

        auto& tmp_chain = tmp->chain;

        semantics::concatenation_point::clear_concat_chain(tmp_chain);

        if (is_keyword(tmp_chain, mngr)) // keyword
        {
            get_keyword_arg(statement, tmp_chain, args, keyword_params, tmp->operand_range);
        }
        else if (tmp_chain.size() == 1 && tmp_chain.front()->type == semantics::concat_type::VAR) // single varsym
        {
            context::macro_data_ptr data = string_to_macrodata(
                semantics::var_sym_conc::evaluate(tmp_chain.front()->access_var()->symbol->evaluate(eval_ctx)));

            args.push_back({ std::move(data), nullptr });
        }
        else // rest
            args.push_back({ create_macro_data(tmp_chain.begin(), tmp_chain.end(), eval_ctx), nullptr });
    }

    return args;
}

void macro_processor::get_keyword_arg(const resolved_statement& statement,
    const semantics::concat_chain& chain,
    std::vector<context::macro_arg>& args,
    std::vector<context::id_index>& keyword_params,
    range op_range) const
{
    context_manager mngr(hlasm_ctx);

    auto id = mngr.try_get_symbol_name(chain[0]->access_str()->value).second;
    assert(id != context::id_storage::empty_id);

    auto named = hlasm_ctx.get_macro_definition(statement.opcode_ref().value)->named_params().find(id);
    if (named == hlasm_ctx.get_macro_definition(statement.opcode_ref().value)->named_params().end()
        || named->second->param_type == context::macro_param_type::POS_PAR_TYPE)
    {
        add_diagnostic(diagnostic_op::error_E010("keyword parameter", op_range));

        // MACROCASE TODO
        auto name = chain[0]->access_str()->value;

        args.push_back({ std::make_unique<context::macro_param_data_single>(
                             name + "=" + semantics::concatenation_point::to_string(chain.begin() + 2, chain.end())),
            nullptr });
    }
    else
    {
        if (std::find(keyword_params.begin(), keyword_params.end(), id) != keyword_params.end())
            add_diagnostic(diagnostic_op::error_E011("Keyword", op_range));
        else
            keyword_params.push_back(id);

        auto chain_begin = chain.begin() + 2;
        auto chain_end = chain.end();
        auto chain_size = chain.size() - 2;
        context::macro_data_ptr data;

        if (chain_size == 1 && (*chain_begin)->type == semantics::concat_type::SUB)
            data = create_macro_data(chain_begin, chain_end, eval_ctx);
        else
            data = string_to_macrodata(semantics::concatenation_point::evaluate(chain_begin, chain_end, eval_ctx));

        args.push_back({ std::move(data), id });
    }
}

context::macro_data_ptr create_macro_data_inner(semantics::concat_chain::const_iterator begin,
    semantics::concat_chain::const_iterator end,
    const std::function<std::string(semantics::concat_chain::const_iterator, semantics::concat_chain::const_iterator)>&
        to_string)
{
    auto size = end - begin;
    if (size == 0)
        return std::make_unique<context::macro_param_data_dummy>();
    else if (size > 1 || (size == 1 && (*begin)->type != semantics::concat_type::SUB))
        return std::make_unique<context::macro_param_data_single>(to_string(begin, end));

    const auto& inner_chains = (*begin)->access_sub()->list;

    std::vector<context::macro_data_ptr> sublist;

    for (auto& inner_chain : inner_chains)
    {
        sublist.push_back(create_macro_data_inner(inner_chain.begin(), inner_chain.end(), to_string));
    }
    return std::make_unique<context::macro_param_data_composite>(std::move(sublist));
}

context::macro_data_ptr macro_processor::create_macro_data(semantics::concat_chain::const_iterator begin,
    semantics::concat_chain::const_iterator end,
    const ranged_diagnostic_collector& add_diagnostic)
{
    auto tmp = semantics::concatenation_point::contains_var_sym(begin, end);
    if (tmp)
    {
        add_diagnostic(diagnostic_op::error_E064);
        return std::make_unique<context::macro_param_data_dummy>();
    }

    auto f = [](semantics::concat_chain::const_iterator b, semantics::concat_chain::const_iterator e) {
        return semantics::concatenation_point::to_string(b, e);
    };
    return create_macro_data_inner(begin, end, f);
}

context::macro_data_ptr macro_processor::create_macro_data(semantics::concat_chain::const_iterator begin,
    semantics::concat_chain::const_iterator end,
    expressions::evaluation_context& eval_ctx)
{
    auto f = [&eval_ctx](semantics::concat_chain::const_iterator b, semantics::concat_chain::const_iterator e) {
        return semantics::concatenation_point::evaluate(b, e, eval_ctx);
    };
    return create_macro_data_inner(begin, end, f);
}

} // namespace hlasm_plugin::parser_library::processing
