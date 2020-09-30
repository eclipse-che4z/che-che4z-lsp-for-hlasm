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

#include "occurence_collector.h"

#include "processing/context_manager.h"

namespace hlasm_plugin::parser_library::processing {

void occurence_collector::visit(const semantics::empty_operand& op) {}

void occurence_collector::visit(const semantics::model_operand& op)
{
    auto tmp = get_occurences(collector_kind_, op.chain);
    occurences_.insert(occurences_.end(), std::make_move_iterator(tmp.begin()), std::make_move_iterator(tmp.end()));
}

void occurence_collector::visit(const semantics::expr_machine_operand& op) { op.}

std::vector<symbol_occurence> get_var_occurence(const semantics::variable_symbol& var)
{
    if (var.created)
        return occurence_collector::get_occurences(occurence_kind::VAR, var.access_created()->created_name);
    return { symbol_occurence { occurence_kind::VAR, var.access_basic()->name, var.symbol_range } };
}

std::vector<symbol_occurence> occurence_collector::get_occurences(
    occurence_kind kind, const semantics::concat_chain& chain)
{
    std::vector<symbol_occurence> occurences, tmp;

    for (const auto& point : chain)
    {
        switch (point->type)
        {
            case semantics::concat_type::STR:
                /* TODO add range to str_conc struct
                if (kind == occurence_kind::ORD)
                {
                    auto [valid, name] =
                processing::context_manager(hlasm_ctx).try_get_symbol_name(point->access_str()->value); if (valid)
                        occurences.push_back(
                            symbol_occurence { occurence_kind::ORD, name, point-> });
                }*/
                break;
            case semantics::concat_type::VAR:
                if (kind == occurence_kind::VAR)
                {
                    auto tmp = get_var_occurence(*point->access_var()->access_var()->symbol);
                    occurences.insert(
                        occurences.end(), std::make_move_iterator(tmp.begin()), std::make_move_iterator(tmp.end()));
                }
                break;
            case semantics::concat_type::SUB:
                for (const auto& ch : point->access_sub()->list)
                {
                    auto tmp = get_occurences(kind, ch);
                    occurences.insert(
                        occurences.end(), std::make_move_iterator(tmp.begin()), std::make_move_iterator(tmp.end()));
                }
                break;
            default:
                break;
        }
    }
    return occurences;
}

} // namespace hlasm_plugin::parser_library::processing
