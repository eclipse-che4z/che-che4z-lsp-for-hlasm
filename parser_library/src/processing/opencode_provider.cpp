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

#include <algorithm>

#include "analyzer.h"
#include "hlasmparser_multiline.h"
#include "lexing/input_source.h"
#include "lexing/token_stream.h"
#include "library_info_transitional.h"
#include "lsp/lsp_context.h"
#include "parsing/error_strategy.h"
#include "parsing/parser_impl.h"
#include "processing/error_statement.h"
#include "processing/processing_manager.h"
#include "semantics/collector.h"
#include "semantics/range_provider.h"
#include "utils/text_matchers.h"
#include "utils/unicode_text.h"

namespace hlasm_plugin::parser_library::processing {

opencode_provider::opencode_provider(std::string_view text,
    const analyzing_context& ctx,
    parse_lib_provider& lib_provider,
    processing_state_listener& state_listener,
    const processing::processing_manager& proc_manager,
    semantics::source_info_processor& src_proc,
    diagnosable_ctx& diag_consumer,
    std::unique_ptr<preprocessor> prep,
    opencode_provider_options opts,
    virtual_file_monitor* virtual_file_monitor,
    std::vector<std::pair<virtual_file_handle, utils::resource::resource_location>>& vf_handles)
    : statement_provider(statement_provider_kind::OPEN)
    , m_input_document(text)
    , m_virtual_files(std::make_shared<std::unordered_map<context::id_index, std::string>>())
    , m_singleline { parsing::parser_holder::create(&src_proc, ctx.hlasm_ctx.get(), &diag_consumer, false),
        parsing::parser_holder::create(nullptr, ctx.hlasm_ctx.get(), nullptr, false),
        parsing::parser_holder::create(nullptr, ctx.hlasm_ctx.get(), nullptr, false) }
    , m_multiline { parsing::parser_holder::create(&src_proc, ctx.hlasm_ctx.get(), &diag_consumer, true),
        parsing::parser_holder::create(nullptr, ctx.hlasm_ctx.get(), nullptr, true),
        parsing::parser_holder::create(nullptr, ctx.hlasm_ctx.get(), nullptr, true) }
    , m_ctx(ctx)
    , m_lib_provider(&lib_provider)
    , m_state_listener(&state_listener)
    , m_processing_manager(proc_manager)
    , m_src_proc(&src_proc)
    , m_diagnoser(&diag_consumer)
    , m_opts(opts)
    , m_preprocessor(std::move(prep))
    , m_virtual_file_monitor(virtual_file_monitor ? virtual_file_monitor : this)
    , m_vf_handles(vf_handles)
{}

opencode_provider::~opencode_provider() = default;

utils::task opencode_provider::start_preprocessor()
{
    m_input_document = co_await m_preprocessor->generate_replacement(std::move(m_input_document));
}

void opencode_provider::onetime_action()
{
    if (m_preprocessor)
        m_state_listener->schedule_helper_task(start_preprocessor());
}

void opencode_provider::rewind_input(context::source_position pos)
{
    m_ainsert_buffer.clear(); // this needs to be tested, but apparently AGO clears AINSERT buffer
    assert(pos.rewind_target <= m_input_document.size());
    m_next_line_index = pos.rewind_target;
}

void opencode_provider::generate_aread_highlighting(std::string_view text, size_t line_no) const
{
    if (text.empty())
        return;

    auto [rest, utf16_skipped] = utils::skip_chars(text, 80);
    if (utf16_skipped)
        m_src_proc->add_hl_symbol(
            token_info(range(position(line_no, 0), position(line_no, utf16_skipped)), hl_scopes::string));

    if (rest.empty())
        return;

    if (auto rest_len = utils::length_utf16(rest))
        m_src_proc->add_hl_symbol(token_info(
            range(position(line_no, utf16_skipped), position(line_no, utf16_skipped + rest_len)), hl_scopes::ignored));
}

std::variant<std::string, utils::value_task<std::string>> opencode_provider::aread()
{
    if (!m_ainsert_buffer.empty())
    {
        auto result = std::move(m_ainsert_buffer.front());
        m_ainsert_buffer.pop_front();
        result.resize(80, ' ');
        return result;
    }

    if (suspend_copy_processing(remove_empty::yes))
        return aread_from_copybook();

    if (should_run_preprocessor())
    {
        if (auto t = run_preprocessor(); t.valid())
            return deferred_aread(std::move(t));
        if (suspend_copy_processing(remove_empty::yes))
            return aread_from_copybook();
    }

    return try_aread_from_document();
}

utils::value_task<std::string> opencode_provider::deferred_aread(utils::task prep_task)
{
    co_await std::move(prep_task);

    if (suspend_copy_processing(remove_empty::yes))
        co_return aread_from_copybook();

    co_return try_aread_from_document();
}

std::string opencode_provider::aread_from_copybook() const
{
    auto& opencode_stack = m_ctx.hlasm_ctx->opencode_copy_stack();
    auto& copy = opencode_stack.back();
    const auto line = copy.suspended_at;
    std::string_view remaining_text = m_ctx.lsp_ctx->get_file_info(copy.definition_location()->resource_loc)
                                          ->data.get_lines_beginning_at({ line, 0 });
    std::string result(lexing::extract_line(remaining_text).first);
    if (remaining_text.empty())
        copy.resume();
    else
        copy.suspend(line + 1);

    while (!opencode_stack.empty() && !opencode_stack.back().suspended())
        opencode_stack.pop_back();

    result.resize(80, ' ');

    return result;
}
std::string opencode_provider::try_aread_from_document()
{
    if (m_next_line_index >= m_input_document.size())
        return std::string();

    const auto& line = m_input_document.at(m_next_line_index++);
    auto line_text = line.text();
    std::string result(lexing::extract_line(line_text).first);
    if (auto lineno = line.lineno(); lineno.has_value())
    {
        m_processing_manager.aread_cb(*lineno, result);
        generate_aread_highlighting(result, *lineno);
    }

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

void opencode_provider::feed_line(const parsing::parser_holder& p, bool is_process)
{
    m_line_fed = true;

    const auto& subs = p.input->new_input(m_current_logical_line);

    if (subs.server && !std::exchange(m_encoding_warning_issued.server, true))
        m_diagnoser->add_diagnostic(
            diagnostic_op::warning_W017(range(position(m_current_logical_line_source.begin_line, 0))));

    if (subs.client && !std::exchange(m_encoding_warning_issued.client, true))
        m_diagnoser->add_diagnostic(
            diagnostic_op::warning_W018(range(position(m_current_logical_line_source.begin_line, 0))));

    p.lex->set_file_offset(
        { m_current_logical_line_source.begin_line, 0 /*lexing::default_ictl.begin-1 really*/ }, 0, is_process);
    p.lex->set_unlimited_line(false);
    p.lex->reset();

    p.stream->reset();

    p.parser->reset();

    p.parser->get_collector().prepare_for_next_statement();

    m_ctx.hlasm_ctx->metrics.lines += m_current_logical_line.segments.size();
}

bool opencode_provider::is_comment()
{
    using string_matcher = utils::text_matchers::basic_string_matcher<true, false>;
    static constexpr const auto comment = utils::text_matchers::alt(string_matcher("*"), string_matcher(".*"));

    auto b = m_current_logical_line.segments.front().code;
    const auto e = m_current_logical_line.segments.front().continuation;

    return comment(b, e);
}

void opencode_provider::process_comment()
{
    size_t line_no = m_current_logical_line_source.begin_line;
    for (const auto& l : m_current_logical_line.segments)
    {
        if (l.code != l.continuation)
        {
            auto skip_len = lexing::logical_distance(l.begin, l.code);
            auto code_len = lexing::logical_distance(l.begin, l.continuation);

            m_src_proc->add_hl_symbol(
                token_info(range(position(line_no, skip_len), position(line_no, code_len)), hl_scopes::comment));
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
            diags->add_diagnostic(diagnostic_op::error_E001(
                range { { line_no, 0 }, { line_no, lexing::logical_distance(s.begin, s.code) } }));

            break;
        }
        ++line_no;
    }
}

namespace {
bool operands_relevant_in_lookahead(bool has_label, const processing_status& status)
{
    static constexpr context::id_index EQU("EQU");
    return status.first.form == processing_form::ASM && status.second.value == context::id_storage::well_known::COPY
        || status.first.form == processing_form::ASM && status.second.value == EQU && has_label
        || status.first.form == processing_form::DAT && has_label;
}
} // namespace

std::shared_ptr<const context::hlasm_statement> opencode_provider::process_lookahead(
    const statement_processor& proc, semantics::collector& collector, op_data operands)
{
    const auto& current_instr = collector.current_instruction();

    // Lookahead processor always returns value
    auto proc_status =
        proc.get_processing_status(proc.resolve_instruction(current_instr), current_instr.field_range).value();

    m_ctx.hlasm_ctx->set_source_position(current_instr.field_range.start);

    const auto& [op_text, op_range, op_logical_column] = operands;

    if (op_text && operands_relevant_in_lookahead(collector.has_label(), proc_status))
    // optimization : if statement has no label and is not COPY, do not even parse operands)
    // optimization : only COPY, EQU and DC/DS/DXD statements actually need operands in lookahead mode
    {
        const auto& h = prepare_operand_parser(*op_text,
            *m_ctx.hlasm_ctx,
            nullptr,
            semantics::range_provider(),
            op_range,
            op_logical_column,
            proc_status,
            true);

        switch (proc_status.first.form)
        {
            case processing_form::ASM:
                h.lookahead_operands_and_remarks_asm();
                break;
            case processing_form::DAT:
                h.lookahead_operands_and_remarks_dat();
                break;
        }

        h.parser->get_collector().clear_hl_symbols();
        collector.append_operand_field(std::move(h.parser->get_collector()));
    }
    range statement_range(position(m_current_logical_line_source.begin_line, 0)); // assign default
    auto result = collector.extract_statement(proc_status, statement_range);

    if (m_current_logical_line.segments.size() > 1)
        m_ctx.hlasm_ctx->metrics.continued_statements++;
    else
        m_ctx.hlasm_ctx->metrics.non_continued_statements++;

    return result;
}

constexpr bool is_multiline(std::string_view v)
{
    auto nl = v.find_first_of("\r\n");
    if (nl == std::string_view::npos)
        return false;
    v.remove_prefix(nl);
    v.remove_prefix(1 + v.starts_with("\r\n"));

    return !v.empty();
}

std::shared_ptr<const context::hlasm_statement> opencode_provider::process_ordinary(const statement_processor& proc,
    semantics::collector& collector,
    op_data operands,
    diagnostic_op_consumer* diags,
    std::optional<context::id_index> resolved_instr)
{
    const auto& current_instr = collector.current_instruction();
    m_ctx.hlasm_ctx->set_source_position(current_instr.field_range.start);

    const auto proc_status_o = proc.get_processing_status(resolved_instr, current_instr.field_range);
    if (!proc_status_o.has_value()) [[unlikely]]
    {
        m_restart_process_ordinary.emplace(
            process_ordinary_restart_data { proc, collector, std::move(operands), diags, std::move(resolved_instr) });
        return nullptr;
    }
    const auto& proc_status = proc_status_o.value();

    const auto& [op_text, op_range, op_logical_column] = operands;

    if (op_text)
    {
        collector.starting_operand_parsing();

        diagnostic_consumer_transform diags_filter([&diags](diagnostic_op diag) {
            if (static const auto template_diag = diagnostic_op::error_E049("", range());
                diags && template_diag.code == diag.code)
                diags->add_diagnostic(std::move(diag));
        });

        const auto& [format, opcode] = proc_status;

        const auto& h = prepare_operand_parser(*op_text,
            *m_ctx.hlasm_ctx,
            (format.occurrence == operand_occurrence::PRESENT && format.form == processing_form::UNKNOWN)
                ? &diags_filter
                : diags,
            semantics::range_provider(),
            op_range,
            op_logical_column,
            proc_status,
            false);

        if (format.occurrence == operand_occurrence::ABSENT)
            h.op_rem_body_noop();
        else
        {
            switch (format.form)
            {
                case processing_form::IGNORED:
                    h.op_rem_body_ignored();
                    break;
                case processing_form::DEFERRED:
                    h.op_rem_body_deferred();
                    break;
                case processing_form::CA: {
                    using wk = context::id_storage::well_known;
                    bool var_def = opcode.value == wk::GBLA || opcode.value == wk::GBLB || opcode.value == wk::GBLC
                        || opcode.value == wk::LCLA || opcode.value == wk::LCLB || opcode.value == wk::LCLC;
                    bool branchlike = opcode.value == wk::AIF || opcode.value == wk::AGO || opcode.value == wk::AIFB
                        || opcode.value == wk::AGOB;
                    if (var_def)
                        h.op_rem_body_ca_var_def();
                    else if (branchlike)
                        h.op_rem_body_ca_branch();
                    else
                        h.op_rem_body_ca_expr();
                    (void)h.parser->get_collector().take_literals(); // drop literals
                    break;
                }
                case processing_form::ASM:
                    h.op_rem_body_asm();
                    break;
                case processing_form::MACH:
                    h.op_rem_body_mach();
                    if (auto& h_collector = h.parser->get_collector(); h_collector.has_operands())
                        transform_reloc_imm_operands(h_collector.current_operands().value, opcode.value);
                    break;
                case processing_form::DAT:
                    h.op_rem_body_dat();
                    break;
                default: {
                    auto [line, line_range, line_logical_column] = h.op_rem_body_mac();

                    if (h.error_handler->error_reported())
                    {
                        line.operands.clear();
                    }
                    else if (line.operands.size())
                    {
                        auto [to_parse, ranges, r] = join_operands(line.operands);

                        semantics::range_provider tmp_provider(
                            r, ranges, semantics::adjusting_state::MACRO_REPARSE, h.lex->get_line_limits());

                        const auto& h_second = prepare_operand_parser(to_parse,
                            *m_ctx.hlasm_ctx,
                            format.form == processing_form::UNKNOWN ? &diags_filter : diags,
                            std::move(tmp_provider),
                            r,
                            line_logical_column,
                            proc_status,
                            true);

                        line.operands = h_second.macro_ops();

                        auto& c = h.parser->get_collector();
                        auto& c_s = h_second.parser->get_collector();
                        if (&c != &c_s)
                        {
                            c.set_literals(c_s.take_literals());
                            c.set_hl_symbols(c_s.extract_hl_symbols());
                        }
                    }

                    h.parser->get_collector().set_operand_remark_field(
                        std::move(line.operands), std::move(line.remarks), line_range);

                    break;
                }
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
        && try_trigger_attribute_lookahead(*result,
            { *m_ctx.hlasm_ctx, library_info_transitional(*m_lib_provider), drop_diagnostic_op },
            *m_state_listener,
            std::move(lookahead_references)))
        return nullptr;

    if (m_current_logical_line.segments.size() > 1)
        m_ctx.hlasm_ctx->metrics.continued_statements++;
    else
        m_ctx.hlasm_ctx->metrics.non_continued_statements++;

    m_src_proc->process_hl_symbols(collector.extract_hl_symbols());

    return result;
}

utils::resource::resource_location generate_virtual_file_name(virtual_file_id id, std::string_view name)
{
    std::string result;
    if (id)
    {
        result += "hlasm://";
        result += std::to_string(id.value());
        result += "/";
    }
    result += name;
    result += ".hlasm";
    return utils::resource::resource_location(std::move(result));
}

namespace {
size_t extract_current_line(size_t next_line_index, const document& doc)
{
    while (next_line_index--)
    {
        if (const auto& lineno = doc.at(next_line_index).lineno(); lineno.has_value())
            return lineno.value() + 1;
    }
    return 0;
}
} // namespace

bool opencode_provider::should_run_preprocessor() const noexcept
{
    return m_preprocessor && m_next_line_index < m_input_document.size()
        && !m_input_document.at(m_next_line_index).is_original();
}


std::pair<virtual_file_handle, std::string_view> opencode_provider::file_generated(std::string_view content)
{
    return { virtual_file_handle(std::shared_ptr<const virtual_file_id>(m_virtual_files, nullptr)), content };
}

utils::task opencode_provider::run_preprocessor()
{
    const auto current_line = extract_current_line(m_next_line_index, m_input_document);

    std::string preprocessor_text;
    auto it = m_input_document.begin() + m_next_line_index;
    for (; it != m_input_document.end() && !it->is_original(); ++it)
    {
        const auto text = it->text();
        preprocessor_text.append(text);
        if (text.empty() || text.back() != '\n')
            preprocessor_text.push_back('\n');
    }
    const size_t stop_line = it != m_input_document.end() ? it->lineno().value() : current_line;
    const auto last_index = it - m_input_document.begin();

    auto virtual_file_name = m_ctx.hlasm_ctx->ids().add("PREPROCESSOR_" + std::to_string(current_line));

    auto [new_file, inserted] = m_virtual_files->try_emplace(virtual_file_name, std::move(preprocessor_text));

    // set up "call site"
    const auto last_statement_line = stop_line - (stop_line != current_line);
    m_ctx.hlasm_ctx->set_source_position(position(last_statement_line, 0));
    m_ctx.hlasm_ctx->set_source_indices(m_next_line_index, last_index);
    m_next_line_index = last_index;


    if (!inserted)
    {
        assert(preprocessor_text == new_file->second); // isn't moved if insert fails
        m_ctx.hlasm_ctx->enter_copy_member(virtual_file_name);
        return utils::task();
    }
    else
    {
        auto [file_handle, file_text] = m_virtual_file_monitor->file_generated(new_file->second);
        auto file_location = generate_virtual_file_name(file_handle.file_id(), virtual_file_name.to_string_view());
        m_vf_handles.emplace_back(std::move(file_handle), file_location);
        return start_nested_parser(file_text,
            analyzer_options {
                std::move(file_location),
                m_lib_provider,
                m_ctx,
                analyzer_options::dependency(virtual_file_name.to_string(), processing_kind::COPY),
            },
            virtual_file_name);
    }
}

utils::task opencode_provider::start_nested_parser(
    std::string_view text, analyzer_options opts, context::id_index vf_name) const
{
    analyzer a(text, std::move(opts));
    co_await a.co_analyze();

    for (auto&& d : a.diags())
        m_diagnoser->add_diagnostic(std::move(d));

    m_ctx.hlasm_ctx->enter_copy_member(vf_name);
}

bool opencode_provider::suspend_copy_processing(remove_empty re) const
{
    auto& opencode_stack = m_ctx.hlasm_ctx->opencode_copy_stack();

    if (opencode_stack.empty())
        return false;

    for (auto& copy : opencode_stack)
    {
        if (copy.suspended())
            continue;

        const auto pos = copy.current_statement_position();
        std::string_view remaining_text =
            m_ctx.lsp_ctx->get_file_info(copy.definition_location()->resource_loc)->data.get_lines_beginning_at(pos);
        auto remaining_text_it = remaining_text.begin();
        const size_t line_no = pos.line;

        // remove line being processed
        lexing::logical_line<std::string_view::iterator> ll = {};
        if (copy.current_statement != context::statement_id())
            lexing::extract_logical_line(ll, remaining_text_it, remaining_text.end(), lexing::default_ictl_copy);

        copy.suspend(line_no + ll.segments.size());
        if (remaining_text_it == remaining_text.end())
        {
            if (re == remove_empty::yes)
                copy.resume();
            copy.current_statement = context::statement_id { copy.cached_definition()->size() - 1 };
        }
    }
    if (re == remove_empty::yes)
    {
        while (!opencode_stack.empty() && !opencode_stack.back().suspended())
            opencode_stack.pop_back();
    }

    return !opencode_stack.empty();
}

utils::task opencode_provider::convert_ainsert_buffer_to_copybook()
{
    std::string result;
    result.reserve(m_ainsert_buffer.size() * (80 + 1));
    for (const auto& s : m_ainsert_buffer)
        result.append(s).push_back('\n');
    m_ainsert_buffer.clear();

    auto virtual_copy_name =
        m_ctx.hlasm_ctx->ids().add("AINSERT_" + std::to_string(m_ctx.hlasm_ctx->obtain_ainsert_id()));

    auto new_file = m_virtual_files->try_emplace(virtual_copy_name, std::move(result)).first;

    auto [file_handle, file_text] = m_virtual_file_monitor->file_generated(new_file->second);
    auto file_location = generate_virtual_file_name(file_handle.file_id(), virtual_copy_name.to_string_view());
    m_vf_handles.emplace_back(std::move(file_handle), file_location);
    co_await start_nested_parser(file_text,
        analyzer_options {
            std::move(file_location),
            m_lib_provider,
            m_ctx,
            analyzer_options::dependency(virtual_copy_name.to_string(), processing_kind::COPY),
        },
        virtual_copy_name);
}

context::shared_stmt_ptr opencode_provider::get_next(const statement_processor& proc)
{
    if (m_restart_process_ordinary) [[unlikely]]
    {
        auto& [p, collector, operands, diags, resolved_instr] = *m_restart_process_ordinary;
        assert(&p == &proc);
        auto result = process_ordinary(p, collector, std::move(operands), diags, std::move(resolved_instr));
        m_restart_process_ordinary.reset();
        return result;
    }
    auto ll_res = extract_next_logical_line();
    if (ll_res == extract_next_logical_line_result::failed)
        return nullptr;
    const bool is_process = ll_res == extract_next_logical_line_result::process;
    const bool multiline = m_current_logical_line.segments.size() > 1;

    const bool lookahead = proc.kind == processing_kind::LOOKAHEAD;
    const bool nested = proc.kind == processing_kind::MACRO || proc.kind == processing_kind::COPY;

    auto& ph = lookahead ? (multiline ? *m_multiline.m_lookahead_parser : *m_singleline.m_lookahead_parser)
                         : (multiline ? *m_multiline.m_parser : *m_singleline.m_parser);
    feed_line(ph, is_process);

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

    if (!is_process && is_comment())
    {
        if (!lookahead)
            process_comment();
        return nullptr;
    }

    if (!lookahead)
        ph.parser->set_diagnoser(diag_target);

    auto operands = [](auto op) {
        auto&& [p1, p2, p3] = op;
        return op_data { std::move(p1), std::move(p2), std::move(p3) };
    }(lookahead ? ph.look_lab_instr() : ph.lab_instr());
    ph.parser->get_collector().resolve_first_part();

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

    m_ctx.hlasm_ctx->set_source_indices(
        m_current_logical_line_source.first_index, m_current_logical_line_source.last_index);

    if (lookahead)
        return process_lookahead(proc, collector, std::move(operands));

    if (proc.kind == processing_kind::ORDINARY
        && try_trigger_attribute_lookahead(collector.current_instruction(),
            { *m_ctx.hlasm_ctx, library_info_transitional(*m_lib_provider), drop_diagnostic_op },
            *m_state_listener,
            std::move(lookahead_references)))
        return nullptr;

    const auto& current_instr = collector.current_instruction();
    m_ctx.hlasm_ctx->set_source_position(current_instr.field_range.start);

    return process_ordinary(proc, collector, std::move(operands), diag_target, proc.resolve_instruction(current_instr));
}

bool opencode_provider::finished() const
{
    if (m_restart_process_ordinary.has_value())
        return false;
    if (m_next_line_index < m_input_document.size())
        return false;
    if (!m_ctx.hlasm_ctx->in_opencode())
        return true;
    if (!m_ainsert_buffer.empty())
        return false;
    const auto& o = m_ctx.hlasm_ctx->opencode_copy_stack();
    if (o.empty())
        return true;
    return std::ranges::none_of(o, &context::copy_member_invocation::suspended);
}

processing::preprocessor* opencode_provider::get_preprocessor()
{
    return m_preprocessor ? m_preprocessor.get() : nullptr;
}

parsing::hlasmparser_multiline& opencode_provider::parser()
{
    if (!m_line_fed)
    {
        auto ll_res = extract_next_logical_line();
        feed_line(*m_multiline.m_parser, ll_res == extract_next_logical_line_result::process);
    }
    assert(m_line_fed);
    return static_cast<parsing::hlasmparser_multiline&>(*m_multiline.m_parser->parser);
}

bool opencode_provider::is_next_line_ictl() const
{
    static constexpr std::string_view ICTL_LITERAL = "ICTL";

    const auto& current_line = m_input_document.at(m_next_line_index);
    if (!current_line.is_original()) // for now, let's say that ICTL can only be specified in the original
        return false;
    const auto current_line_text = current_line.text();
    const auto non_blank = current_line_text.find_first_not_of(' ');
    if (non_blank == std::string_view::npos || non_blank == 0)
        return false;
    const auto test_ictl = current_line_text.substr(non_blank);

    if (test_ictl.size() > ICTL_LITERAL.size() && test_ictl[ICTL_LITERAL.size()] != ' ')
        return false;

    auto possible_ictl = test_ictl.substr(0, ICTL_LITERAL.size());
    return std::ranges::equal(possible_ictl, ICTL_LITERAL, {}, [](unsigned char c) { return std::toupper(c); });
}

bool opencode_provider::is_next_line_process() const
{
    static constexpr std::string_view PROCESS_LITERAL = "*PROCESS";

    const auto& current_line = m_input_document.at(m_next_line_index);
    if (!current_line.is_original()) // for now, let's say that *PROCESS can only be specified in the original
        return false;

    const auto current_line_text = current_line.text();
    if (current_line_text.size() > PROCESS_LITERAL.size() && current_line_text[PROCESS_LITERAL.size()] != ' ')
        return false;

    const auto test_process = current_line_text.substr(0, PROCESS_LITERAL.size());
    return std::ranges::equal(test_process, PROCESS_LITERAL, {}, [](unsigned char c) { return std::toupper(c); });
}

extract_next_logical_line_result opencode_provider::extract_next_logical_line_from_copy_buffer()
{
    assert(m_ctx.hlasm_ctx->in_opencode());

    auto& opencode_copy_stack = m_ctx.hlasm_ctx->opencode_copy_stack();
    while (!opencode_copy_stack.empty())
    {
        auto& copy_file = opencode_copy_stack.back();
        if (!copy_file.suspended())
            return extract_next_logical_line_result::failed; // copy processing resumed
        const auto line = copy_file.suspended_at;

        context::statement_id resync;
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
            ++resync.value;
        }
        copy_file.current_statement = resync;

        const auto* copy_text = m_ctx.lsp_ctx->get_file_info(copy_file.definition_location()->resource_loc);
        if (std::string_view remaining_text = copy_text->data.get_lines_beginning_at({ line, 0 });
            !lexing::extract_logical_line(m_current_logical_line,
                utils::utf8_iterator<std::string_view::iterator, utils::utf8_utf16_counter>(remaining_text.begin()),
                remaining_text.end(),
                lexing::default_ictl_copy))
        {
            opencode_copy_stack.pop_back();
            continue;
        }

        m_current_logical_line_source.begin_line = line;
        m_current_logical_line_source.first_index = m_next_line_index;
        m_current_logical_line_source.last_index = m_next_line_index;
        m_current_logical_line_source.source = logical_line_origin::source_type::copy;

        copy_file.resume();

        return extract_next_logical_line_result::normal; // unaligned statement extracted
    }
    return extract_next_logical_line_result::failed; // next round
}

extract_next_logical_line_result opencode_provider::extract_next_logical_line()
{
    bool ictl_allowed = false;
    if (m_opts.ictl_allowed)
    {
        m_opts.ictl_allowed = false;
        ictl_allowed = true;
    }

    m_current_logical_line.clear();
    m_current_logical_line_source = {};

    if (m_ctx.hlasm_ctx->in_opencode())
    {
        if (!m_ainsert_buffer.empty())
        {
            m_state_listener->schedule_helper_task(convert_ainsert_buffer_to_copybook());
            return extract_next_logical_line_result::failed;
        }

        if (!m_ctx.hlasm_ctx->opencode_copy_stack().empty())
            return extract_next_logical_line_from_copy_buffer();
    }

    if (m_next_line_index >= m_input_document.size())
        return extract_next_logical_line_result::failed;

    if (ictl_allowed)
        ictl_allowed = is_next_line_ictl();

    if (!ictl_allowed && m_opts.process_remaining)
    {
        if (!is_next_line_process())
            m_opts.process_remaining = 0;
        else
        {
            const auto first_index = m_next_line_index;
            const auto& current_line = m_input_document.at(m_next_line_index++);
            const auto current_line_text = current_line.text();
            append_to_logical_line(m_current_logical_line,
                utils::utf8_iterator<std::string_view::iterator, utils::utf8_utf16_counter>(current_line_text.begin()),
                current_line_text.end(),
                lexing::default_ictl);
            finish_logical_line(m_current_logical_line, lexing::default_ictl);
            --m_opts.process_remaining;

            m_current_logical_line_source.begin_line = current_line.lineno().value();
            m_current_logical_line_source.first_index = first_index;
            m_current_logical_line_source.last_index = m_next_line_index;
            m_current_logical_line_source.source = logical_line_origin::source_type::file;

            return extract_next_logical_line_result::process;
        }
    }

    if (should_run_preprocessor())
    {
        if (auto t = run_preprocessor(); t.valid())
            m_state_listener->schedule_helper_task(std::move(t));
        return extract_next_logical_line_result::failed;
    }

    const auto first_index = m_next_line_index;
    const auto current_lineno = m_input_document.at(m_next_line_index).lineno().value();
    while (m_next_line_index < m_input_document.size())
    {
        if (const auto current_line_text = m_input_document.at(m_next_line_index++).text();
            !append_to_logical_line(m_current_logical_line,
                utils::utf8_iterator<std::string_view::iterator, utils::utf8_utf16_counter>(current_line_text.begin()),
                current_line_text.end(),
                lexing::default_ictl))
            break;
    }
    finish_logical_line(m_current_logical_line, lexing::default_ictl);

    if (m_current_logical_line.segments.empty())
        return extract_next_logical_line_result::failed;

    m_current_logical_line_source.begin_line = current_lineno;
    m_current_logical_line_source.first_index = first_index;
    m_current_logical_line_source.last_index = m_next_line_index;
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
    size_t logical_column,
    const processing_status& proc_status,
    bool unlimited_line)
{
    auto& h = is_multiline(text) ? *m_multiline.m_operand_parser : *m_singleline.m_operand_parser;

    h.prepare_parser(
        text, &hlasm_ctx, diags, std::move(range_prov), text_range, logical_column, proc_status, unlimited_line);

    return h;
}

} // namespace hlasm_plugin::parser_library::processing
