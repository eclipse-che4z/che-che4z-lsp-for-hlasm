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

#include "analyzer.h"
#include "hlasmparser.h"
#include "lexing/token_stream.h"
#include "parsing/error_strategy.h"
#include "parsing/parser_impl.h"
#include "processing/error_statement.h"
#include "semantics/collector.h"
#include "semantics/range_provider.h"

namespace hlasm_plugin::parser_library::processing {

namespace {
struct dummy_vfm final : public virtual_file_monitor
{
    virtual_file_handle file_generated(std::string_view content) override { return virtual_file_handle(); }
};
dummy_vfm fallback_vfm;
} // namespace

opencode_provider::opencode_provider(std::string_view text,
    analyzing_context& ctx,
    workspaces::parse_lib_provider& lib_provider,
    processing_state_listener& state_listener,
    semantics::source_info_processor& src_proc,
    diagnosable_ctx& diag_consumer,
    std::unique_ptr<preprocessor> preprocessor,
    opencode_provider_options opts,
    virtual_file_monitor* virtual_file_monitor)
    : statement_provider(statement_provider_kind::OPEN)
    , m_original_text(text)
    , m_next_line_text(text)
    , m_parser(parsing::parser_holder::create(&src_proc, ctx.hlasm_ctx.get(), &diag_consumer))
    , m_lookahead_parser(parsing::parser_holder::create(nullptr, ctx.hlasm_ctx.get(), nullptr))
    , m_operand_parser(parsing::parser_holder::create(nullptr, ctx.hlasm_ctx.get(), nullptr))
    , m_ctx(&ctx)
    , m_lib_provider(&lib_provider)
    , m_state_listener(&state_listener)
    , m_src_proc(&src_proc)
    , m_diagnoser(&diag_consumer)
    , m_opts(opts)
    , m_preprocessor(std::move(preprocessor))
    , m_virtual_file_monitor(virtual_file_monitor ? virtual_file_monitor : &fallback_vfm)
{}

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

    if (auto rest_len = lexing::length_utf16(rest))
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
    }
    else if (suspend_copy_processing(remove_empty::yes)
        || m_preprocessor && try_running_preprocessor() && suspend_copy_processing(remove_empty::yes))
    {
        auto& opencode_stack = m_ctx->hlasm_ctx->opencode_copy_stack();
        auto& copy = opencode_stack.back();
        const auto line = copy.suspended_at;
        std::string_view remaining_text =
            m_ctx->lsp_ctx->get_file_info(copy.definition_location()->file)->data.get_lines_beginning_at({ line, 0 });
        result = lexing::extract_line(remaining_text).first;
        if (remaining_text.empty())
            copy.resume();
        else
            copy.suspend(line + 1);

        while (!opencode_stack.empty() && !opencode_stack.back().suspended())
            opencode_stack.pop_back();
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
        case ainsert_destination::back:
            m_ainsert_buffer.push_back(rec);
            break;
        case ainsert_destination::front:
            m_ainsert_buffer.push_front(rec);
            break;
    }
    // ainsert buffer may contain macro call that will remove code lines from copybooks
    // this prevents copybook unwinding
    suspend_copy_processing(remove_empty::no);
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
            auto skip_len = lexing::length_utf16(l.line.substr(0, l.code.data() - l.line.data()));
            auto code_len = lexing::length_utf16(l.code);

            m_src_proc->add_hl_symbol(
                token_info(range(position(line_no, skip_len), position(line_no, skip_len + code_len)),
                    semantics::hl_scopes::comment));
        }
        ++line_no;
    }
}

void opencode_provider::generate_continuation_error_messages(diagnostic_op_consumer* diags) const
{
    auto line_no = m_current_logical_line_source.begin_line;
    for (const auto& s : m_current_logical_line.segments)
    {
        if (s.continuation_error)
        {
            diags->add_diagnostic(
                diagnostic_op::error_E001(range { { line_no, 0 }, { line_no, s.code_offset_utf16 } }));

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
        && proc_status.first.form != processing_form::IGNORED
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
    const range& op_range,
    diagnostic_op_consumer* diags)
{
    diagnostic_consumer_transform drop_diags([](diagnostic_op) {});

    if (proc.kind == processing_kind::ORDINARY
        && try_trigger_attribute_lookahead(
            collector.current_instruction(), { *m_ctx->hlasm_ctx, *m_lib_provider, drop_diags }, *m_state_listener))
        return nullptr;

    m_ctx->hlasm_ctx->set_source_position(collector.current_instruction().field_range.start);
    auto proc_status = proc.get_processing_status(collector.peek_instruction());

    if (op_text)
    {
        const auto& h = prepare_operand_parser(*op_text, *m_ctx->hlasm_ctx, diags, {}, op_range, proc_status, false);

        const auto& [format, opcode] = proc_status;
        if (format.occurence == operand_occurence::ABSENT || format.form == processing_form::UNKNOWN)
            h.parser->op_rem_body_noop();
        else
        {
            switch (format.form)
            {
                case processing_form::IGNORED:
                    h.parser->op_rem_body_ignored();
                    break;
                case processing_form::DEFERRED:
                    h.parser->op_rem_body_deferred();
                    break;
                case processing_form::CA:
                    h.parser->op_rem_body_ca();
                    break;
                case processing_form::MAC: {
                    auto rule = h.parser->op_rem_body_mac();
                    auto line = std::move(rule->line);
                    auto line_range = rule->line_range;

                    if (line.operands.size())
                    {
                        auto [to_parse, ranges, r] = join_operands(line.operands);

                        semantics::range_provider tmp_provider(r, ranges, semantics::adjusting_state::MACRO_REPARSE);

                        const auto& h_second = prepare_operand_parser(
                            to_parse, *m_ctx->hlasm_ctx, diags, std::move(tmp_provider), r, proc_status, true);

                        line.operands = std::move(h_second.parser->macro_ops()->list);
                        h.parser->get_collector().set_literals(h_second.parser->get_collector().take_literals());
                    }

                    h.parser->get_collector().set_operand_remark_field(
                        std::move(line.operands), std::move(line.remarks), line_range);
                }
                break;
                case processing_form::ASM:
                    h.parser->op_rem_body_asm();
                    break;
                case processing_form::MACH:
                    h.parser->op_rem_body_mach();
                    if (auto& h_collector = h.parser->get_collector(); h_collector.has_operands())
                        transform_reloc_imm_operands(h_collector.current_operands().value, opcode.value);
                    break;
                case processing_form::DAT:
                    h.parser->op_rem_body_dat();
                    break;
                default:
                    break;
            }
        }

        if (format.form != processing_form::IGNORED)
        {
            collector.append_operand_field(std::move(h.parser->get_collector()));
        }
    }
    range statement_range(position(m_current_logical_line_source.begin_line, 0)); // assign default
    auto result = collector.extract_statement(proc_status, statement_range);

    if (proc.kind == processing_kind::ORDINARY
        && try_trigger_attribute_lookahead(
            *result, { *m_ctx->hlasm_ctx, *m_lib_provider, drop_diags }, *m_state_listener))
        return nullptr;

    if (m_current_logical_line.segments.size() > 1)
        m_ctx->hlasm_ctx->metrics.continued_statements++;
    else
        m_ctx->hlasm_ctx->metrics.non_continued_statements++;

    m_src_proc->process_hl_symbols(collector.extract_hl_symbols());

    return result;
}

std::string generate_virtual_file_name(virtual_file_handle handle, std::string_view name)
{
    std::string result;
    if (auto id = handle.file_id(); id)
    {
        result += "hlasm://";
        result += std::to_string(id.value());
        result += "/";
    }
    result += name;
    return result;
}

bool opencode_provider::try_running_preprocessor()
{
    const auto current_line = m_current_line;
    auto result = m_preprocessor->generate_replacement(m_next_line_text, m_current_line);
    if (!result.has_value() || result.value().empty())
        return false;

    auto virtual_file_name = m_ctx->hlasm_ctx->ids().add("preprocessor:" + std::to_string(current_line));

    auto [new_file, inserted] = m_virtual_files.try_emplace(virtual_file_name, std::move(result.value()));

    // set up "call site"
    const auto current_offset =
        m_next_line_text.empty() ? m_original_text.size() : m_next_line_text.data() - m_original_text.data();
    const auto last_statement_line = m_current_line - (current_line != m_current_line);
    m_ctx->hlasm_ctx->set_source_position(position(last_statement_line, 0));
    m_ctx->hlasm_ctx->set_source_indices(current_offset, current_offset, last_statement_line);

    if (inserted)
    {
        auto vf_handle = m_virtual_file_monitor->file_generated(new_file->second);
        m_vf_handles.push_back(vf_handle);

        analyzer a(new_file->second,
            analyzer_options {
                generate_virtual_file_name(std::move(vf_handle), *virtual_file_name),
                m_lib_provider,
                *m_ctx,
                workspaces::library_data { processing_kind::COPY, virtual_file_name },
            });
        a.analyze();
        m_diagnoser->collect_diags_from_child(a);
    }
    else
    {
        assert(result.value() == new_file->second);
    }

    m_ctx->hlasm_ctx->enter_copy_member(virtual_file_name);

    return true;
}

bool opencode_provider::suspend_copy_processing(remove_empty re) const
{
    auto& opencode_stack = m_ctx->hlasm_ctx->opencode_copy_stack();

    if (opencode_stack.empty())
        return false;

    for (auto& copy : opencode_stack)
    {
        if (copy.suspended())
            continue;

        const auto pos = copy.current_statement_position();
        std::string_view remaining_text =
            m_ctx->lsp_ctx->get_file_info(copy.definition_location()->file)->data.get_lines_beginning_at(pos);
        const size_t line_no = pos.line;

        // remove line being processed
        lexing::logical_line ll = {};
        if (const bool before_first_read = copy.current_statement == (size_t)-1; !before_first_read)
            lexing::extract_logical_line(ll, remaining_text, lexing::default_ictl_copy);

        copy.suspend(line_no + ll.segments.size());
        if (remaining_text.empty())
        {
            if (re == remove_empty::yes)
                copy.resume();
            copy.current_statement = copy.cached_definition()->size() - 1;
        }
    }
    if (re == remove_empty::yes)
    {
        while (!opencode_stack.empty() && !opencode_stack.back().suspended())
            opencode_stack.pop_back();
    }

    return !opencode_stack.empty();
}

void opencode_provider::convert_ainsert_buffer_to_copybook()
{
    std::string result;
    result.reserve(m_ainsert_buffer.size() * (80 + 1));
    for (const auto& s : m_ainsert_buffer)
        result.append(s).push_back('\n');
    m_ainsert_buffer.clear();

    auto virtual_copy_name =
        m_ctx->hlasm_ctx->ids().add("AINSERT:" + std::to_string(m_ctx->hlasm_ctx->obtain_ainsert_id()));

    auto new_file = m_virtual_files.try_emplace(virtual_copy_name, std::move(result)).first;

    auto vf_handle = m_virtual_file_monitor->file_generated(new_file->second);
    m_vf_handles.push_back(vf_handle);

    analyzer a(new_file->second,
        analyzer_options {
            generate_virtual_file_name(std::move(vf_handle), *virtual_copy_name),
            m_lib_provider,
            *m_ctx,
            workspaces::library_data { processing_kind::COPY, virtual_copy_name },
        });
    a.analyze();
    m_diagnoser->collect_diags_from_child(a);

    m_ctx->hlasm_ctx->enter_copy_member(virtual_copy_name);
}

context::shared_stmt_ptr opencode_provider::get_next(const statement_processor& proc)
{
    const bool lookahead = proc.kind == processing_kind::LOOKAHEAD;
    const bool nested = proc.kind == processing_kind::MACRO || proc.kind == processing_kind::COPY;

    auto& ph = lookahead ? *m_lookahead_parser : *m_parser;

    auto feed_line_res = feed_line(ph);
    if (feed_line_res == extract_next_logical_line_result::failed)
        return nullptr;

    auto& collector = ph.parser->get_collector();
    auto* diag_target = nested ? collector.diag_collector() : static_cast<diagnostic_op_consumer*>(m_diagnoser);

    if (m_current_logical_line.continuation_error)
    {
        // report continuation errors immediately
        if (proc.kind == processing_kind::MACRO)
            generate_continuation_error_messages(static_cast<diagnostic_op_consumer*>(m_diagnoser));
        // lookahead may read something that will be removed from the input stream later on
        else if (!lookahead)
            generate_continuation_error_messages(diag_target);
    }

    if (feed_line_res != extract_next_logical_line_result::process && is_comment())
    {
        if (!lookahead)
            process_comment();
        return nullptr;
    }

    if (!lookahead)
        ph.parser->set_diagnoser(diag_target);

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
    {
        if (proc.kind == processing_kind::MACRO)
        {
            // these kinds of errors are reported right away,
            for (auto& diag : collector.diag_container().diags)
                m_diagnoser->add_diagnostic(std::move(diag));
            // indicate errors were produced, but do not report them again
            return std::make_shared<error_statement>(
                range(position(m_current_logical_line_source.begin_line, 0)), std::vector<diagnostic_op>());
        }
        else if (lookahead)
            return nullptr;
        else
            return std::make_shared<error_statement>(range(position(m_current_logical_line_source.begin_line, 0)),
                std::move(collector.diag_container().diags));
    }

    m_ctx->hlasm_ctx->set_source_indices(m_current_logical_line_source.begin_offset,
        m_current_logical_line_source.end_offset,
        m_current_logical_line_source.end_line);

    return lookahead ? process_lookahead(proc, collector, op_text, op_range)
                     : process_ordinary(proc, collector, op_text, op_range, diag_target);
}

bool opencode_provider::finished() const
{
    if (!m_next_line_text.empty())
        return false;
    if (!m_ctx->hlasm_ctx->in_opencode())
        return true;
    if (!m_ainsert_buffer.empty())
        return false;
    if (m_preprocessor && !m_preprocessor->finished())
        return false;
    const auto& o = m_ctx->hlasm_ctx->opencode_copy_stack();
    if (o.empty())
        return true;
    return std::none_of(o.begin(), o.end(), [](const auto& c) { return c.suspended(); });
}

parsing::hlasmparser& opencode_provider::parser()
{
    if (!m_line_fed)
        feed_line(*m_parser);
    assert(m_line_fed);
    return *m_parser->parser;
}

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

extract_next_logical_line_result opencode_provider::extract_next_logical_line_from_copy_buffer()
{
    assert(m_ctx->hlasm_ctx->in_opencode());

    auto& opencode_copy_stack = m_ctx->hlasm_ctx->opencode_copy_stack();
    while (!opencode_copy_stack.empty())
    {
        auto& copy_file = opencode_copy_stack.back();
        if (!copy_file.suspended())
            return extract_next_logical_line_result::failed; // copy processing resumed
        const auto line = copy_file.suspended_at;

        size_t resync = (size_t)-1;
        for (const auto& stmt : *copy_file.cached_definition())
        {
            auto stmt_line = stmt.get_base()->statement_position().line;
            if (stmt_line == line)
            {
                copy_file.current_statement = resync;
                copy_file.resume();
                return extract_next_logical_line_result::failed; // copy processing resumed
            }
            if (stmt_line > line)
                break;
            ++resync;
        }
        copy_file.current_statement = resync;

        const auto* copy_text = m_ctx->lsp_ctx->get_file_info(copy_file.definition_location()->file);
        std::string_view full_text = copy_text->data.get_lines_beginning_at({ 0, 0 });
        std::string_view remaining_text = copy_text->data.get_lines_beginning_at({ line, 0 });
        if (!lexing::extract_logical_line(m_current_logical_line, remaining_text, lexing::default_ictl_copy))
        {
            opencode_copy_stack.pop_back();
            continue;
        }

        m_current_logical_line_source.begin_line = line;
        m_current_logical_line_source.end_line = line + m_current_logical_line.segments.size() - 1;
        m_current_logical_line_source.begin_offset =
            m_current_logical_line.segments.front().code.data() - full_text.data();
        m_current_logical_line_source.end_offset =
            remaining_text.size() ? remaining_text.data() - full_text.data() : full_text.size();
        m_current_logical_line_source.source = logical_line_origin::source_type::copy;

        copy_file.resume();

        return extract_next_logical_line_result::normal; // unaligned statement extracted
    }
    return extract_next_logical_line_result::failed; // next round
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

    if (m_ctx->hlasm_ctx->in_opencode())
    {
        if (!m_ainsert_buffer.empty())
        {
            convert_ainsert_buffer_to_copybook();
            return extract_next_logical_line_result::failed;
        }

        if (!m_ctx->hlasm_ctx->opencode_copy_stack().empty())
            return extract_next_logical_line_from_copy_buffer();
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

    if (m_preprocessor && try_running_preprocessor())
        return extract_next_logical_line_result::failed;

    while (!m_next_line_text.empty())
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
    diagnostic_op_consumer* diags,
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


    h.parser->reinitialize(&hlasm_ctx, std::move(range_prov), proc_status, diags);

    h.parser->reset();

    h.parser->get_collector().prepare_for_next_statement();

    return h;
}

} // namespace hlasm_plugin::parser_library::processing
