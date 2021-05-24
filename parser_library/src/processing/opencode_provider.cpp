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

#include "opencode_provider.h"

#include "hlasmparser.h"
#include "lexing/token_stream.h"
#include "parsing/error_strategy.h"
#include "parsing/parser_error_listener_ctx.h"
#include "parsing/parser_impl.h"
#include "semantics/collector.h"
#include "semantics/range_provider.h"

namespace hlasm_plugin::parser_library::processing {

opencode_provider::opencode_provider(std::string_view text,
    analyzing_context& ctx,
    workspaces::parse_lib_provider& lib_provider,
    processing::processing_state_listener& state_listener,
    semantics::source_info_processor& src_proc,
    parsing::parser_error_listener& err_listener)
    : statement_provider(processing::statement_provider_kind::OPEN)
    , m_original_text(text)
    , m_current_text(text)
    , m_parser(parsing::parser_holder::create(&src_proc))
    , m_second_parser(parsing::parser_holder::create(nullptr))
    , m_ctx(&ctx)
    , m_lib_provider(&lib_provider)
    , m_state_listener(&state_listener)
    , m_src_proc(&src_proc)
{
    m_parser->parser->initialize(*m_ctx, m_src_proc);
    m_parser->parser->setErrorHandler(std::make_shared<parsing::error_strategy>());
    m_parser->parser->removeErrorListeners();
    m_parser->parser->addErrorListener(&err_listener);
}

void opencode_provider::rewind_input(context::source_position pos)
{
    apply_pending_line_changes();

    m_ainsert_buffer.clear(); // this needs to be tested, but apparently AGO clears AINSERT buffer
    m_current_text = m_original_text.substr(pos.file_offset);
    m_current_line = pos.file_line;
}

std::string opencode_provider::aread()
{
    apply_pending_line_changes();

    std::string result;
    if (!m_ainsert_buffer.empty())
    {
        result = std::move(m_ainsert_buffer.front());
        m_ainsert_buffer.pop_front();
    }
    else if (!m_copy_files.empty())
    {
        result = lexing::extract_line(m_copy_files.back()).first;
        if (m_copy_files.back().empty())
            m_copy_files.pop_back();
    }
    else if (!m_preprocessor_buffer.empty())
    {
        result = std::move(m_preprocessor_buffer.back());
        m_preprocessor_buffer.pop_back();
    }
    else
    {
        result = lexing::extract_line(m_current_text).first;
    }

    if (!result.empty())
        result.resize(80, ' ');
    return result;
}

void opencode_provider::ainsert(const std::string& rec, ainsert_destination dest)
{
    switch (dest)
    {
        case hlasm_plugin::parser_library::processing::ainsert_destination::back:
            m_ainsert_buffer.push_back(rec);
            break;
        case hlasm_plugin::parser_library::processing::ainsert_destination::front:
            m_ainsert_buffer.push_front(rec);
            break;
    }
}

bool opencode_provider::feed_line()
{
    if (!extract_next_logical_line())
        return false;
    line_fed = true;

    m_parser->input->reset("");
    bool first_line = true;
    m_current_logical_line.segments.back().eol = lexing::logical_line_segment_eol::none; // do not add the last EOL
    for (auto& s : m_current_logical_line.segments)
    {
        if (!first_line)
        {
            if (s.continuation_error)
                m_parser->input->append(std::string_view("XXXXXXXXXXXXXXX"));
            else
                m_parser->input->append(std::string_view("               "));
        }
        else
            first_line = false;
        m_parser->input->append(s.code);
        m_parser->input->append(s.continuation);
        m_parser->input->append(s.ignore);

        switch (s.eol)
        {
            case hlasm_plugin::parser_library::lexing::logical_line_segment_eol::none:
                break;
            case hlasm_plugin::parser_library::lexing::logical_line_segment_eol::lf:
                m_parser->input->append("\n");
                break;
            case hlasm_plugin::parser_library::lexing::logical_line_segment_eol::crlf:
                m_parser->input->append("\r\n");
                break;
            case hlasm_plugin::parser_library::lexing::logical_line_segment_eol::cr:
                m_parser->input->append("\r");
                break;
        }
    }
    m_parser->input->reset();

    m_parser->lex->set_file_offset({ m_current_line, 0 }); // lexing::default_ictl.begin-1 really
    m_parser->lex->set_unlimited_line(false);
    m_parser->lex->reset();

    m_parser->stream->reset();

    m_parser->parser->reset();

    m_parser->parser->get_collector().prepare_for_next_statement();

    m_ctx->hlasm_ctx->metrics.lines += m_current_logical_line.segments.size();

    return true;
}
bool opencode_provider::process_comment()
{
    auto prefix = m_current_logical_line.segments.front().code.substr(0, 2);
    if (prefix != ".*" && prefix.substr(0, 1) != "*")
        return false;

    size_t i = 0;
    for (const auto& l : m_current_logical_line.segments)
    {
        if (l.code.size())
        {
            auto [skip_len, _] = lexing::length_utf16(l.line.substr(0, l.code.data() - l.line.data()));
            auto [code_len, last_big] = lexing::length_utf16(l.code);

            m_src_proc->add_hl_symbol(
                token_info(range(position(m_current_line + i, skip_len),
                               position(m_current_line + i, skip_len + code_len - !!code_len - last_big)),
                    semantics::hl_scopes::comment));
        }
        ++i;
    }
    return true;
}

context::shared_stmt_ptr opencode_provider::get_next(const statement_processor& proc)
{
    if (!feed_line())
        return nullptr;

    if (process_comment())
        return nullptr;

    auto& collector = m_parser->parser->get_collector();

    collector.prepare_for_next_statement();

    parsing::shared_stmt_ptr result;
    if (proc.kind == processing::processing_kind::LOOKAHEAD)
    {
        auto look_lab_instr = m_parser->parser->look_lab_instr();

        m_ctx->hlasm_ctx->set_source_indices(
            m_current_logical_line.segments.front().code.data() - m_original_text.data(),
            m_current_text.size() ? m_current_text.data() - m_original_text.data() : m_original_text.size(),
            m_current_line + m_current_logical_line.segments.size() - 1);
        m_ctx->hlasm_ctx->set_source_position(collector.current_instruction().field_range.start);
        auto proc_status = proc.get_processing_status(collector.peek_instruction());

        if (look_lab_instr->op_text
            && proc_status.first.form != processing::processing_form::IGNORED
            // optimization : if statement has no label and is not COPY, do not even parse operands
            && (collector.has_label() || collector.current_instruction().type != semantics::instruction_si_type::ORD
                || std::get<context::id_index>(collector.current_instruction().value)
                    == m_ctx->hlasm_ctx->ids().well_known.COPY))
        {
            const auto& text = *look_lab_instr->op_text;
            const auto& text_range = look_lab_instr->op_range;

            parsing::parser_error_listener_ctx listener(*m_ctx->hlasm_ctx, std::nullopt);
            semantics::range_provider range_prov;
            const auto& h =
                prepare_second_parser(text, *m_ctx->hlasm_ctx, listener, range_prov, text_range, proc_status, true);

            h.parser->lookahead_operands_and_remarks();

            h.parser->get_collector().clear_hl_symbols();
            collector.append_operand_field(std::move(h.parser->get_collector()));
            // lookahead ignores messages collect_diags_from_child(listener);
        }
        range statement_range(position(m_current_line, 0)); // assign default
        result = collector.extract_statement(proc_status, statement_range);

        if (m_current_logical_line.segments.size() > 1)
            m_ctx->hlasm_ctx->metrics.continued_statements++;
        else
            m_ctx->hlasm_ctx->metrics.non_continued_statements++;

        m_src_proc->process_hl_symbols(collector.extract_hl_symbols());
    }
    else
    {
        auto lab_instr = m_parser->parser->lab_instr();

        m_ctx->hlasm_ctx->set_source_indices(
            m_current_logical_line.segments.front().code.data() - m_original_text.data(),
            m_current_text.size() ? m_current_text.data() - m_original_text.data() : m_original_text.size(),
            m_current_line + m_current_logical_line.segments.size() - 1);

        if (collector.has_instruction())
        {
            if (proc.kind == processing::processing_kind::ORDINARY
                && try_trigger_attribute_lookahead(
                    collector.current_instruction(), { *m_ctx, *m_lib_provider }, *m_state_listener))
                return result;

            m_ctx->hlasm_ctx->set_source_position(collector.current_instruction().field_range.start);
            auto proc_status = proc.get_processing_status(collector.peek_instruction());

            if (lab_instr->op_text)
            {
                // parse_operands(std::move(*lab_instr->op_text), lab_instr->op_range);

                parsing::parser_error_listener_ctx listener(*m_ctx->hlasm_ctx, std::nullopt);
                semantics::range_provider range_prov;
                const auto& h = prepare_second_parser(*lab_instr->op_text,
                    *m_ctx->hlasm_ctx,
                    listener,
                    range_prov,
                    lab_instr->op_range,
                    proc_status,
                    false);

                auto& [format, opcode] = proc_status;
                if (format.occurence == processing::operand_occurence::ABSENT
                    || format.form == processing::processing_form::UNKNOWN)
                    h.parser->op_rem_body_noop();
                else
                {
                    switch (format.form)
                    {
                        case processing::processing_form::IGNORED:
                            h.parser->op_rem_body_ignored();
                            break;
                        case processing::processing_form::DEFERRED:
                            h.parser->op_rem_body_deferred();
                            break;
                        case processing::processing_form::CA:
                            h.parser->op_rem_body_ca();
                            break;
                        case processing::processing_form::MAC: {
                            auto rule = h.parser->op_rem_body_mac();
                            auto line = std::move(rule->line);
                            auto line_range = rule->line_range;

                            if (line.operands.size())
                            {
                                size_t string_size = line.operands.size();
                                std::vector<range> ranges;

                                for (auto& op : line.operands)
                                    if (auto m_op = dynamic_cast<semantics::macro_operand_string*>(op.get()))
                                        string_size += m_op->value.size();

                                std::string to_parse;
                                to_parse.reserve(string_size);

                                for (size_t i = 0; i < line.operands.size(); ++i)
                                {
                                    if (auto m_op =
                                            dynamic_cast<semantics::macro_operand_string*>(line.operands[i].get()))
                                        to_parse.append(m_op->value);
                                    if (i != line.operands.size() - 1)
                                        to_parse.push_back(',');
                                    ranges.push_back(line.operands[i]->operand_range);
                                }
                                auto r = semantics::range_provider::union_range(
                                    line.operands.front()->operand_range, line.operands.back()->operand_range);

                                // line.operands = parse_macro_operands(std::move(to_parse), r, std::move(ranges));
                                // semantics::operand_list parser_impl::parse_macro_operands(std::string operands, range
                                // field_range, std::vector<range> operand_ranges)

                                semantics::range_provider tmp_provider(
                                    r, ranges, semantics::adjusting_state::MACRO_REPARSE);
                                parsing::parser_error_listener_ctx listener(
                                    *m_ctx->hlasm_ctx, std::nullopt, tmp_provider);


                                const auto& h = prepare_second_parser(
                                    to_parse, *m_ctx->hlasm_ctx, listener, tmp_provider, r, proc_status, true);

                                line.operands = std::move(h.parser->macro_ops()->list);

                                collect_diags_from_child(listener);
                            }

                            h.parser->get_collector().set_operand_remark_field(
                                std::move(line.operands), std::move(line.remarks), line_range);
                        }
                        break;
                        case processing::processing_form::ASM:
                            h.parser->op_rem_body_asm();
                            break;
                        case processing::processing_form::MACH:
                            h.parser->op_rem_body_mach();
                            break;
                        case processing::processing_form::DAT:
                            h.parser->op_rem_body_dat();
                            break;
                        default:
                            break;
                    }
                }

                if (format.form != processing::processing_form::IGNORED)
                {
                    collector.append_operand_field(std::move(h.parser->get_collector()));
                }
                collect_diags_from_child(listener);
            }
            range statement_range(position(m_current_line, 0)); // assign default
            result = collector.extract_statement(proc_status, statement_range);

            if (proc.kind == processing::processing_kind::ORDINARY
                && try_trigger_attribute_lookahead(*result, { *m_ctx, *m_lib_provider }, *m_state_listener))
                return nullptr;

            if (m_current_logical_line.segments.size() > 1)
                m_ctx->hlasm_ctx->metrics.continued_statements++;
            else
                m_ctx->hlasm_ctx->metrics.non_continued_statements++;

            m_src_proc->process_hl_symbols(collector.extract_hl_symbols());
        }
    }

    return result;
}

bool opencode_provider::finished() const
{
    return m_current_text.empty() && m_copy_files.empty() && m_ainsert_buffer.empty() && m_preprocessor_buffer.empty();
}

parsing::hlasmparser& opencode_provider::parser()
{
    if (!line_fed)
        throw 0;
    return *m_parser->parser;
}


void opencode_provider::collect_diags() const
{
    collect_diags_from_child(*m_parser->parser);
    collect_diags_from_child(*m_second_parser->parser);
}

void opencode_provider::apply_pending_line_changes()
{
    m_ainsert_buffer.erase(m_ainsert_buffer.begin(), m_ainsert_buffer.begin() + m_lines_to_remove.ainsert_buffer);
    m_current_line += m_lines_to_remove.current_text_lines;

    m_lines_to_remove = {};
}

bool opencode_provider::extract_next_logical_line()
{
    apply_pending_line_changes();

    m_current_logical_line.clear();

    if (m_ainsert_buffer.size())
    {
        for (std::string_view line : m_ainsert_buffer)
        {
            ++m_lines_to_remove.ainsert_buffer;
            if (!lexing::append_to_logical_line(m_current_logical_line, line, lexing::default_ictl_copy))
                break;
        }
        finish_logical_line(m_current_logical_line, lexing::default_ictl_copy);
        return true;
    }

    // TODO: other sources

    while (m_current_text.size())
    {
        ++m_lines_to_remove.current_text_lines;
        if (!append_to_logical_line(m_current_logical_line, m_current_text, lexing::default_ictl))
            break;
    }
    finish_logical_line(m_current_logical_line, lexing::default_ictl);

    return !m_current_logical_line.segments.empty();
}

const parsing::parser_holder& opencode_provider::prepare_second_parser(const std::string& text,
    context::hlasm_context& hlasm_ctx,
    parsing::parser_error_listener_ctx& err_listener,
    semantics::range_provider& range_prov,
    range text_range,
    const processing_status& proc_status,
    bool unlimited_line)
{
    auto& h = *m_second_parser;

    h.input->reset(text);

    h.lex->reset();
    h.lex->set_file_offset(text_range.start);
    h.lex->set_unlimited_line(unlimited_line);

    h.stream->reset();

    h.parser->reinitialize(&hlasm_ctx, range_prov, proc_status);
    h.parser->setErrorHandler(std::make_shared<parsing::error_strategy>());
    h.parser->removeErrorListeners();
    h.parser->addErrorListener(&err_listener);

    h.parser->reset();

    h.parser->get_collector().prepare_for_next_statement();

    return h;
}

} // namespace hlasm_plugin::parser_library::processing
