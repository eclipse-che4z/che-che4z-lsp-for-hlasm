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

#include "antlr4-runtime.h"

#include "source_info_processor.h"
#include "processing/op_code.h"
#include "statement.h"

namespace hlasm_plugin {
namespace parser_library {
namespace semantics {

// class containing methods for collecting parsed statement fields
class collector
{
public:
    collector();
    const label_si& current_label();
    bool has_label() const;
    const instruction_si& current_instruction();
    bool has_instruction() const;
    const operands_si& current_operands();
    const remarks_si& current_remarks();

    void set_label_field(range symbol_range);
    void set_label_field(std::string label, range symbol_range);
    void set_label_field(seq_sym sequence_symbol, range symbol_range);
    void set_label_field(const std::string* label, antlr4::ParserRuleContext* ctx, range symbol_range);
    void set_label_field(concat_chain label, range symbol_range);

    void set_instruction_field(range symbol_range);
    void set_instruction_field(context::id_index instr, range symbol_range);
    void set_instruction_field(concat_chain instr, range symbol_range);

    void set_operand_remark_field(range symbol_range);
    void set_operand_remark_field(
        std::string deferred, std::vector<vs_ptr> vars, remark_list remarks, range symbol_range);
    void set_operand_remark_field(operand_list operands, remark_list remarks, range symbol_range);

    void add_hl_symbol(token_info symbol);
    void clear_hl_symbols();

    void append_operand_field(collector&& c);

    const instruction_si& peek_instruction();
    context::shared_stmt_ptr extract_statement(processing::processing_status status, range& statement_range);
    std::vector<token_info> extract_hl_symbols();
    void prepare_for_next_statement();

private:
    std::optional<label_si> lbl_;
    std::optional<instruction_si> instr_;
    std::optional<operands_si> op_;
    std::optional<remarks_si> rem_;
    std::optional<deferred_operands_si> def_;
    std::vector<token_info> hl_symbols_;
    bool lsp_symbols_extracted_;
    bool hl_symbols_extracted_;

    void add_operand_remark_hl_symbols();
};

} // namespace semantics
} // namespace parser_library
} // namespace hlasm_plugin
#endif
