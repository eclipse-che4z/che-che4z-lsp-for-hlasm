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

namespace hlasm_plugin::parser_library::context {
class set_symbol_base;
} // namespace hlasm_plugin::parser_library::context

namespace hlasm_plugin::parser_library::processing {
class processing_state_listener;
class opencode_provider;

// processor of conditional assembly instructions
class ca_processor : public instruction_processor
{
    using process_table_t =
        std::unordered_map<context::id_index, std::function<void(const processing::resolved_statement&)>>;

    const process_table_t table_;
    processing_state_listener& listener_;

public:
    ca_processor(const analyzing_context& ctx,
        branching_provider& branch_provider,
        parse_lib_provider& lib_provider,
        processing_state_listener& listener,
        opencode_provider& open_code);

    void process(std::shared_ptr<const processing::resolved_statement> stmt) override;

private:
    opencode_provider* open_code_;

    process_table_t create_table();

    void register_seq_sym(const processing::resolved_statement& stmt);

    struct SET_info
    {
        context::set_symbol_base* symbol = nullptr;
        context::id_index name;
        context::A_t index = 0;
    };

    struct GLB_LCL_info
    {
        context::id_index id;
        bool scalar;
        range r;

        GLB_LCL_info(context::id_index id, bool scalar, range r)
            : id(id)
            , scalar(scalar)
            , r(r)
        {}
    };
    std::vector<GLB_LCL_info> m_glb_lcl_work;
    std::vector<expressions::ca_expression*> m_set_work;

    template<typename T>
    SET_info get_SET_symbol(const processing::resolved_statement& stmt);
    bool prepare_SET_operands(
        const processing::resolved_statement& stmt, std::vector<expressions::ca_expression*>& expr_values);

    template<typename T>
    void process_SET(const processing::resolved_statement& stmt);

    bool prepare_GBL_LCL(const processing::resolved_statement& stmt, std::vector<GLB_LCL_info>& info) const;

    template<typename T, bool global>
    void process_GBL_LCL(const processing::resolved_statement& stmt);

    void process_ANOP(const processing::resolved_statement& stmt);

    void process_ACTR(const processing::resolved_statement& stmt);

    const semantics::seq_sym* prepare_AGO(const processing::resolved_statement& stmt);
    void process_AGO(const processing::resolved_statement& stmt);

    const semantics::seq_sym* prepare_AIF(const processing::resolved_statement& stmt);
    void process_AIF(const processing::resolved_statement& stmt);

    void process_MACRO(const processing::resolved_statement& stmt);
    void process_MEXIT(const processing::resolved_statement& stmt);
    void process_MEND(const processing::resolved_statement& stmt);
    void process_AEJECT(const processing::resolved_statement& stmt);
    void process_ASPACE(const processing::resolved_statement& stmt);
    void process_AREAD(const processing::resolved_statement& stmt);

    void process_empty(const processing::resolved_statement&);

    void process_MHELP(const processing::resolved_statement& stmt);
};

} // namespace hlasm_plugin::parser_library::processing

#endif
