/*
 * Copyright (c) 2022 Broadcom.
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

#include <memory>
#include <regex>
#include <utility>
#include <vector>

#include "analyzing_context.h"
#include "expressions/mach_expr_term.h"
#include "preprocessor_options.h"
#include "processing/instruction_sets/asm_processor.h"
#include "processing/preprocessor.h"
#include "semantics/collector.h"
#include "semantics/operand_impls.h"
#include "semantics/source_info_processor.h"
#include "semantics/statement.h"

namespace hlasm_plugin::parser_library::processing {

namespace {
struct stack_entry
{
    std::string name;
    document doc;
    document::iterator current;
    stack_entry(std::string name, document doc_)
        : name(std::move(name))
        , doc(std::move(doc_))
        , current(doc.begin())
    {}

    stack_entry(stack_entry&&) = default;
    stack_entry& operator=(stack_entry&&) = default;

    void next() { ++current; }
    bool end() const { return current == doc.end(); }
};

struct statement_details
{
    range instr_range;
    range operand_range;
    range remark_range;
    std::string_view member_name;

    range get_stmt_range() { return range(instr_range.start, operand_range.end); }
};

statement_details get_statement_details(std::match_results<std::string_view::iterator>& matches, size_t line_no)
{
    if (matches.size() != 6)
        return {};

    std::string_view inc(std::to_address(matches[1].first), matches[1].length());
    auto inc_range = range(position(line_no, 0), position(line_no, inc.size()));

    std::string_view member(std::to_address(matches[3].first), matches[3].length());
    auto member_start = inc.size() + matches[2].str().length();
    auto member_range = range(position(line_no, member_start), position(line_no, member_start + member.size()));

    std::string_view remark(std::to_address(matches[5].first), matches[5].length());
    auto remark_start = member_start + member.size() + matches[4].str().length();
    auto remark_end = remark_start + remark.size();
    auto remark_range = range(position(line_no, remark_start), position(line_no, remark_end));

    return { std::move(inc_range), std::move(member_range), std::move(remark_range), member };
}
} // namespace

class endevor_preprocessor : public preprocessor
{
    library_fetcher m_libs;
    diagnostic_op_consumer* m_diags = nullptr;
    endevor_preprocessor_options m_options;
    semantics::source_info_processor& m_src_proc;
    analyzing_context& m_ctx;
    workspaces::parse_lib_provider& m_lib_provider;

    void process_member(std::string_view member, std::vector<stack_entry>& stack)
    {
        if (std::any_of(stack.begin(), stack.end(), [&member](const auto& e) { return e.name == member; }))
        {
            if (m_diags)
                m_diags->add_diagnostic(diagnostic_op::error_END002(
                    range(position(std::prev(stack.front().current)->lineno().value_or(0), 0)), member));
            return;
        }

        auto lib = m_libs(member);
        if (!lib.has_value())
        {
            if (m_diags)
                m_diags->add_diagnostic(diagnostic_op::error_END001(
                    range(position(std::prev(stack.front().current)->lineno().value_or(0), 0)), member));

            // just continue...
        }
        else
        {
            document member_doc(lib.value());
            member_doc.convert_to_replaced();
            stack.emplace_back(std::string(member), std::move(member_doc));
        }
    }

    semantics::statement_si get_statement_si(std::match_results<std::string_view::iterator>& matches, size_t line_no)
    {
        std::string_view inc(std::to_address(matches[1].first), matches[1].length());
        auto inc_range = range(position(line_no, 0), position(line_no, inc.size()));

        std::string_view member(std::to_address(matches[3].first), matches[3].length());
        auto member_start = inc.size() + matches[2].str().length();
        auto member_range = range(position(line_no, member_start), position(line_no, member_start + member.size()));

        std::string_view remark(std::to_address(matches[5].first), matches[5].length());
        auto remark_start = member_start + member.size() + matches[4].str().length();
        auto remark_end = remark_start + remark.size();
        auto remark_range = range(position(line_no, remark_start), position(line_no, remark_end));

        semantics::operand_list operands;
        auto symbol = std::make_unique<expressions::mach_expr_symbol>(
            m_ctx.hlasm_ctx->ids().add(std::string(member)), nullptr, member_range);
        operands.push_back(
            std::make_unique<semantics::expr_assembler_operand>(std::move(symbol), std::string(member), member_range));

        semantics::remarks_si remarks(remark_range, { remark_range });

        return semantics::statement_si(range(position(line_no, 0), position(line_no, remark_end)),
            semantics::label_si(range()),
            semantics::instruction_si(inc_range, m_ctx.hlasm_ctx->ids().add(std::string(inc))),
            semantics::operands_si(member_range, std::move(operands)),
            std::move(remarks),
            {});
    }

    void do_highlighting(const semantics::statement_si& stmt)
    {
        m_src_proc.add_hl_symbol(token_info(stmt.instruction_ref().field_range, semantics::hl_scopes::instruction));

        for (const auto& op : stmt.operands.value)
        {
            m_src_proc.add_hl_symbol(token_info(op->operand_range, semantics::hl_scopes::operand));
        }

        m_src_proc.add_hl_symbol(token_info(stmt.remarks_ref().field_range, semantics::hl_scopes::remark));
    }

    void provide_occurrences(const semantics::statement_si& stmt)
    {
        lsp::file_occurences_t opencode_occurences;
        lsp::vardef_storage opencode_var_defs;
        lsp::occurence_storage stmt_occurences;
        static std::string empty_text;

        for (const auto& op : stmt.operands.value)
        {
            auto sym_expr =
                dynamic_cast<expressions::mach_expr_symbol*>(op->access_asm()->access_expr()->expression.get());

            if (sym_expr)
            {
                stmt_occurences.emplace_back(lsp::occurence_kind::INSTR,
                    std::get<context::id_index>(stmt.instruction_ref().value),
                    stmt.instruction_ref().field_range);
                stmt_occurences.emplace_back(lsp::occurence_kind::COPY_OP, sym_expr->value, op->operand_range);
            }
        }

        auto& file_occs = opencode_occurences[m_ctx.hlasm_ctx->current_statement_location().resource_loc];
        file_occs.insert(
            file_occs.end(), std::move_iterator(stmt_occurences.begin()), std::move_iterator(stmt_occurences.end()));

        m_ctx.lsp_ctx->add_opencode(
            std::make_unique<lsp::opencode_info>(std::move(opencode_var_defs), std::move(opencode_occurences)),
            lsp::text_data_ref_t(empty_text));
    }

public:
    endevor_preprocessor(const endevor_preprocessor_options& options,
        library_fetcher libs,
        diagnostic_op_consumer* diags,
        semantics::source_info_processor& src_proc,
        analyzing_context& ctx,
        workspaces::parse_lib_provider& lib_provider)
        : m_libs(std::move(libs))
        , m_diags(diags)
        , m_options(options)
        , m_src_proc(src_proc)
        , m_ctx(ctx)
        , m_lib_provider(lib_provider)
    {}

    // Inherited via preprocessor
    document generate_replacement(document doc) override
    {
        if (std::none_of(doc.begin(), doc.end(), [](const auto& l) {
                auto text = l.text();
                return text.starts_with("-INC ") || text.starts_with("++INCLUDE ");
            }))
            return doc;

        static std::regex include_regex(R"(^(-INC|\+\+INCLUDE)(\s+)(\S+)(?:(\s+)(\S+))?)");

        std::vector<document_line> result;
        result.reserve(doc.size());

        std::vector<stack_entry> stack;
        stack.emplace_back(std::string(), std::move(doc));

        std::match_results<std::string_view::iterator> matches;

        while (!stack.empty())
        {
            auto& entry = stack.back();
            if (entry.end())
            {
                stack.pop_back();
                continue;
            }
            const auto& line = *entry.current;
            entry.next();

            if (const auto& text = line.text(); !std::regex_search(text.begin(), text.end(), matches, include_regex))
            {
                result.push_back(line);
                continue;
            }

            auto line_no = std::prev(stack.back().current)->lineno();
            auto stmt_details = get_statement_details(matches, line_no.value_or(0));
            auto stmt_si = get_statement_si(matches, line_no.value_or(0));

            process_member(stmt_details.member_name, stack);

            if (line_no)
            {
                do_highlighting(stmt_si);
                provide_occurrences(stmt_si);

                asm_processor::process_copy(m_ctx,
                    m_lib_provider,
                    m_ctx.hlasm_ctx->ids().add(std::string(stmt_details.member_name)),
                    stmt_details.operand_range,
                    stmt_details.get_stmt_range(),
                    nullptr);
            }
        }

        return document(std::move(result));
    }
};

std::unique_ptr<preprocessor> preprocessor::create(const endevor_preprocessor_options& opts,
    library_fetcher lf,
    diagnostic_op_consumer* diags,
    semantics::source_info_processor& src_proc,
    analyzing_context& ctx,
    workspaces::parse_lib_provider& lib_provider)
{
    return std::make_unique<endevor_preprocessor>(opts, std::move(lf), diags, src_proc, ctx, lib_provider);
}
} // namespace hlasm_plugin::parser_library::processing
