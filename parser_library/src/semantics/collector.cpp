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

const operands_si& collector::current_operands() { return *op_; }

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

void collector::set_label_field(const std::string* label, antlr4::ParserRuleContext* parser_ctx, range symbol_range)
{
    if (lbl_)
        throw std::runtime_error("field already assigned");
    // recognise, whether label consists only of ORDSYMBOL token
    if (!parser_ctx
        || (parser_ctx->getStart() == parser_ctx->getStop()
            && parser_ctx->getStart()->getType() == lexing::lexer::Tokens::ORDSYMBOL))
    {
        add_lsp_symbol(label, symbol_range, context::symbol_type::ord);
        lbl_.emplace(symbol_range, *label);
    }
    // otherwise it is macro label parameter
    else
    {
        lbl_.emplace(symbol_range, *label, label_si::mac_flag());
    }
}

void collector::set_label_field(concat_chain label, range symbol_range)
{
    if (lbl_)
        throw std::runtime_error("field already assigned");

    if (label.size() == 1 && label[0]->type == concat_type::VAR) // label is variable symbol
    {
        lbl_.emplace(symbol_range, std::move(label[0]->access_var()->symbol));
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
    rem_.emplace(symbol_range, std::vector<range>());
    def_.emplace("", symbol_range);

    add_operand_remark_hl_symbols();
}

void collector::set_operand_remark_field(std::string deferred, std::vector<range> remarks, range symbol_range)
{
    if (op_ || rem_ || def_)
        throw std::runtime_error("field already assigned");
    def_.emplace(std::move(deferred), symbol_range);
    rem_.emplace(symbol_range, std::move(remarks));

    add_operand_remark_hl_symbols();
}

void collector::set_operand_remark_field(
    std::vector<operand_ptr> operands, std::vector<range> remarks, range symbol_range)
{
    if (op_ || rem_ || def_)
        throw std::runtime_error("field already assigned");
    if (operands.size() == 1 && (!operands.front() || operands.front()->type == semantics::operand_type::EMPTY))
        operands.clear();
    op_.emplace(symbol_range, std::move(operands));
    rem_.emplace(symbol_range, std::move(remarks));

    add_operand_remark_hl_symbols();
}

void collector::add_lsp_symbol(const std::string* name, range symbol_range, context::symbol_type type)
{
    lsp_symbols_.push_back({ name, symbol_range, type });
}

void collector::add_hl_symbol(token_info symbol) { hl_symbols_.push_back(std::move(symbol)); }

void collector::clear_hl_lsp_symbols()
{
    lsp_symbols_.clear();
    hl_symbols_.clear();
}

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
    for (auto& symbol : c.lsp_symbols_)
        lsp_symbols_.push_back(std::move(symbol));
}

const instruction_si& collector::peek_instruction() { return *instr_; }

context::shared_stmt_ptr collector::extract_statement(processing::processing_status status, range& statement_range)
{
    if (!lbl_)
        lbl_.emplace(statement_range);
    if (!instr_)
        instr_.emplace(statement_range);
    if (!rem_)
        rem_.emplace(statement_range, remark_list {});

    bool deferred_hint = status.first.form == processing::processing_form::DEFERRED;

    assert(!deferred_hint || !(op_ && !op_->value.empty()));

    if (deferred_hint)
    {
        if (!def_)
            def_.emplace("", instr_.value().field_range);
        return std::make_shared<statement_si_deferred>(
            range_provider::union_range(lbl_.value().field_range, def_->second),
            std::move(*lbl_),
            std::move(*instr_),
            std::move(def_.value().first),
            def_.value().second);
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

        statement_range = range_provider::union_range(lbl_->field_range, op_->field_range);
        auto stmt_si = std::make_shared<statement_si>(
            statement_range, std::move(*lbl_), std::move(*instr_), std::move(*op_), std::move(*rem_));
        return std::make_shared<processing::resolved_statement_impl>(std::move(stmt_si), std::move(status));
    }
}

std::vector<context::lsp_symbol> collector::extract_lsp_symbols()
{
    if (lsp_symbols_extracted_)
        throw std::runtime_error("data already extracted");

    lsp_symbols_extracted_ = true;
    return std::move(lsp_symbols_);
}

std::vector<token_info> collector::extract_hl_symbols()
{
    if (hl_symbols_extracted_)
        throw std::runtime_error("data already extracted");

    hl_symbols_extracted_ = true;
    return std::move(hl_symbols_);
}

void collector::prepare_for_next_statement()
{
    lbl_.reset();
    instr_.reset();
    op_.reset();
    rem_.reset();
    def_.reset();

    lsp_symbols_.clear();
    hl_symbols_.clear();
    lsp_symbols_extracted_ = false;
    hl_symbols_extracted_ = false;
}
