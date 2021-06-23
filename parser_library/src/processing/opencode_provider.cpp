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
    const std::string& filename,
    opencode_provider_options opts)
    : statement_provider(processing::statement_provider_kind::OPEN)
    , m_original_text(text)
    , m_next_line_text(text)
    , m_parser(parsing::parser_holder::create(&src_proc))
    , m_lookahead_parser(parsing::parser_holder::create(nullptr))
    , m_operand_parser(parsing::parser_holder::create(nullptr))
    , m_ctx(&ctx)
    , m_lib_provider(&lib_provider)
    , m_state_listener(&state_listener)
    , m_src_proc(&src_proc)
    , m_opts(opts)
    , m_listener(filename)
{
    m_parser->parser->initialize(m_ctx->hlasm_ctx.get(), &m_listener);
    m_parser->parser->removeErrorListeners();
    m_parser->parser->addErrorListener(&m_listener);

    m_lookahead_parser->parser->initialize(m_ctx->hlasm_ctx.get(), nullptr);
    m_lookahead_parser->parser->removeErrorListeners();
}

void opencode_provider::rewind_input(context::source_position pos)
{
    apply_pending_line_changes();

    m_ainsert_buffer.clear(); // this needs to be tested, but apparently AGO clears AINSERT buffer
    m_next_line_text = m_original_text.substr(pos.file_offset);
    m_current_line = pos.file_line;
}

void opencode_provider::generate_aread_highlighting(std::string_view text, size_t line_no) const
{
    if (text.empty())
        return;

    auto [rest, utf16_skipped] = lexing::skip_chars(text, 80);
    if (utf16_skipped)
        m_src_proc->add_hl_symbol(
            token_info(range(position(line_no, 0), position(line_no, utf16_skipped)), semantics::hl_scopes::string));

    if (rest.empty())
        return;

    auto [rest_len, last_big] = lexing::length_utf16(rest);
    if (rest_len)
        m_src_proc->add_hl_symbol(
            token_info(range(position(line_no, utf16_skipped), position(line_no, utf16_skipped + rest_len)),
                semantics::hl_scopes::ignored));
}

std::string opencode_provider::aread()
{
    apply_pending_line_changes();

    bool adjust_length = true;
    std::string result;
    if (!m_ainsert_buffer.empty())
    {
        result = std::move(m_ainsert_buffer.front());
        m_ainsert_buffer.pop_front();
        // resume normal copy processing if the last line from ainsert buffer was removed
        // AND no modifications were done to sources on the copy stack
        if (m_ainsert_buffer.empty() && m_copy_suspended && !m_copy_files_aread_ready)
            resume_copy(0, resume_copy::ignore_line);
    }
    else if (!m_copy_files.empty() || !m_copy_files_aread_ready && fill_copy_buffer_for_aread())
    {
        auto& current = m_copy_files.back();
        result = lexing::extract_line(current.text).first;
        ++current.line_no;
        while (!m_copy_files.empty() && m_copy_files.back().text.empty())
        {
            m_copy_files.pop_back();
            m_ctx->hlasm_ctx->opencode_copy_stack().pop_back();
        }
    }
    else if (!m_preprocessor_buffer.empty())
    {
        result = std::move(m_preprocessor_buffer.back());
        m_preprocessor_buffer.pop_back();
    }
    else if (!m_next_line_text.empty())
    {
        result = lexing::extract_line(m_next_line_text).first;

        generate_aread_highlighting(result, m_current_line);

        m_lines_to_remove.current_text_lines++;
    }
    else
        adjust_length = false;

    if (adjust_length)
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
    // ainsert buffer may contain macro call that will remove code lines from copybooks
    // this prevents copybook unwinding
    if (!m_copy_suspended)
        suspend_copy();
}

extract_next_logical_line_result opencode_provider::feed_line(parsing::parser_holder& p)
{
    auto ll_res = extract_next_logical_line();
    if (ll_res == extract_next_logical_line_result::failed)
        return ll_res;
    m_line_fed = true;

    p.input->reset(m_current_logical_line);

    p.lex->set_file_offset({ m_current_logical_line_source.begin_line, 0 /*lexing::default_ictl.begin-1 really*/ },
        ll_res == extract_next_logical_line_result::process);
    p.lex->set_unlimited_line(false);
    p.lex->reset();

    p.stream->reset();

    p.parser->reset();

    p.parser->get_collector().prepare_for_next_statement();

    m_ctx->hlasm_ctx->metrics.lines += m_current_logical_line.segments.size();

    return ll_res;
}

bool opencode_provider::is_comment()
{
    auto prefix = m_current_logical_line.segments.front().code.substr(0, 2);
    return prefix == ".*" || prefix.substr(0, 1) == "*";
}

void opencode_provider::process_comment()
{
    size_t line_no = m_current_logical_line_source.begin_line;
    for (const auto& l : m_current_logical_line.segments)
    {
        if (l.code.size())
        {
            auto [skip_len, _] = lexing::length_utf16(l.line.substr(0, l.code.data() - l.line.data()));
            auto [code_len, last_big] = lexing::length_utf16(l.code);

            m_src_proc->add_hl_symbol(
                token_info(range(position(line_no, skip_len), position(line_no, skip_len + code_len)),
                    semantics::hl_scopes::comment));
        }
        ++line_no;
    }
}

void opencode_provider::generate_continuation_error_messages() const
{
    auto line_no = m_current_logical_line_source.begin_line;
    for (const auto& s : m_current_logical_line.segments)
    {
        if (s.continuation_error)
        {
            parsing::parser_error_listener_ctx listener(*m_ctx->hlasm_ctx, nullptr);
            listener.add_diagnostic(
                diagnostic_op::error_E001(range { { line_no, 0 }, { line_no, s.code_offset_utf16 } }));
            collect_diags_from_child(listener);
            break;
        }
        ++line_no;
    }
}
std::shared_ptr<const context::hlasm_statement> opencode_provider::process_lookahead(const statement_processor& proc,
    semantics::collector& collector,
    const std::optional<std::string>& op_text,
    const range& op_range)
{
    m_ctx->hlasm_ctx->set_source_position(collector.current_instruction().field_range.start);
    auto proc_status = proc.get_processing_status(collector.peek_instruction());

    if (op_text
        && proc_status.first.form != processing::processing_form::IGNORED
        // optimization : if statement has no label and is not COPY, do not even parse operands
        && (collector.has_label() || collector.current_instruction().type != semantics::instruction_si_type::ORD
            || std::get<context::id_index>(collector.current_instruction().value)
                == m_ctx->hlasm_ctx->ids().well_known.COPY))
    {
        const auto& h = prepare_operand_parser(*op_text, *m_ctx->hlasm_ctx, nullptr, {}, op_range, proc_status, true);

        h.parser->lookahead_operands_and_remarks();

        h.parser->get_collector().clear_hl_symbols();
        collector.append_operand_field(std::move(h.parser->get_collector()));
    }
    range statement_range(position(m_current_logical_line_source.begin_line, 0)); // assign default
    auto result = collector.extract_statement(proc_status, statement_range);

    if (m_current_logical_line.segments.size() > 1)
        m_ctx->hlasm_ctx->metrics.continued_statements++;
    else
        m_ctx->hlasm_ctx->metrics.non_continued_statements++;

    return result;
}
std::shared_ptr<const context::hlasm_statement> opencode_provider::process_ordinary(const statement_processor& proc,
    semantics::collector& collector,
    const std::optional<std::string>& op_text,
    const range& op_range)
{
    if (proc.kind == processing::processing_kind::ORDINARY
        && try_trigger_attribute_lookahead(
            collector.current_instruction(), { *m_ctx, *m_lib_provider }, *m_state_listener))
        return nullptr;

    m_ctx->hlasm_ctx->set_source_position(collector.current_instruction().field_range.start);
    auto proc_status = proc.get_processing_status(collector.peek_instruction());

    if (op_text)
    {
        parsing::parser_error_listener_ctx listener(*m_ctx->hlasm_ctx, nullptr);
        const auto& h =
            prepare_operand_parser(*op_text, *m_ctx->hlasm_ctx, &listener, {}, op_range, proc_status, false);

        const auto& [format, opcode] = proc_status;
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
                        auto [to_parse, ranges, r] = join_operands(line.operands);

                        semantics::range_provider tmp_provider(r, ranges, semantics::adjusting_state::MACRO_REPARSE);
                        parsing::parser_error_listener_ctx tmp_listener(*m_ctx->hlasm_ctx, nullptr, tmp_provider);

                        const auto& h_second = prepare_operand_parser(
                            to_parse, *m_ctx->hlasm_ctx, &tmp_listener, std::move(tmp_provider), r, proc_status, true);

                        line.operands = std::move(h_second.parser->macro_ops()->list);

                        collect_diags_from_child(tmp_listener);
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
                    if (auto& h_collector = h.parser->get_collector(); h_collector.has_operands())
                        transform_reloc_imm_operands(h_collector.current_operands().value, opcode.value);
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
    range statement_range(position(m_current_logical_line_source.begin_line, 0)); // assign default
    auto result = collector.extract_statement(proc_status, statement_range);

    if (proc.kind == processing::processing_kind::ORDINARY
        && try_trigger_attribute_lookahead(*result, { *m_ctx, *m_lib_provider }, *m_state_listener))
        return nullptr;

    if (m_current_logical_line.segments.size() > 1)
        m_ctx->hlasm_ctx->metrics.continued_statements++;
    else
        m_ctx->hlasm_ctx->metrics.non_continued_statements++;

    m_src_proc->process_hl_symbols(collector.extract_hl_symbols());

    return result;
}

bool opencode_provider::fill_copy_buffer_for_aread()
{
    auto& opencode_stack = m_ctx->hlasm_ctx->opencode_copy_stack();
    if (opencode_stack.empty())
        return false;

    const auto cmi_to_cms = [lsp_ctx = m_ctx->lsp_ctx.get()](const context::copy_member_invocation& copy) {
        const auto pos = copy.cached_definition->at(copy.current_statement).get_base()->statement_position();
        const auto* const copy_content = lsp_ctx->get_file_info(copy.definition_location->file);

        std::string_view full_text = copy_content->data.get_lines_beginning_at({ 0, 0 });
        std::string_view remaining_text = copy_content->data.get_lines_beginning_at(pos);

        // remove line being processed
        lexing::logical_line ll = {};
        lexing::extract_logical_line(ll, remaining_text, lexing::default_ictl_copy);

        return copy_member_state { remaining_text, full_text, pos.line + ll.segments.size() };
    };
    std::transform(opencode_stack.begin(), opencode_stack.end(), std::back_inserter(m_copy_files), cmi_to_cms);

    while (!m_copy_files.empty() && m_copy_files.back().text.empty())
    {
        m_copy_files.pop_back();
        opencode_stack.pop_back();
    }

    m_copy_files_aread_ready = true;

    if (m_copy_files.empty())
        return false;

    suspend_copy();

    return true;
}

context::shared_stmt_ptr opencode_provider::get_next(const statement_processor& proc)
{
    const bool lookahead = proc.kind == processing::processing_kind::LOOKAHEAD;

    auto& ph = lookahead ? *m_lookahead_parser : *m_parser;

    auto feed_line_res = feed_line(ph);
    if (feed_line_res == extract_next_logical_line_result::failed)
        return nullptr;

    // lookahead may read something that will be removed from the input stream later on
    if (m_current_logical_line.continuation_error && !lookahead)
        generate_continuation_error_messages();

    if (feed_line_res != extract_next_logical_line_result::process && is_comment())
    {
        if (!lookahead)
            process_comment();
        return nullptr;
    }

    auto& collector = ph.parser->get_collector();

    collector.prepare_for_next_statement();

    const auto& [op_text, op_range] = [parser = ph.parser.get(), lookahead]() {
        if (lookahead)
        {
            auto look_lab_instr = parser->look_lab_instr();
            return std::tie(look_lab_instr->op_text, look_lab_instr->op_range);
        }
        else
        {
            auto lab_instr = parser->lab_instr();
            return std::tie(lab_instr->op_text, lab_instr->op_range);
        }
    }();

    if (!collector.has_instruction())
        return nullptr;

    m_ctx->hlasm_ctx->set_source_indices(m_current_logical_line_source.begin_offset,
        m_current_logical_line_source.end_offset,
        m_current_logical_line_source.end_line);

    return lookahead ? process_lookahead(proc, collector, op_text, op_range)
                     : process_ordinary(proc, collector, op_text, op_range);
}

bool opencode_provider::finished() const
{
    return m_next_line_text.empty() && m_copy_files.empty() && m_ainsert_buffer.empty()
        && m_preprocessor_buffer.empty();
}

parsing::hlasmparser& opencode_provider::parser()
{
    if (!m_line_fed)
        feed_line(*m_parser);
    assert(m_line_fed);
    return *m_parser->parser;
}


void opencode_provider::collect_diags() const { collect_diags_from_child(m_listener); }

void opencode_provider::apply_pending_line_changes()
{
    m_ainsert_buffer.erase(m_ainsert_buffer.begin(), m_ainsert_buffer.begin() + m_lines_to_remove.ainsert_buffer);
    m_current_line += m_lines_to_remove.current_text_lines;

    m_lines_to_remove = {};
}

bool opencode_provider::is_next_line_ictl() const
{
    static constexpr std::string_view ICTL_LITERAL = "ICTL";

    const auto non_blank = m_next_line_text.find_first_not_of(' ');
    if (non_blank == std::string_view::npos || non_blank == 0)
        return false;
    const auto test_ictl = m_next_line_text.substr(non_blank);

    if (test_ictl.size() > ICTL_LITERAL.size() && test_ictl[ICTL_LITERAL.size()] != ' ')
        return false;

    auto possible_ictl = test_ictl.substr(0, ICTL_LITERAL.size());
    return std::equal(
        possible_ictl.cbegin(), possible_ictl.cend(), ICTL_LITERAL.cbegin(), [](unsigned char l, unsigned char r) {
            return std::toupper(l) == r;
        });
}

bool opencode_provider::is_next_line_process() const
{
    static constexpr std::string_view PROCESS_LITERAL = "*PROCESS";

    if (m_next_line_text.size() > PROCESS_LITERAL.size() && m_next_line_text[PROCESS_LITERAL.size()] != ' ')
        return false;

    const auto test_process = m_next_line_text.substr(0, PROCESS_LITERAL.size());
    return std::equal(
        test_process.cbegin(), test_process.cend(), PROCESS_LITERAL.cbegin(), [](unsigned char l, unsigned char r) {
            return std::toupper(l) == r;
        });
}


extract_next_logical_line_result opencode_provider::extract_next_logical_line_from_ainsert_buffer()
{
    for (std::string_view line : m_ainsert_buffer)
    {
        ++m_lines_to_remove.ainsert_buffer;
        if (!lexing::append_to_logical_line(m_current_logical_line, line, lexing::default_ictl_copy))
            break;
    }
    finish_logical_line(m_current_logical_line, lexing::default_ictl_copy);

    m_current_logical_line_source.begin_line = 0;
    m_current_logical_line_source.end_line = m_current_logical_line.segments.size() - 1;
    m_current_logical_line_source.begin_offset = 0;
    m_current_logical_line_source.end_offset = 0;
    m_current_logical_line_source.source = logical_line_origin::source_type::ainsert;

    // resume normal copy processing if the last line from ainsert buffer was removed
    // AND no modifications were done to sources on the copy stack
    if (m_lines_to_remove.ainsert_buffer == m_ainsert_buffer.size() && m_copy_suspended && !m_copy_files_aread_ready)
        resume_copy(0, resume_copy::ignore_line);

    return extract_next_logical_line_result::normal;
}


extract_next_logical_line_result opencode_provider::extract_next_logical_line_from_copy_buffer()
{
    auto& opencode_copy_stack = m_ctx->hlasm_ctx->opencode_copy_stack();
    assert(&opencode_copy_stack == &m_ctx->hlasm_ctx->current_copy_stack());

    auto& copy_file = m_copy_files.back();
    if (!lexing::extract_logical_line(m_current_logical_line, copy_file.text, lexing::default_ictl_copy))
        return extract_next_logical_line_result::failed;

    m_current_logical_line_source.begin_line = copy_file.line_no;
    m_current_logical_line_source.end_line = copy_file.line_no + m_current_logical_line.segments.size() - 1;
    m_current_logical_line_source.begin_offset =
        m_current_logical_line.segments.front().code.data() - copy_file.full_text.data();
    m_current_logical_line_source.end_offset =
        copy_file.text.size() ? copy_file.text.data() - copy_file.full_text.data() : copy_file.full_text.size();
    m_current_logical_line_source.source = logical_line_origin::source_type::copy;

    copy_file.line_no += m_current_logical_line.segments.size() - 1;

    bool restarted = false;
    while (!m_copy_files.empty())
    {
        if (!m_copy_files.back().text.empty())
        {
            restarted = resume_copy(m_copy_files.back().line_no, resume_copy::exact_or_next_line);
            if (restarted)
                break;
        }
        m_copy_files.pop_back();
        opencode_copy_stack.pop_back();
    }

    if (m_copy_files.empty())
        restarted = resume_copy(0, resume_copy::ignore_line);

    assert(restarted);

    m_copy_files.clear();

    return extract_next_logical_line_result::normal;
}

extract_next_logical_line_result opencode_provider::extract_next_logical_line()
{
    apply_pending_line_changes();

    bool ictl_allowed = false;
    if (m_opts.ictl_allowed)
    {
        m_opts.ictl_allowed = false;
        ictl_allowed = true;
    }

    m_current_logical_line.clear();
    m_current_logical_line_source = {};

    if (m_ainsert_buffer.size())
        return extract_next_logical_line_from_ainsert_buffer();

    if (m_copy_files_aread_ready)
    {
        m_copy_files_aread_ready = false;

        assert(&m_ctx->hlasm_ctx->current_copy_stack() == &m_ctx->hlasm_ctx->opencode_copy_stack());

        if (m_copy_suspended)
        {
            if (m_copy_files.empty())
                resume_copy(0, resume_copy::ignore_line);
            else if (resume_copy(m_copy_files.back().line_no, resume_copy::exact_line_match))
            {
                // copy processing was resumed, pre-parsed statements will be executed
                m_copy_files.clear();
                return extract_next_logical_line_result::failed;
            }
        }
    }

    if (!m_copy_files.empty())
        return extract_next_logical_line_from_copy_buffer();

    if (!m_preprocessor_buffer.empty())
    {
        // TODO: other sources
    }

    if (ictl_allowed)
        ictl_allowed = is_next_line_ictl();

    if (!ictl_allowed && m_opts.process_remaining)
    {
        if (!is_next_line_process())
            m_opts.process_remaining = 0;
        else
        {
            ++m_lines_to_remove.current_text_lines;
            append_to_logical_line(m_current_logical_line, m_next_line_text, lexing::default_ictl);
            finish_logical_line(m_current_logical_line, lexing::default_ictl);
            --m_opts.process_remaining;

            m_current_logical_line_source.begin_line = m_current_line;
            m_current_logical_line_source.end_line = m_current_line + m_current_logical_line.segments.size() - 1;
            m_current_logical_line_source.begin_offset =
                m_current_logical_line.segments.front().code.data() - m_original_text.data();
            m_current_logical_line_source.end_offset =
                m_next_line_text.size() ? m_next_line_text.data() - m_original_text.data() : m_original_text.size();
            m_current_logical_line_source.source = logical_line_origin::source_type::file;

            return extract_next_logical_line_result::process;
        }
    }

    while (m_next_line_text.size())
    {
        ++m_lines_to_remove.current_text_lines;
        if (!append_to_logical_line(m_current_logical_line, m_next_line_text, lexing::default_ictl))
            break;
    }
    finish_logical_line(m_current_logical_line, lexing::default_ictl);

    if (m_current_logical_line.segments.empty())
        return extract_next_logical_line_result::failed;

    m_current_logical_line_source.begin_line = m_current_line;
    m_current_logical_line_source.end_line = m_current_line + m_current_logical_line.segments.size() - 1;
    m_current_logical_line_source.begin_offset =
        m_current_logical_line.segments.front().code.data() - m_original_text.data();
    m_current_logical_line_source.end_offset =
        m_next_line_text.size() ? m_next_line_text.data() - m_original_text.data() : m_original_text.size();
    m_current_logical_line_source.source = logical_line_origin::source_type::file;

    if (ictl_allowed)
        return extract_next_logical_line_result::ictl;

    return extract_next_logical_line_result::normal;
}

const parsing::parser_holder& opencode_provider::prepare_operand_parser(const std::string& text,
    context::hlasm_context& hlasm_ctx,
    parsing::parser_error_listener_ctx* err_listener,
    semantics::range_provider range_prov,
    range text_range,
    const processing_status& proc_status,
    bool unlimited_line)
{
    auto& h = *m_operand_parser;

    h.input->reset(text);

    h.lex->reset();
    h.lex->set_file_offset(text_range.start);
    h.lex->set_unlimited_line(unlimited_line);

    h.stream->reset();

    h.parser->reinitialize(&hlasm_ctx, std::move(range_prov), proc_status, err_listener);
    h.parser->removeErrorListeners();
    if (err_listener)
        h.parser->addErrorListener(err_listener);

    h.parser->reset();

    h.parser->get_collector().prepare_for_next_statement();

    return h;
}

void opencode_provider::suspend_copy()
{
    m_state_listener->suspend_opencode_copy_processing();
    m_copy_suspended = true;
}

bool opencode_provider::resume_copy(size_t line_no, processing::resume_copy resume_opts)
{
    if (m_state_listener->resume_opencode_copy_processing_at(line_no, resume_opts))
    {
        m_copy_suspended = false;
        return true;
    }
    else
        return false;
}

} // namespace hlasm_plugin::parser_library::processing
