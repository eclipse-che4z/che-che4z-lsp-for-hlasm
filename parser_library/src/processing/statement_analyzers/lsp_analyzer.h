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

#ifndef PROCESSING_LSP_ANALYZER_H
#define PROCESSING_LSP_ANALYZER_H

#include "lsp/lsp_context.h"
#include "occurence_collector.h"
#include "processing/processing_format.h"
#include "processing/statement.h"
#include "statement_analyzer.h"

namespace hlasm_plugin::parser_library::processing {

class lsp_analyzer : public statement_analyzer
{
    context::hlasm_context& hlasm_ctx_;
    lsp::lsp_context& lsp_ctx_;
    // text of the file this analyzer is assigned to
    const std::string& file_text_;

    bool in_macro_ = false;
    size_t macro_nest_ = 1;
    lsp::file_occurences_t macro_occurences_;

    lsp::file_occurences_t opencode_occurences_;
    lsp::vardef_storage opencode_var_defs_;

    lsp::occurence_storage stmt_occurences_;

public:
    lsp_analyzer(context::hlasm_context& hlasm_ctx, lsp::lsp_context& lsp_ctx, const std::string& file_text);

    void analyze(const context::hlasm_statement& statement,
        statement_provider_kind prov_kind,
        processing_kind proc_kind) override;

    void macrodef_started(const macrodef_start_data& data);
    void macrodef_finished(context::macro_def_ptr macrodef, macrodef_processing_result&& result);

    void copydef_finished(context::copy_member_ptr copydef, copy_processing_result&& result);

    void opencode_finished();

private:
    void assign_statement_occurences();

    void collect_occurences(lsp::occurence_kind kind, const context::hlasm_statement& statement);

    void collect_occurence(const semantics::label_si& label, occurence_collector& collector);
    void collect_occurence(const semantics::instruction_si& instruction, occurence_collector& collector);
    void collect_occurence(const semantics::operands_si& operands, occurence_collector& collector);
    void collect_occurence(const semantics::deferred_operands_si& operands, occurence_collector& collector);

    void collect_var_definition(const processing::resolved_statement& statement);
    void collect_copy_operands(const processing::resolved_statement& statement);

    void collect_SET_defs(const processing::resolved_statement& statement, context::SET_t_enum type);
    void collect_LCL_GBL_defs(const processing::resolved_statement& statement, context::SET_t_enum type, bool global);
    void add_var_def(const semantics::variable_symbol* var, context::SET_t_enum type, bool global);

    void add_copy_operand(context::id_index name, const range& operand_range);

    void update_macro_nest(const processing::resolved_statement& statement);

    struct LCL_GBL_instr
    {
        context::id_index name;
        context::SET_t_enum type;
        bool global;
    };
    std::array<LCL_GBL_instr, 6> LCL_GBL_instructions_;
    std::array<std::pair<context::id_index, context::SET_t_enum>, 3> SET_instructions_;
    bool is_LCL_GBL(const processing::resolved_statement& statement, context::SET_t_enum& set_type, bool& global) const;
    bool is_SET(const processing::resolved_statement& statement, context::SET_t_enum& set_type) const;
};

} // namespace hlasm_plugin::parser_library::processing

#endif
