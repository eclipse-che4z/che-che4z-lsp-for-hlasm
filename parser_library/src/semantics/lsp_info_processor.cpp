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

#include "lsp_info_processor.h"

#include <algorithm>
#include <sstream>

#include "context/instruction.h"

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::semantics;
using namespace hlasm_plugin::parser_library::context;

lsp_info_processor::lsp_info_processor(
    std::string file, const std::string& text, context::hlasm_context* ctx, bool collect_hl_info)
    : file_name(ctx ? ctx->ids().add(file, true) : nullptr)
    , empty_string(ctx ? ctx->ids().well_known.empty : nullptr)
    , ctx_(ctx)
    , collect_hl_info_(collect_hl_info)
    , instruction_regex("^([^*][^*]\\S*\\s+\\S+|\\s+\\S*)")
{
    // initialize text vector
    std::string line;
    std::stringstream text_ss(text);
    while (std::getline(text_ss, line))
        text_.push_back(line);

    if (!ctx)
        return;

    hl_info_.document = { *file_name };

    // initialize context
    if (!ctx_->lsp_ctx->initialized)
    {
        for (const auto& machine_instr : instruction::machine_instructions)
        {
            std::stringstream documentation(" ");
            std::stringstream detail(""); // operands used for hover - e.g. V,D12U(X,B)[,M]
            std::stringstream autocomplete(""); // operands used for autocomplete - e.g. V,D12U(X,B) [,M]
            for (size_t i = 0; i < machine_instr.second->operands.size(); i++)
            {
                const auto& op = machine_instr.second->operands[i];
                if (machine_instr.second->no_optional == 1 && machine_instr.second->operands.size() - i == 1)
                {
                    autocomplete << " [";
                    detail << "[";
                    if (i != 0)
                    {
                        autocomplete << ",";
                        detail << ",";
                    }
                    detail << op.to_string() << "]";
                    autocomplete << op.to_string() << "]";
                }
                else if (machine_instr.second->no_optional == 2 && machine_instr.second->operands.size() - i == 2)
                {
                    autocomplete << " [";
                    detail << "[";
                    if (i != 0)
                    {
                        autocomplete << ",";
                        detail << ",";
                    }
                    detail << op.to_string() << "]";
                    autocomplete << op.to_string() << "[,";
                }
                else if (machine_instr.second->no_optional == 2 && machine_instr.second->operands.size() - i == 1)
                {
                    detail << op.to_string() << "]]";
                    autocomplete << op.to_string() << "]]";
                }
                else
                {
                    if (i != 0)
                    {
                        autocomplete << ",";
                        detail << ",";
                    }
                    detail << op.to_string();
                    autocomplete << op.to_string();
                }
            }
            documentation << "Machine instruction " << std::endl
                          << "Instruction format: "
                          << instruction::mach_format_to_string.at(machine_instr.second->format);
            ctx_->lsp_ctx->all_instructions.push_back({ machine_instr.first,
                "Operands: " + detail.str(),
                machine_instr.first + "   " + autocomplete.str(),
                { documentation.str() } });
        }

        for (const auto& asm_instr : instruction::assembler_instructions)
        {
            std::stringstream documentation(" ");
            std::stringstream detail("");

            // int min_op = asm_instr.second.min_operands;
            // int max_op = asm_instr.second.max_operands;
            std::string description = asm_instr.second.description;

            detail << asm_instr.first << "   " << description;
            documentation << "Assembler instruction";
            ctx_->lsp_ctx->all_instructions.push_back(
                { asm_instr.first, detail.str(), asm_instr.first + "   " /*+ description*/, { documentation.str() } });
        }

        for (const auto& mnemonic_instr : instruction::mnemonic_codes)
        {
            std::stringstream documentation(" ");
            std::stringstream detail("");
            std::stringstream subs_ops_mnems(" ");
            std::stringstream subs_ops_nomnems(" ");

            // get mnemonic operands
            size_t iter_over_mnem = 0;

            auto instr_name = mnemonic_instr.second.instruction;
            auto mach_operands = instruction::machine_instructions[instr_name]->operands;
            auto no_optional = instruction::machine_instructions[instr_name]->no_optional;
            bool first = true;
            std::vector<std::string> mnemonic_with_operand_ommited = { "VNOT", "NOTR", "NOTGR" };

            auto replaces = mnemonic_instr.second.replaced;

            for (size_t i = 0; i < mach_operands.size(); i++)
            {
                if (replaces.size() > iter_over_mnem)
                {
                    auto [position, value] = replaces[iter_over_mnem];
                    // can still replace mnemonics
                    if (position == i)
                    {
                        // mnemonics can be substituted when no_optional is 1, but not 2 -> 2 not implemented
                        if (no_optional == 1 && mach_operands.size() - i == 1)
                        {
                            subs_ops_mnems << "[";
                            if (i != 0)
                                subs_ops_mnems << ",";
                            subs_ops_mnems << std::to_string(value) + "]";
                            continue;
                        }
                        // replace current for mnemonic
                        if (i != 0)
                            subs_ops_mnems << ",";
                        if (std::find(mnemonic_with_operand_ommited.begin(),
                                mnemonic_with_operand_ommited.end(),
                                mnemonic_instr.first)
                            != mnemonic_with_operand_ommited.end())
                        {
                            subs_ops_mnems << mach_operands[i - 1].to_string();
                        }
                        else
                            subs_ops_mnems << std::to_string(value);
                        iter_over_mnem++;
                        continue;
                    }
                }
                // do not replace by a mnemonic
                std::string curr_op_with_mnem = "";
                std::string curr_op_without_mnem = "";
                if (no_optional == 0)
                {
                    if (i != 0)
                        curr_op_with_mnem += ",";
                    if (!first)
                        curr_op_without_mnem += ",";
                    curr_op_with_mnem += mach_operands[i].to_string();
                    curr_op_without_mnem += mach_operands[i].to_string();
                }
                else if (no_optional == 1 && mach_operands.size() - i == 1)
                {
                    curr_op_with_mnem += "[";
                    curr_op_without_mnem += "[";
                    if (i != 0)
                        curr_op_with_mnem += ",";
                    if (!first)
                        curr_op_without_mnem += ",";
                    curr_op_with_mnem += mach_operands[i].to_string() + "]";
                    curr_op_without_mnem += mach_operands[i].to_string() + "]";
                }
                else if (no_optional == 2 && mach_operands.size() - i == 1)
                {
                    curr_op_with_mnem += mach_operands[i].to_string() + "]]";
                    curr_op_without_mnem += mach_operands[i].to_string() + "]]";
                }
                else if (no_optional == 2 && mach_operands.size() - i == 2)
                {
                    curr_op_with_mnem += "[";
                    curr_op_without_mnem += "[";
                    if (i != 0)
                        curr_op_with_mnem += ",";
                    if (!first)
                        curr_op_without_mnem += ",";
                    curr_op_with_mnem += mach_operands[i].to_string() + "[,";
                    curr_op_without_mnem += mach_operands[i].to_string() + "[,";
                }
                subs_ops_mnems << curr_op_with_mnem;
                subs_ops_nomnems << curr_op_without_mnem;
                first = false;
            }
            detail << "Operands: " + subs_ops_nomnems.str();
            documentation << "Mnemonic code for " << instr_name << " instruction" << std::endl
                          << "Substituted operands: " << subs_ops_mnems.str() << std::endl
                          << "Instruction format: "
                          << instruction::mach_format_to_string.at(
                                 instruction::machine_instructions[instr_name]->format);
            ctx_->lsp_ctx->all_instructions.push_back({ mnemonic_instr.first,
                detail.str(),
                mnemonic_instr.first + "   " + subs_ops_nomnems.str(),
                { documentation.str() } });
        }

        for (const auto& ca_instr : instruction::ca_instructions)
        {
            ctx_->lsp_ctx->all_instructions.push_back({ ca_instr.name, "", ca_instr.name, { "Conditional Assembly" } });
        }

        ctx_->lsp_ctx->initialized = true;
    }
};

void lsp_info_processor::process_hl_symbols(std::vector<token_info> symbols)
{
    for (const auto& symbol : symbols)
    {
        add_hl_symbol(symbol);
    }
}

void lsp_info_processor::process_lsp_symbols(std::vector<context::lsp_symbol> symbols, const std::string* given_file)
{
    if (!ctx_)
        return;

    bool only_ord = false;
    auto symbol_file = file_name;
    // if the file is given, process only ordinary symbols
    if (given_file != nullptr)
    {
        only_ord = true;
        symbol_file = given_file;
    }
    // the order of the symbols cannot change
    for (auto& symbol : symbols)
    {
        symbol.symbol_range.file = symbol_file;
        if (!only_ord || symbol.type == symbol_type::ord)
            add_lsp_symbol(symbol);
    }
    if (!only_ord && !symbols.empty())
    {
        process_instruction_sym_();
        process_var_syms_();
    }
}
template<typename T>
bool lsp_info_processor::find_definition_(
    const position& pos, const definitions<T>& symbols, position_uri_s& found) const
{
    for (const auto& symbol : symbols)
    {
        for (const auto& occ : symbol.second)
        {
            if (is_in_range_(pos, occ))
            {
                found = { *symbol.first.file_name, symbol.first.definition_range.start };
                return true;
            }
        }
    }
    return false;
}

template<typename T>
bool lsp_info_processor::find_references_(
    const position& pos, const definitions<T>& symbols, std::vector<position_uri_s>& found) const
{
    // for each symbol
    for (const auto& symbol : symbols)
    {
        // for each of its occurences
        for (const auto& occ : symbol.second)
        {
            // if at least one occurence is in range
            if (is_in_range_(pos, occ))
            {
                // return all of them as result
                for (const auto& found_occ : symbol.second)
                    found.push_back({ *found_occ.file_name, found_occ.symbol_range.start });

                return true;
            }
        }
    }
    return false;
}

completion_list_s lsp_info_processor::completion(const position& pos, const char trigger_char, int trigger_kind) const
{
    if (!ctx_->lsp_ctx || ctx_->lsp_ctx.use_count() == 0 || text_.size() == 0)
        return { false, {} };

    std::string line_before = (pos.line > 0) ? text_[(unsigned int)pos.line - 1] : "";
    auto line = text_[(unsigned int)pos.line];
    auto line_so_far = line.substr(0, (pos.column == 0) ? 1 : (unsigned int)pos.column);
    char last_char = (trigger_kind == 1 && line_so_far != "") ? line_so_far.back() : trigger_char;

    if (last_char == '&')
        return complete_var_(pos);
    else if (last_char == '.')
        return complete_seq_(pos);
    else if ((line_before.size() <= hl_info_.cont_info.continuation_column
                 || std::isspace(line_before[hl_info_.cont_info.continuation_column]))
        && std::regex_match(line_so_far, instruction_regex))
        return { false, ctx_->lsp_ctx->all_instructions };

    return { false, {} };
}

position_uri_s lsp_info_processor::go_to_definition(const position& pos) const
{
    position_uri_s result;
    if (find_definition_(pos, ctx_->lsp_ctx->seq_symbols, result)
        || find_definition_(pos, ctx_->lsp_ctx->var_symbols, result)
        || find_definition_(pos, ctx_->lsp_ctx->ord_symbols, result)
        || find_definition_(pos, ctx_->lsp_ctx->instructions, result))
        return result;
    return { *file_name, pos };
}
std::vector<position_uri_s> lsp_info_processor::references(const position& pos) const
{
    std::vector<position_uri_s> result;
    if (find_references_(pos, ctx_->lsp_ctx->seq_symbols, result)
        || find_references_(pos, ctx_->lsp_ctx->var_symbols, result)
        || find_references_(pos, ctx_->lsp_ctx->ord_symbols, result)
        || find_references_(pos, ctx_->lsp_ctx->instructions, result))
        return result;
    return { { *file_name, pos } };
}
std::vector<std::string> lsp_info_processor::hover(const position& pos) const
{
    std::vector<std::string> result;
    if (get_text_(pos, ctx_->lsp_ctx->seq_symbols, result) || get_text_(pos, ctx_->lsp_ctx->var_symbols, result)
        || get_text_(pos, ctx_->lsp_ctx->ord_symbols, result) || get_text_(pos, ctx_->lsp_ctx->instructions, result))
        return result;
    return result;
}

void lsp_info_processor::finish() { std::sort(hl_info_.lines.begin(), hl_info_.lines.end()); }

const lines_info& lsp_info_processor::semantic_tokens() const { return hl_info_.lines; }

void lsp_info_processor::add_lsp_symbol(lsp_symbol& symbol)
{
    symbol.scope = get_top_macro_stack_();
    switch (symbol.type)
    {
        case symbol_type::ord:
            process_ord_sym_(symbol);
            break;
        case symbol_type::var:
            deferred_vars_.push_back(symbol);
            break;
        case symbol_type::instruction:
            deferred_instruction_.init(symbol.symbol_range.file, symbol.name, symbol.symbol_range.r);
            break;
        case symbol_type::seq:
            process_seq_sym_(symbol);
            break;
    }
}

void lsp_info_processor::add_hl_symbol(token_info symbol)
{
    // file is open in IDE, get its highlighting
    if (collect_hl_info_)
    {
        if (symbol.scope == hl_scopes::continuation)
        {
            hl_info_.cont_info.continuation_positions.push_back(
                { symbol.token_range.start.line, symbol.token_range.start.column });
        }

        // split multi line symbols
        auto rest = symbol;
        while (rest.token_range.start.line != rest.token_range.end.line)
        {
            // remove first line and add as separate token
            auto first = rest;
            first.token_range.end.line = first.token_range.start.line;
            first.token_range.end.column = hl_info_.cont_info.continuation_column;
            hl_info_.lines.push_back(std::move(first));
            rest.token_range.start.line++;
            rest.token_range.start.column = hl_info_.cont_info.continue_column;
        }

        if (rest.token_range.start != rest.token_range.end) // do not add empty tokens
            hl_info_.lines.push_back(std::move(rest));
    }
}

bool lsp_info_processor::is_in_range_(const position& pos, const occurence& occ) const
{
    // check for multi line
    if (occ.symbol_range.start.line != occ.symbol_range.end.line)
    {
        if (file_name != occ.file_name)
            return false;
        if (pos.line < occ.symbol_range.start.line || pos.line > occ.symbol_range.end.line)
            return false;
        // find appropriate line
        for (const auto& cont_pos : hl_info_.cont_info.continuation_positions)
        {
            // might be multi line
            if (cont_pos.line == pos.line)
            {
                // occurences begin line, position cannot be smaller than occ begin column or bigger than continuation
                // column
                if (pos.line == occ.symbol_range.start.line
                    && (pos.column < occ.symbol_range.start.column || pos.column > cont_pos.column))
                    return false;
                // occurences end line, position cannot be bigger than occ end column or smaller than continue column
                if (pos.line == occ.symbol_range.end.line
                    && (pos.column > occ.symbol_range.end.column || pos.column < hl_info_.cont_info.continue_column))
                    return false;
                // in between begin and end lines, only check for continue/continuation columns
                if (pos.column < hl_info_.cont_info.continue_column || pos.column > cont_pos.column)
                    return false;
                return true;
            }
        }
    }
    // no continuation, symbol is single line
    return file_name == occ.file_name && pos.line == occ.symbol_range.start.line
        && pos.line == occ.symbol_range.end.line && pos.column >= occ.symbol_range.start.column
        && pos.column <= occ.symbol_range.end.column;
}

template<typename T>
bool lsp_info_processor::get_text_(
    const position& pos, const definitions<T>& symbols, std::vector<std::string>& found) const
{
    for (const auto& symbol : symbols)
    {
        // for each of its occurences
        for (const auto& occ : symbol.second)
        {
            if (is_in_range_(pos, occ))
            {
                found = symbol.first.get_value();
                return true;
            }
        }
    }
    return false;
}

void lsp_info_processor::process_ord_sym_(const context::ord_definition& symbol)
{
    if (deferred_instruction_.name == ctx_->ids().well_known.COPY)
    {
        ctx_->lsp_ctx->deferred_ord_occs.push_back({ symbol, false });
        ctx_->lsp_ctx->copy = true;
        return;
    }
    // to be processed after parsing
    if (symbol.definition_range.start.column == 0)
        ctx_->lsp_ctx->deferred_ord_defs.push_back(symbol);
    else
        ctx_->lsp_ctx->deferred_ord_occs.push_back({ symbol, false });
}

void lsp_info_processor::process_var_syms_()
{
    for (auto& symbol : deferred_vars_)
    {
        symbol.scope = get_top_macro_stack_();
        auto definition = ctx_->lsp_ctx->var_symbols.find(symbol);

        // definition does not exist, add new one
        if (definition == ctx_->lsp_ctx->var_symbols.end())
        {
            var_type type;
            if (deferred_instruction_.name == ctx_->ids().well_known.SETC
                || deferred_instruction_.name == ctx_->ids().well_known.LCLC
                || deferred_instruction_.name == ctx_->ids().well_known.GBLC)
                type = context::var_type::STRING;
            else if (deferred_instruction_.name == ctx_->ids().well_known.SETA
                || deferred_instruction_.name == ctx_->ids().well_known.LCLA
                || deferred_instruction_.name == ctx_->ids().well_known.GBLA)
                type = context::var_type::NUM;
            else if (deferred_instruction_.name == ctx_->ids().well_known.SETB
                || deferred_instruction_.name == ctx_->ids().well_known.LCLB
                || deferred_instruction_.name == ctx_->ids().well_known.GBLB)
                type = context::var_type::BOOL;
            // macro params
            else if (get_top_macro_stack_().name == deferred_instruction_.name)
                type = context::var_type::MACRO;
            // incorrectly defined first occurence, skip it
            else
                continue;

            macro_id scope = { empty_string, 0 };
            if (deferred_instruction_.name != ctx_->ids().well_known.GBLB
                && deferred_instruction_.name != ctx_->ids().well_known.GBLA
                && deferred_instruction_.name != ctx_->ids().well_known.GBLC)
                scope = get_top_macro_stack_();
            // add it
            ctx_->lsp_ctx
                ->var_symbols[var_definition(symbol.name, symbol.file_name, symbol.definition_range, type, scope)]
                .push_back({ symbol.definition_range, symbol.file_name });
        }
        else
        {
            // add occurence to definition
            definition->second.push_back({ symbol.definition_range, symbol.file_name });
        }
    }
    // clean
    deferred_vars_.clear();
    deferred_instruction_.clear(empty_string);
}

void lsp_info_processor::process_seq_sym_(const context::seq_definition& symbol)
{
    auto found = ctx_->lsp_ctx->seq_symbols.find(symbol);
    // there is definition, add occurence
    if (found != ctx_->lsp_ctx->seq_symbols.end())
    {
        found->second.push_back({ symbol.definition_range, symbol.file_name });
    }
    else
    {
        // definition, create it
        if (symbol.definition_range.start.column == 0)
        {
            // add
            auto occurences = &ctx_->lsp_ctx->seq_symbols[context::seq_definition(
                symbol.name, symbol.file_name, symbol.definition_range, get_top_macro_stack_())];
            occurences->push_back({ symbol.definition_range, symbol.file_name });

            // add deferred if its matching current definition
            decltype(ctx_->lsp_ctx->deferred_seqs) temp_seqs;
            for (auto& deferred_sym : ctx_->lsp_ctx->deferred_seqs)
            {
                // there is definition, add occurence to it and remove it from deferred
                if (deferred_sym.name == symbol.name)
                    occurences->push_back({ deferred_sym.definition_range, deferred_sym.file_name });
                else
                    temp_seqs.push_back(std::move(deferred_sym));
            }
            ctx_->lsp_ctx->deferred_seqs = std::move(temp_seqs);
        }
        // not a definition, defer
        else
            ctx_->lsp_ctx->deferred_seqs.push_back(symbol);
    }
}
void lsp_info_processor::process_instruction_sym_()
{
    // COPY started
    if (ctx_->lsp_ctx->copy && deferred_instruction_.name != ctx_->ids().well_known.COPY)
    {
        // check for last deferred ordinary symbol - defines copy file
        auto deferred_ord = ctx_->lsp_ctx->deferred_ord_occs.back();
        // add current instruction as its definition
        ctx_->lsp_ctx
            ->ord_symbols[context::ord_definition(
                deferred_instruction_.name, deferred_instruction_.file_name, deferred_instruction_.definition_range)]
            .push_back({ deferred_ord.first.definition_range, deferred_ord.first.file_name });
        // remove it
        ctx_->lsp_ctx->deferred_ord_occs.pop_back();
        ctx_->lsp_ctx->copy = false;
    }

    if (deferred_instruction_.name == ctx_->ids().well_known.MACRO)
    {
        ctx_->lsp_ctx->parser_macro_stack.push({ empty_string, 0 });
        return;
    }
    else if (!ctx_->lsp_ctx->parser_macro_stack.empty() && deferred_instruction_.name == ctx_->ids().well_known.MEND)
    {
        ctx_->lsp_ctx->parser_macro_stack.pop();
        return;
    }
    // define macro
    else if (!ctx_->lsp_ctx->parser_macro_stack.empty() && get_top_macro_stack_().name == empty_string)
    {
        if (deferred_instruction_.name == empty_string)
        {
            ctx_->lsp_ctx->parser_macro_stack.top() = { ctx_->ids().well_known.ASPACE, 0 };
            return;
        }

        // find if the macro already exists
        // create new version of it
        auto latest = find_latest_version_(deferred_instruction_, ctx_->lsp_ctx->instructions);
        size_t current_version = latest + 1;

        // parameters text
        std::stringstream params_text;
        ctx_->lsp_ctx->parser_macro_stack.top() = { deferred_instruction_.name, current_version };
        size_t index = 0;
        // before parameter
        if (!deferred_vars_.empty() && deferred_vars_[0].definition_range.start.column == 0)
        {
            params_text << *deferred_vars_[0].name << " ";
            index++;
        }
        // name
        bool first = true;
        // after parameters
        for (size_t i = index; i < deferred_vars_.size(); ++i)
        {
            if (!first)
                params_text << ",";
            else
                first = false;
            params_text << *deferred_vars_[i].name;
        }

        // add it to list of completion items
        ctx_->lsp_ctx->all_instructions.push_back({ *deferred_instruction_.name,
            params_text.str(),
            *deferred_instruction_.name + "   " + params_text.str(),
            content_pos((unsigned int)deferred_instruction_.definition_range.start.line, &text_) });

        // add it to definitions
        auto occurences = &ctx_->lsp_ctx->instructions[context::instr_definition(deferred_instruction_.name,
            deferred_instruction_.file_name,
            deferred_instruction_.definition_range,
            ctx_->lsp_ctx->all_instructions.back(),
            current_version)];
        occurences->push_back({ deferred_instruction_.definition_range, deferred_instruction_.file_name });
        if (ctx_->lsp_ctx->deferred_macro_statement.name == deferred_instruction_.name)
        {
            occurences->push_back({ ctx_->lsp_ctx->deferred_macro_statement.definition_range,
                ctx_->lsp_ctx->deferred_macro_statement.file_name });

            ctx_->lsp_ctx->deferred_macro_statement.clear(empty_string);
        }
    }
    // not a macro, check for predefined instructions
    else
    {
        // check if it is defined
        deferred_instruction_.version = find_latest_version_(deferred_instruction_, ctx_->lsp_ctx->instructions);
        auto definition = ctx_->lsp_ctx->instructions.find(deferred_instruction_);
        // it exists, add it as occurence
        if (definition != ctx_->lsp_ctx->instructions.end())
        {
            definition->second.push_back({ deferred_instruction_.definition_range, deferred_instruction_.file_name });
        }
        // define new instruction
        else
        {
            auto instr = std::find_if(ctx_->lsp_ctx->all_instructions.begin(),
                ctx_->lsp_ctx->all_instructions.end(),
                [&](const context::completion_item_s& instr) {
                    return deferred_instruction_.name && instr.label == *deferred_instruction_.name;
                });
            if (instr != ctx_->lsp_ctx->all_instructions.end())
            {
                ctx_->lsp_ctx
                    ->instructions[context::instr_definition(deferred_instruction_.name,
                        deferred_instruction_.file_name,
                        deferred_instruction_.definition_range,
                        *instr,
                        (size_t)-1)]
                    .push_back({ deferred_instruction_.definition_range, deferred_instruction_.file_name });
            }
            // undefined instruction, special case when macro name statement goes before macro parsing
            // or an error
            else
                ctx_->lsp_ctx->deferred_macro_statement.init(deferred_instruction_.file_name,
                    deferred_instruction_.name,
                    deferred_instruction_.definition_range);
        }
    }
}

completion_list_s lsp_info_processor::complete_var_(const position& pos) const
{
    std::vector<context::completion_item_s> items;
    for (const auto& symbol : ctx_->lsp_ctx->var_symbols)
    {
        if (symbol.first.definition_range.start.line < pos.line && symbol.first.file_name == file_name)
        {
            auto value = symbol.first.get_value();
            assert(value.size() == 1);
            items.push_back({ "&" + *symbol.first.name, value[0], "&" + *symbol.first.name, { "" }, (size_t)5 });
        }
    }
    return { false, items };
}

completion_list_s lsp_info_processor::complete_seq_(const position& pos) const
{
    std::vector<context::completion_item_s> items;
    for (auto& symbol : ctx_->lsp_ctx->seq_symbols)
    {
        if (symbol.first.definition_range.start.line < pos.line && symbol.first.file_name == file_name)
        {
            auto value = symbol.first.get_value();
            assert(value.size() == 1);
            items.push_back({ "." + *symbol.first.name, value[0], "." + *symbol.first.name, { "" }, (size_t)5 });
        }
    }
    return { false, items };
}

int lsp_info_processor::find_latest_version_(
    const context::instr_definition& current, const context::definitions<context::instr_definition>& to_check) const
{
    auto definition = to_check.find(current);
    if (definition == to_check.end())
        return -1;

    auto curr_copy = current;
    auto version = curr_copy.version;
    while (true)
    {
        if (to_check.find(curr_copy) == to_check.end())
            return (int)version;
        version = curr_copy.version;
        curr_copy.version++;
    }
}

context::macro_id lsp_info_processor::get_top_macro_stack_() const
{
    if (!ctx_->lsp_ctx->parser_macro_stack.empty())
        return ctx_->lsp_ctx->parser_macro_stack.top();
    return { empty_string, 0 };
}
