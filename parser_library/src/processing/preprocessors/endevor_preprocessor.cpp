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

#include <regex>
#include <utility>
#include <vector>

#include "preprocessor_options.h"
#include "processing/preprocessor.h"
#include "semantics/collector.h"
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

semantics::preprocessor_statement get_statement_details(
    std::match_results<std::string_view::iterator>& matches, size_t line_no)
{
    if (matches.size() != 5)
        return {};

    std::string_view inc(std::to_address(matches[1].first), matches[1].length());
    auto inc_range = range(position(line_no, 0), position(line_no, inc.size()));

    std::string_view member(std::to_address(matches[3].first), matches[3].length());
    auto member_start = inc.size() + matches[2].str().length();
    auto member_range = range(position(line_no, member_start), position(line_no, member_start + member.size()));

    std::optional<semantics::preprocessor_statement::stmt_part> remark = std::nullopt;

    if (matches[4].length())
    {
        semantics::preprocessor_statement::stmt_part tmp;
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
    std::vector<semantics::preprocessor_statement> m_statements;

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

    void do_highlighting(const semantics::preprocessor_statement& stmt)
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

public:
    endevor_preprocessor(const endevor_preprocessor_options& options,
        library_fetcher libs,
        diagnostic_op_consumer* diags,
        semantics::source_info_processor& src_proc)
        : m_libs(std::move(libs))
        , m_diags(diags)
        , m_options(options)
        , m_src_proc(src_proc)
    {}

    // Inherited via preprocessor
    document generate_replacement(document doc) override
    {
        m_statements.clear();

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
                m_statements.emplace_back(stmt_details);
            }
        }

        return document(std::move(result));
    }

    void finished() override {}

    const std::vector<semantics::preprocessor_statement>& get_statements() const override { return m_statements; }
};

std::unique_ptr<preprocessor> preprocessor::create(const endevor_preprocessor_options& opts,
    library_fetcher lf,
    diagnostic_op_consumer* diags,
    semantics::source_info_processor& src_proc)
{
    return std::make_unique<endevor_preprocessor>(opts, std::move(lf), diags, src_proc);
}
} // namespace hlasm_plugin::parser_library::processing
