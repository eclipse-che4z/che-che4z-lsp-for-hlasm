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

    void next() { ++current; }
    bool end() const { return current == doc.end(); }
};

std::string_view get_copy_member(const std::match_results<std::string_view::iterator>& matches)
{
    if (matches.size() != 4)
        return "";

    return std::string_view(std::to_address(matches[2].first), matches[2].length());
}

std::pair<std::string_view, range> get_stmt_part_pair(
    const std::match_results<std::string_view::iterator>& matches, size_t index, size_t line_no)
{
    std::string_view name(std::to_address(matches[index].first), matches[index].length());
    auto r = range(position(line_no, std::distance(matches[0].first, matches[index].first)),
        position(line_no, std::distance(matches[0].first, matches[index].second)));

    return { name, std::move(r) };
}

std::shared_ptr<semantics::endevor_statement_si> get_preproc_statement(
    const std::match_results<std::string_view::iterator>& matches, size_t line_no, context::id_storage& ids)
{
    if (matches.size() != 4)
        return nullptr;

    auto stmt_r = range({ line_no, 0 }, { line_no, matches[0].str().length() });

    auto inc_range = get_stmt_part_pair(matches, 1, line_no).second;
    auto [member, member_range] = get_stmt_part_pair(matches, 2, line_no);

    auto remarks_r = range();
    std::vector<range> rems;
    if (matches[3].length())
    {
        remarks_r = get_stmt_part_pair(matches, 3, line_no).second;
        rems.emplace_back(remarks_r);
    }

    auto remarks_si = semantics::remarks_si(std::move(remarks_r), std::move(rems));

    return std::make_shared<semantics::endevor_statement_si>(
        std::move(stmt_r), std::move(inc_range), member, std::move(member_range), std::move(remarks_si), ids);
}
} // namespace

class endevor_preprocessor final : public preprocessor
{
    library_fetcher m_libs;
    diagnostic_op_consumer* m_diags = nullptr;
    endevor_preprocessor_options m_options;
    semantics::source_info_processor& m_src_proc;
    context::id_storage& m_ids;

    bool process_member(std::string_view member, std::vector<stack_entry>& stack)
    {
        std::string member_upper = context::to_upper_copy(std::string(member));

        if (std::any_of(stack.begin(), stack.end(), [member = std::string_view(member_upper)](const auto& e) {
                return e.name == member;
            }))
        {
            if (m_diags)
                m_diags->add_diagnostic(diagnostic_op::error_END002(
                    range(position(std::prev(stack.front().current)->lineno().value_or(0), 0)), member));
            return false;
        }

        std::optional<std::pair<std::string, utils::resource::resource_location>> library;
        if (m_libs)
            library = m_libs(member_upper);

        if (!library.has_value())
        {
            if (m_diags)
                m_diags->add_diagnostic(diagnostic_op::error_END001(
                    range(position(std::prev(stack.front().current)->lineno().value_or(0), 0)), member));

            // just continue...
        }
        else
        {
            auto& [lib_text, lib_loc] = *library;
            document member_doc(lib_text);
            member_doc.convert_to_replaced();
            stack.emplace_back(member_upper, std::move(member_doc));
            append_included_member(std::make_unique<included_member_details>(
                included_member_details { std::move(member_upper), std::move(lib_text), std::move(lib_loc) }));
        }

        return true;
    }

public:
    endevor_preprocessor(const endevor_preprocessor_options& options,
        library_fetcher libs,
        diagnostic_op_consumer* diags,
        semantics::source_info_processor& src_proc,
        context::id_storage& ids)
        : m_libs(std::move(libs))
        , m_diags(diags)
        , m_options(options)
        , m_src_proc(src_proc)
        , m_ids(ids)
    {}

    // Inherited via preprocessor
    document generate_replacement(document doc) override
    {
        reset();

        if (std::none_of(doc.begin(), doc.end(), [](const auto& l) {
                auto text = l.text();
                return text.starts_with("-INC ") || text.starts_with("++INCLUDE ");
            }))
            return doc;

        static std::regex include_regex(R"(^(-INC|\+\+INCLUDE)\s+(\S+)(?:\s+(.*))?)");

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

            auto copy_member = get_copy_member(matches);
            auto line_no = std::prev(stack.back().current)->lineno();

            if (!process_member(copy_member, stack))
                break;

            if (line_no)
            {
                auto stmt = get_preproc_statement(matches, *line_no, m_ids);
                do_highlighting(*stmt, m_src_proc);
                set_statement(std::move(stmt));
            }
        }

        return document(std::move(result));
    }
};

std::unique_ptr<preprocessor> preprocessor::create(const endevor_preprocessor_options& opts,
    library_fetcher lf,
    diagnostic_op_consumer* diags,
    semantics::source_info_processor& src_proc,
    context::id_storage& ids)
{
    return std::make_unique<endevor_preprocessor>(opts, std::move(lf), diags, src_proc, ids);
}
} // namespace hlasm_plugin::parser_library::processing
