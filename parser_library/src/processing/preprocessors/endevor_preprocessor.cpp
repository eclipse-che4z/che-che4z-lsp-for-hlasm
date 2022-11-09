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
    struct stmt_part
    {
        std::string_view value;
        range r;
    };

    stmt_part instr;
    std::vector<stmt_part> operands;
    std::optional<stmt_part> remark;

    range get_stmt_range() const
    {
        return remark ? range(instr.r.start, remark.value().r.end) : range(instr.r.start, operands.front().r.end);
    }
};

statement_details get_statement_details(std::match_results<std::string_view::iterator>& matches, size_t line_no)
{
    if (matches.size() != 5)
        return {};

    std::string_view inc(std::to_address(matches[1].first), matches[1].length());
    auto inc_range = range(position(line_no, 0), position(line_no, inc.size()));

    std::string_view member(std::to_address(matches[3].first), matches[3].length());
    auto member_start = inc.size() + matches[2].str().length();
    auto member_range = range(position(line_no, member_start), position(line_no, member_start + member.size()));

    std::optional<statement_details::stmt_part> remark = std::nullopt;

    if (matches[4].length())
    {
        statement_details::stmt_part tmp;
        tmp.value = std::string_view(std::to_address(matches[4].first), matches[4].length());
        auto remark_start = member_start + member.size();
        auto remark_end = remark_start + tmp.value.length();
        tmp.r = range(position(line_no, remark_start), position(line_no, remark_end));
        remark = tmp;
    }

    return { { inc, std::move(inc_range) }, { { member, std::move(member_range) } }, std::move(remark) };
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

    bool process_member(std::string_view member, std::vector<stack_entry>& stack)
    {
        if (std::any_of(stack.begin(), stack.end(), [&member](const auto& e) { return e.name == member; }))
        {
            if (m_diags)
                m_diags->add_diagnostic(diagnostic_op::error_END002(
                    range(position(std::prev(stack.front().current)->lineno().value_or(0), 0)), member));
            return false;
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

        return true;
    }

    void do_highlighting(const statement_details& stmt)
    {
        m_src_proc.add_hl_symbol(token_info(stmt.instr.r, semantics::hl_scopes::instruction));

        for (const auto& op : stmt.operands)
        {
            m_src_proc.add_hl_symbol(token_info(op.r, semantics::hl_scopes::operand));
        }

        if (stmt.remark)
        {
            m_src_proc.add_hl_symbol(token_info(stmt.remark.value().r, semantics::hl_scopes::remark));
        }

        m_diags->add_diagnostic(diagnostic_op::fade(stmt.get_stmt_range()));
    }

    void provide_occurrences(const statement_details& stmt)
    {
        lsp::file_occurences_t opencode_occurences;
        lsp::vardef_storage opencode_var_defs;
        lsp::occurence_storage stmt_occurences;
        static std::string empty_text;

        for (const auto& op : stmt.operands)
        {
            stmt_occurences.emplace_back(
                lsp::occurence_kind::INSTR, m_ctx.hlasm_ctx->ids().add(std::string(stmt.instr.value)), stmt.instr.r);
            stmt_occurences.emplace_back(
                lsp::occurence_kind::COPY_OP, m_ctx.hlasm_ctx->ids().add(std::string(op.value)), op.r);
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

        static std::regex include_regex(R"(^(-INC|\+\+INCLUDE)(\s+)(\S+)(?:(.*))?)");

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

            if (!process_member(stmt_details.operands.front().value, stack))
                break;

            if (line_no)
            {
                do_highlighting(stmt_details);
                provide_occurrences(stmt_details);

                asm_processor::parse_copy(m_ctx,
                    m_lib_provider,
                    m_ctx.hlasm_ctx->ids().add(std::string(stmt_details.operands.front().value)),
                    stmt_details.operands.front().r,
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
