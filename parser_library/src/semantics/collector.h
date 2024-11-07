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

#ifndef SEMANTICS_COLLECTOR_H
#define SEMANTICS_COLLECTOR_H

#include <optional>
#include <span>

#include "ParserRuleContext.h"
#include "processing/op_code.h"
#include "source_info_processor.h"
#include "statement.h"

namespace hlasm_plugin::parser_library::expressions {
struct data_definition;
} // namespace hlasm_plugin::parser_library::expressions

namespace hlasm_plugin::parser_library::semantics {

// class containing methods for collecting parsed statement fields
class collector
{
public:
    collector();
    const label_si& current_label();
    bool has_label() const;
    const instruction_si& current_instruction();
    bool has_instruction() const;
    const operands_si& current_operands() const;
    operands_si& current_operands();
    bool has_operands() const;
    const remarks_si& current_remarks();

    void set_label_field(range symbol_range);
    void set_label_field(std::string label, range symbol_range);
    void set_label_field(seq_sym sequence_symbol, range symbol_range);
    void set_label_field(
        context::id_index label, std::string mixed_case_label, antlr4::ParserRuleContext* ctx, range symbol_range);
    void set_label_field(concat_chain label, range symbol_range);

    void set_instruction_field(range symbol_range);
    void set_instruction_field(context::id_index instr, range symbol_range);
    void set_instruction_field(concat_chain instr, range symbol_range);

    void set_operand_remark_field(range symbol_range);
    void set_operand_remark_field(lexing::u8string_with_newlines deferred,
        std::vector<vs_ptr> vars,
        remark_list remarks,
        range symbol_range,
        size_t logical_column);
    void set_operand_remark_field(operand_list operands, remark_list remarks, range symbol_range);

    void add_hl_symbol(token_info symbol);
    void clear_hl_symbols();

    void append_operand_field(collector&& c);

    context::shared_stmt_ptr extract_statement(processing::processing_status status, range& statement_range);
    std::span<const token_info> extract_hl_symbols();
    void set_hl_symbols(std::span<const token_info>);
    void prepare_for_next_statement();

    diagnostic_op_consumer* diag_collector() { return &statement_diagnostics; }
    diagnostic_op_consumer_container& diag_container() { return statement_diagnostics; }

    std::shared_ptr<literal_si_data> add_literal(std::string text, expressions::data_definition dd, range r);
    std::vector<literal_si> take_literals();
    void set_literals(std::vector<literal_si> lit);

    void resolve_first_part();

    void starting_operand_parsing();

private:
    std::optional<label_si> lbl_;
    std::optional<instruction_si> instr_;
    std::optional<operands_si> op_;
    std::optional<remarks_si> rem_;
    std::vector<literal_si> lit_;
    std::optional<deferred_operands_si> def_;
    std::vector<token_info> hl_symbols_;
    bool lsp_symbols_extracted_;
    bool hl_symbols_extracted_;

    diagnostic_op_consumer_container statement_diagnostics;
    size_t statement_diagnostics_without_operands = 0;

    void add_operand_remark_hl_symbols();
};

} // namespace hlasm_plugin::parser_library::semantics
#endif
