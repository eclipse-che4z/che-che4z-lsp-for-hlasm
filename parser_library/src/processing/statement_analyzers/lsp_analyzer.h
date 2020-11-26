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

    bool in_macro_;
    lsp::file_occurences_t macro_occurences_;

    lsp::file_occurences_t opencode_occurences_;
    lsp::vardef_storage opencode_var_defs_;

public:
    lsp_analyzer(context::hlasm_context& hlasm_ctx, lsp::lsp_context& lsp_ctx);

    virtual void analyze(const context::hlasm_statement& statement,
        statement_provider_kind prov_kind,
        processing_kind proc_kind) override;

    virtual void macrodef_started(const macrodef_start_data& data) override;
    virtual void macrodef_finished(context::macro_def_ptr macrodef, macrodef_processing_result&& result) override;

    virtual void copydef_started(const copy_start_data& data) override;
    virtual void copydef_finished(context::copy_member_ptr copydef, copy_processing_result&& result) override;

    virtual void opencode_finished() override;

private:
    void collect_occurences(lsp::occurence_kind kind, const context::hlasm_statement& statement);

    void collect_occurence(const semantics::label_si& label, occurence_collector& collector);
    void collect_occurence(const semantics::instruction_si& instruction, occurence_collector& collector);
    void collect_occurence(const semantics::operands_si& operands, occurence_collector& collector);
    void collect_occurence(const semantics::deferred_operands_si& operands, occurence_collector& collector);

    void collect_var_definition(const context::hlasm_statement& statement);

    void collect_SET_defs(const processing::resolved_statement& statement, context::SET_t_enum type);
    void collect_LCL_GBL_defs(const processing::resolved_statement& statement, context::SET_t_enum type, bool global);
    void add_var_def(const semantics::variable_symbol* var, context::SET_t_enum type, bool global);
};

} // namespace hlasm_plugin::parser_library::processing

#endif
