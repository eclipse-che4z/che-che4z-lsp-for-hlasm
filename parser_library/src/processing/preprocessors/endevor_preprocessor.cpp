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

#include <algorithm>
#include <iterator>
#include <memory>
#include <regex>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "diagnostic_consumer.h"
#include "document.h"
#include "preprocessor_options.h"
#include "preprocessor_utils.h"
#include "processing/preprocessor.h"
#include "range.h"
#include "semantics/source_info_processor.h"
#include "utils/resource_location.h"
#include "utils/string_operations.h"
#include "utils/task.h"

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

std::string get_copy_member(const std::match_results<std::string_view::iterator>& matches)
{
    if (matches.size() != 4)
        return "";

    return std::string(matches[2].first, matches[2].second);
}
} // namespace

class endevor_preprocessor final : public preprocessor
{
    library_fetcher m_libs;
    diagnostic_op_consumer* m_diags = nullptr;
    endevor_preprocessor_options m_options;
    semantics::source_info_processor& m_src_proc;

    [[nodiscard]] utils::value_task<bool> process_member(std::string member, std::vector<stack_entry>& stack)
    {
        std::string member_upper = utils::to_upper_copy(member);

        if (std::ranges::find(stack, member_upper, &stack_entry::name) != stack.end())
        {
            if (m_diags)
                m_diags->add_diagnostic(diagnostic_op::error_END002(
                    range(position(std::prev(stack.front().current)->lineno().value_or(0), 0)), member));
            co_return false;
        }

        std::optional<std::pair<std::string, utils::resource::resource_location>> library;
        if (m_libs)
            library = co_await m_libs(member_upper);

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

        co_return true;
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
    [[nodiscard]] utils::value_task<document> generate_replacement(document doc) override
    {
        reset();

        static constexpr const auto include_statement = [](const auto& l) {
            return l.starts_with("-INC ") || l.starts_with("++INCLUDE ");
        };
        if (std::ranges::none_of(doc, include_statement, &document_line::text))
            co_return doc;

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

            auto line_no = std::prev(stack.back().current)->lineno();

            if (!co_await process_member(get_copy_member(matches), stack))
                break;

            if (line_no)
            {
                static const stmt_part_ids part_ids { std::nullopt, { 1 }, 2, 3 };

                auto m = make_preproc_matches<3>(matches);
                auto stmt = get_preproc_statement<semantics::endevor_statement_si>(
                    std::span(m.cbegin(), m.cend()), part_ids, *line_no, true);
                do_highlighting(*stmt, m_src_proc);
                set_statement(std::move(stmt));
            }
        }

        co_return document(std::move(result));
    }
};

std::unique_ptr<preprocessor> preprocessor::create(const endevor_preprocessor_options& opts,
    library_fetcher lf,
    diagnostic_op_consumer* diags,
    semantics::source_info_processor& src_proc)
{
    return std::make_unique<endevor_preprocessor>(opts, std::move(lf), diags, src_proc);
}
} // namespace hlasm_plugin::parser_library::processing
