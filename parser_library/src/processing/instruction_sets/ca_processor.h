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

#ifndef PROCESSING_CA_PROCESSOR_H
#define PROCESSING_CA_PROCESSOR_H

#include "instruction_processor.h"
#include "processing/context_manager.h"
#include "processing/processing_state_listener.h"

namespace hlasm_plugin::parser_library::processing {

// processor of conditional assembly instructions
class ca_processor : public instruction_processor
{
    using process_table_t =
        std::unordered_map<context::id_index, std::function<void(const semantics::complete_statement&)>>;

    const process_table_t table_;
    processing_state_listener& listener_;

public:
    ca_processor(context::hlasm_context& hlasm_ctx,
        branching_provider& branch_provider,
        workspaces::parse_lib_provider& lib_provider,
        processing_state_listener& listener);

    virtual void process(context::shared_stmt_ptr stmt) override;

private:
    process_table_t create_table(context::hlasm_context& hlasm_ctx);

    void register_seq_sym(const semantics::complete_statement& stmt);

    bool test_symbol_for_assignment(const semantics::variable_symbol* symbol,
        context::SET_t_enum type,
        int& idx,
        context::set_symbol_base*& set_symbol,
        context::id_index& name);
    bool prepare_SET_symbol(const semantics::complete_statement& stmt,
        context::SET_t_enum type,
        int& idx,
        context::set_symbol_base*& set_symbol,
        context::id_index& name);
    bool prepare_SET_operands(
        const semantics::complete_statement& stmt, std::vector<expressions::ca_expression*>& expr_values);

    template<typename T>
    void process_SET(const semantics::complete_statement& stmt);

    bool prepare_GBL_LCL(
        const semantics::complete_statement& stmt, std::vector<context::id_index>& ids, std::vector<bool>& scalar_info);

    template<typename T, bool global>
    void process_GBL_LCL(const semantics::complete_statement& stmt);

    void process_ANOP(const semantics::complete_statement& stmt);

    bool prepare_ACTR(const semantics::complete_statement& stmt, context::A_t& ctr);
    void process_ACTR(const semantics::complete_statement& stmt);

    bool prepare_AGO(const semantics::complete_statement& stmt,
        context::A_t& branch,
        std::vector<std::pair<context::id_index, range>>& targets);
    void process_AGO(const semantics::complete_statement& stmt);

    bool prepare_AIF(const semantics::complete_statement& stmt,
        context::B_t& condition,
        context::id_index& target,
        range& target_range);
    void process_AIF(const semantics::complete_statement& stmt);

    void process_MACRO(const semantics::complete_statement& stmt);
    void process_MEXIT(const semantics::complete_statement& stmt);
    void process_MEND(const semantics::complete_statement& stmt);
    void process_AEJECT(const semantics::complete_statement& stmt);
    void process_ASPACE(const semantics::complete_statement& stmt);
    void process_AREAD(const semantics::complete_statement& stmt);

    void process_empty(const semantics::complete_statement&);
};

template<typename T>
inline void ca_processor::process_SET(const semantics::complete_statement& stmt)
{
    std::vector<expressions::ca_expression*> expr_values;
    int index;
    context::id_index name;
    context::set_symbol_base* set_symbol;
    bool ok = prepare_SET_symbol(stmt, context::object_traits<T>::type_enum, index, set_symbol, name);

    if (!ok)
        return;

    if (!set_symbol)
        set_symbol = hlasm_ctx.create_local_variable<T>(name, index == -1).get();

    ok = prepare_SET_operands(stmt, expr_values);

    if (!ok)
        return;

    for (size_t i = 0; i < expr_values.size(); i++)
        set_symbol->access_set_symbol<T>()->set_value(expr_values[i]->evaluate<T>(eval_ctx), index - 1 + i);
}

template<typename T, bool global>
inline void ca_processor::process_GBL_LCL(const semantics::complete_statement& stmt)
{
    register_seq_sym(stmt);

    std::vector<context::id_index> ids;
    std::vector<bool> scalar_info;
    bool ok = prepare_GBL_LCL(stmt, ids, scalar_info);

    if (!ok)
        return;

    for (size_t i = 0; i < ids.size(); ++i)
    {
        if (global)
            hlasm_ctx.create_global_variable<T>(ids[i], scalar_info[i]);
        else
            hlasm_ctx.create_local_variable<T>(ids[i], scalar_info[i]);
    }
}

} // namespace hlasm_plugin::parser_library::processing

#endif