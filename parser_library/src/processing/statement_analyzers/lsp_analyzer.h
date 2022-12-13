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

#include <array>
#include <string>

#include "context/common_types.h"
#include "context/copy_member.h"
#include "context/macro.h"
#include "lsp/macro_info.h"
#include "lsp/symbol_occurence.h"
#include "processing/processing_format.h"
#include "processing/statement_providers/statement_provider_kind.h"
#include "statement_analyzer.h"

namespace hlasm_plugin::parser_library {
struct range;
} // namespace hlasm_plugin::parser_library

namespace hlasm_plugin::parser_library::context {
class hlasm_context;
class id_index;
} // namespace hlasm_plugin::parser_library::context

namespace hlasm_plugin::parser_library::lsp {
class lsp_context;
} // namespace hlasm_plugin::parser_library::lsp

namespace hlasm_plugin::parser_library::processing {
struct copy_processing_result;
struct macrodef_processing_result;
struct macrodef_start_data;
struct resolved_statement;

class occurence_collector;
} // namespace hlasm_plugin::parser_library::processing

namespace hlasm_plugin::parser_library::semantics {
struct deferred_operands_si;
struct instruction_si;
struct label_si;
struct operands_si;
struct preprocessor_statement_si;
struct variable_symbol;
} // namespace hlasm_plugin::parser_library::semantics

namespace hlasm_plugin::utils::resource {
class resource_location;
} // namespace hlasm_plugin::utils::resource

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
        processing_kind proc_kind,
        bool evaluated_model) override;

    void analyze(const semantics::preprocessor_statement_si& statement);

    void macrodef_started(const macrodef_start_data& data);
    void macrodef_finished(context::macro_def_ptr macrodef, macrodef_processing_result&& result);

    void copydef_finished(context::copy_member_ptr copydef, copy_processing_result&& result);

    void opencode_finished();

private:
    void assign_statement_occurences(const utils::resource::resource_location& doc_location);

    void collect_occurences(lsp::occurence_kind kind, const context::hlasm_statement& statement, bool evaluated_model);
    void collect_occurences(lsp::occurence_kind kind, const semantics::preprocessor_statement_si& statement);

    void collect_occurence(const semantics::label_si& label, occurence_collector& collector);
    void collect_occurence(const semantics::instruction_si& instruction, occurence_collector& collector);
    void collect_occurence(const semantics::operands_si& operands, occurence_collector& collector);
    void collect_occurence(const semantics::deferred_operands_si& operands, occurence_collector& collector);

    void collect_var_definition(const processing::resolved_statement& statement);
    void collect_copy_operands(const processing::resolved_statement& statement, bool evaluated_model);

    void collect_SET_defs(const processing::resolved_statement& statement, context::SET_t_enum type);
    void collect_LCL_GBL_defs(const processing::resolved_statement& statement, context::SET_t_enum type, bool global);
    void add_var_def(const semantics::variable_symbol* var, context::SET_t_enum type, bool global);

    void add_copy_operand(context::id_index name, const range& operand_range, bool evaluated_model);

    void update_macro_nest(const processing::resolved_statement& statement);

    bool is_LCL_GBL(const processing::resolved_statement& statement, context::SET_t_enum& set_type, bool& global) const;
    bool is_SET(const processing::resolved_statement& statement, context::SET_t_enum& set_type) const;
};

} // namespace hlasm_plugin::parser_library::processing

#endif
