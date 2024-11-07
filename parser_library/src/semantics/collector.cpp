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

#include "collector.h"

#include "antlr4-runtime.h"

#include "expressions/data_definition.h"
#include "lexing/lexer.h"
#include "operand_impls.h"
#include "processing/statement.h"
#include "range_provider.h"

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::semantics;


collector::collector()
    : lsp_symbols_extracted_(false)
    , hl_symbols_extracted_(false)
{}

const label_si& collector::current_label() { return *lbl_; }

bool collector::has_label() const { return lbl_.has_value(); }

const instruction_si& collector::current_instruction() { return *instr_; }

bool collector::has_instruction() const { return instr_.has_value(); }


const operands_si& collector::current_operands() const { return *op_; }
operands_si& collector::current_operands() { return *op_; }

bool collector::has_operands() const { return op_.has_value(); }

const remarks_si& collector::current_remarks() { return *rem_; }

void collector::set_label_field(range symbol_range)
{
    if (lbl_)
        throw std::runtime_error("field already assigned");
    lbl_.emplace(symbol_range);
}

void collector::set_label_field(std::string label, range symbol_range)
{
    if (lbl_)
        throw std::runtime_error("field already assigned");
    lbl_.emplace(symbol_range, std::move(label), label_si::mac_flag());
}

void collector::set_label_field(seq_sym sequence_symbol, range symbol_range)
{
    if (lbl_)
        throw std::runtime_error("field already assigned");
    lbl_.emplace(symbol_range, std::move(sequence_symbol));
}

void collector::set_label_field(
    context::id_index label, std::string mixed_case_label, antlr4::ParserRuleContext* parser_ctx, range symbol_range)
{
    if (lbl_)
        throw std::runtime_error("field already assigned");
    // recognise, whether label consists only of ORDSYMBOL token
    if (!parser_ctx
        || ((parser_ctx->getStart() == parser_ctx->getStop()
                || parser_ctx->getStart()->getTokenIndex() == parser_ctx->getStop()->getTokenIndex())
            && parser_ctx->getStart()->getType() == lexing::lexer::Tokens::ORDSYMBOL))
    {
        lbl_.emplace(symbol_range, ord_symbol_string { label, std::move(mixed_case_label) });
    }
    // otherwise it is macro label parameter
    else
    {
        lbl_.emplace(symbol_range, mixed_case_label, label_si::mac_flag());
    }
}

void collector::set_label_field(concat_chain label, range symbol_range)
{
    if (lbl_)
        throw std::runtime_error("field already assigned");

    if (concat_chain_matches<var_sym_conc>(label)) // label is variable symbol
    {
        lbl_.emplace(symbol_range, std::move(std::get<var_sym_conc>(label[0].value).symbol));
    }
    else // label is concatenation
    {
        lbl_.emplace(symbol_range, std::move(label));
    }
}

void collector::set_instruction_field(range symbol_range)
{
    if (instr_)
        throw std::runtime_error("field already assigned");
    instr_.emplace(symbol_range);
}

void collector::set_instruction_field(context::id_index instr, range symbol_range)
{
    if (instr_)
        throw std::runtime_error("field already assigned");
    instr_.emplace(symbol_range, instr);
}

void collector::set_instruction_field(concat_chain instr, range symbol_range)
{
    if (instr_)
        throw std::runtime_error("field already assigned");

    instr_.emplace(symbol_range, std::move(instr));
}

void collector::set_operand_remark_field(range symbol_range)
{
    if (op_ || rem_ || def_)
        throw std::runtime_error("field already assigned");
    op_.emplace(symbol_range, operand_list());
    rem_.emplace(std::vector<range>());
    def_.emplace(symbol_range, 0, lexing::u8string_with_newlines(), std::vector<vs_ptr>());

    add_operand_remark_hl_symbols();
}

void collector::set_operand_remark_field(lexing::u8string_with_newlines deferred,
    std::vector<vs_ptr> vars,
    remark_list remarks,
    range symbol_range,
    size_t logical_column)
{
    if (op_ || rem_ || def_)
        throw std::runtime_error("field already assigned");
    def_.emplace(symbol_range, logical_column, std::move(deferred), std::move(vars));
    rem_.emplace(std::move(remarks));

    add_operand_remark_hl_symbols();
}

void collector::set_operand_remark_field(operand_list operands, remark_list remarks, range symbol_range)
{
    if (op_ || rem_ || def_)
        throw std::runtime_error("field already assigned");
    if (operands.size() == 1 && (!operands.front() || operands.front()->type == semantics::operand_type::EMPTY))
        operands.clear();
    op_.emplace(symbol_range, std::move(operands));
    rem_.emplace(std::move(remarks));

    add_operand_remark_hl_symbols();
}

void collector::add_hl_symbol(token_info symbol) { hl_symbols_.push_back(std::move(symbol)); }

void collector::clear_hl_symbols() { hl_symbols_.clear(); }

void collector::add_operand_remark_hl_symbols()
{
    if (rem_)
    {
        for (const auto& remark : current_remarks().value)
            add_hl_symbol(token_info(remark, hl_scopes::remark));
    }
}

void collector::append_operand_field(collector&& c)
{
    if (c.op_)
        op_.emplace(std::move(*c.op_));
    if (c.rem_)
        rem_.emplace(std::move(*c.rem_));
    if (c.def_)
        def_.emplace(std::move(*c.def_));

    for (auto& symbol : c.hl_symbols_)
        hl_symbols_.push_back(std::move(symbol));

    statement_diagnostics.diags.insert(statement_diagnostics.diags.end(),
        std::make_move_iterator(c.statement_diagnostics.diags.begin()),
        std::make_move_iterator(c.statement_diagnostics.diags.end()));

    lit_.insert(lit_.end(), std::make_move_iterator(c.lit_.begin()), std::make_move_iterator(c.lit_.end()));
}

// struct holding full semantic information (si) about whole instruction statement, whole logical line
struct statement_si final : public processing::resolved_statement
{
    statement_si(range stmt_range,
        label_si label,
        instruction_si instruction,
        operands_si operands,
        remarks_si remarks,
        std::vector<semantics::literal_si>&& literals,
        processing::processing_status status,
        std::vector<diagnostic_op>&& diags)
        : label(std::move(label))
        , instruction(std::move(instruction))
        , operands(std::move(operands))
        , remarks(std::move(remarks))
        , collected_literals(std::make_move_iterator(literals.begin()), std::make_move_iterator(literals.end()))
        , status(std::move(status))
        , statement_diagnostics(std::make_move_iterator(diags.begin()), std::make_move_iterator(diags.end()))
        , stmt_range(stmt_range)
    {}

    label_si label;
    instruction_si instruction;
    operands_si operands;
    remarks_si remarks;
    std::vector<semantics::literal_si> collected_literals;
    processing::processing_status status;
    std::vector<diagnostic_op> statement_diagnostics;
    range stmt_range;

    const range& stmt_range_ref() const override { return stmt_range; }
    const label_si& label_ref() const override { return label; }
    const instruction_si& instruction_ref() const override { return instruction; }
    const operands_si& operands_ref() const override { return operands; }
    std::span<const semantics::literal_si> literals() const override { return collected_literals; }
    const remarks_si& remarks_ref() const override { return remarks; }

    const processing::op_code& opcode_ref() const override { return status.second; }
    processing::processing_format format_ref() const override { return status.first; }
    std::span<const diagnostic_op> diagnostics() const override
    {
        return { statement_diagnostics.data(), statement_diagnostics.data() + statement_diagnostics.size() };
    }
};

context::shared_stmt_ptr collector::extract_statement(processing::processing_status status, range& statement_range)
{
    if (!lbl_)
        lbl_.emplace(statement_range);
    if (!instr_)
        instr_.emplace(statement_range);
    if (!rem_)
        rem_.emplace(remark_list {});

    bool deferred_hint = status.first.form == processing::processing_form::DEFERRED;

    assert(!deferred_hint || !(op_ && !op_->value.empty()));

    if (deferred_hint)
    {
        assert(lit_.empty());
        if (!def_)
            def_.emplace(instr_->field_range, 0, lexing::u8string_with_newlines(), std::vector<vs_ptr>());
        return std::make_shared<deferred_statement>(union_range(lbl_->field_range, def_->field_range),
            std::move(*lbl_),
            std::move(*instr_),
            std::move(*def_),
            std::move(statement_diagnostics.diags),
            statement_diagnostics_without_operands);
    }
    else
    {
        if (!op_)
            op_.emplace(instr_.value().field_range, operand_list {});

        // foreach operand substitute null with empty
        for (size_t i = 0; i < op_->value.size(); i++)
        {
            if (!op_->value[i])
                op_->value[i] = std::make_unique<empty_operand>(instr_.value().field_range);
        }

        return std::make_shared<statement_si>(union_range(lbl_->field_range, op_->field_range),
            std::move(*lbl_),
            std::move(*instr_),
            std::move(*op_),
            std::move(*rem_),
            std::move(lit_),
            std::move(status),
            std::move(statement_diagnostics.diags));
    }
}

std::span<const token_info> collector::extract_hl_symbols()
{
    if (hl_symbols_extracted_)
        throw std::runtime_error("data already extracted");

    hl_symbols_extracted_ = true;
    return hl_symbols_;
}

void collector::set_hl_symbols(std::span<const token_info> m) { hl_symbols_.assign(m.begin(), m.end()); }

void collector::prepare_for_next_statement()
{
    lbl_.reset();
    instr_.reset();
    op_.reset();
    rem_.reset();
    lit_.clear();
    def_.reset();

    hl_symbols_.clear();
    lsp_symbols_extracted_ = false;
    hl_symbols_extracted_ = false;

    statement_diagnostics.diags.clear();
    statement_diagnostics_without_operands = 0;
}

std::shared_ptr<literal_si_data> collector::add_literal(std::string text, expressions::data_definition dd, range r)
{
    return lit_.emplace_back(std::make_shared<literal_si_data>(std::move(text), std::move(dd), r));
}

std::vector<literal_si> hlasm_plugin::parser_library::semantics::collector::take_literals() { return std::move(lit_); }

void hlasm_plugin::parser_library::semantics::collector::set_literals(std::vector<literal_si> lit)
{
    lit_ = std::move(lit);
}

void collector::resolve_first_part()
{
    if (lbl_)
        lbl_->resolve(statement_diagnostics);
    if (instr_)
        instr_->resolve(statement_diagnostics);
}

void hlasm_plugin::parser_library::semantics::collector::starting_operand_parsing()
{
    statement_diagnostics_without_operands = statement_diagnostics.diags.size();
}
