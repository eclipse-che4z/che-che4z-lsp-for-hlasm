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

namespace hlasm_plugin::parser_library::processing {

class endevor_preprocessor : public preprocessor
{
    library_fetcher m_libs;
    diagnostic_op_consumer* m_diags = nullptr;
    endevor_preprocessor_options m_options;

public:
    endevor_preprocessor(
        const endevor_preprocessor_options& options, library_fetcher libs, diagnostic_op_consumer* diags)
        : m_libs(std::move(libs))
        , m_diags(diags)
        , m_options(options)
    {}

    // Inherited via preprocessor
    document generate_replacement(document doc) override
    {
        if (std::none_of(doc.begin(), doc.end(), [](const auto& l) {
                auto text = l.text();
                return text.starts_with("-INC ") || text.starts_with("++INCLUDE ");
            }))
            return doc;

        static std::regex include_regex(R"(^(?:-INC|\+\+INCLUDE)\s+(\S+))");

        std::vector<document_line> result;
        result.reserve(doc.size());

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
            result.push_back(line);
            entry.next();

            if (const auto& text = line.text(); !std::regex_search(text.begin(), text.end(), matches, include_regex))
            {
                continue;
            }

            std::string_view member(std::to_address(matches[1].first), matches[1].length());
            if (std::any_of(stack.begin(), stack.end(), [&member](const auto& e) { return e.name == member; }))
            {
                if (m_diags)
                    m_diags->add_diagnostic(diagnostic_op::error_END002(
                        range(position(std::prev(stack.front().current)->lineno().value_or(0), 0)), member));
                break;
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

        return document(std::move(result));
    }
};

std::unique_ptr<preprocessor> preprocessor::create(
    const endevor_preprocessor_options& opts, library_fetcher lf, diagnostic_op_consumer* diags)
{
    return std::make_unique<endevor_preprocessor>(opts, std::move(lf), diags);
}
} // namespace hlasm_plugin::parser_library::processing
