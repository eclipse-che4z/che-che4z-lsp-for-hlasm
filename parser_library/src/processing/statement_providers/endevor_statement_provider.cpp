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

#include "endevor_statement_provider.h"

#include <regex>

#include "expressions/mach_expr_term.h"
#include "semantics/operand_impls.h"

namespace hlasm_plugin::parser_library::processing {
// endevor_statement_provider::endevor_statement_provider(std::string text, analyzing_context ctx)
//     : statement_provider(statement_provider_kind::OPEN)
//     , m_text(std::move(text))
//     , m_ctx(ctx)
//     , m_line_no(7)
//     //, m_src_proc(m_ctx.)
//     //, m_opencode_prov(opencode_prov)
//     , m_collector()
//{
//     m_collector.prepare_for_next_statement();
// }

endevor_statement_provider::endevor_statement_provider(
    std::string text, analyzing_context ctx, size_t line_no, semantics::source_info_processor& src_proc)
    : statement_provider(statement_provider_kind::OPEN)
    , m_text(std::move(text))
    , m_ctx(ctx)
    , m_line_no(line_no)
    , m_src_proc(src_proc)
    //, m_opencode_prov(opencode_prov)
    , m_collector()
{
    m_collector.prepare_for_next_statement();
}

context::shared_stmt_ptr endevor_statement_provider::get_next(const statement_processor& processor)
{
    std::match_results<std::string::iterator> matches;
    static std::regex include_regex(R"(^(-INC|\+\+INCLUDE)(\s+)(\S+)((\s+)(\S+))?)");

    if (!std::regex_search(m_text.begin(), m_text.end(), matches, include_regex))
    {
        m_text.clear();
        return nullptr;
    }

    std::string_view inc(std::to_address(matches[1].first), matches[1].length());

    size_t spaces_1 = matches[2].str().length();
    std::string_view member(std::to_address(matches[3].first), matches[3].length());
    auto member_start = inc.size() + spaces_1;
    auto member_range = range(position(m_line_no, member_start), position(m_line_no, member_start + member.size()));

    size_t spaces_2 = matches[5].str().length();
    std::string_view remark(std::to_address(matches[6].first), matches[6].length());
    auto remark_start = member_start + member.size() + spaces_2;
    auto remark_end = remark_start + remark.size();

    m_collector.set_instruction_field(
        m_ctx.hlasm_ctx->ids().add(std::string(inc)), range(position(m_line_no, 0), position(m_line_no, inc.size())));

    auto tmp = std::make_unique<expressions::mach_expr_symbol>(
        m_ctx.hlasm_ctx->ids().add(std::string(member)), nullptr, member_range);
    semantics::operand_list ops;
    ops.emplace_back(
        std::make_unique<semantics::expr_assembler_operand>(std::move(tmp), std::string(member), member_range));

    m_collector.set_operand_remark_field(std::move(ops),
        semantics::remark_list(
            { range(position(m_line_no, remark_start), position(m_line_no, remark_start + remark.size())) }),
        range(position(m_line_no, member_start), position(m_line_no, remark_start + remark.size())));

    m_src_proc.add_hl_symbol(
        token_info(range(position(m_line_no, 0), position(m_line_no, inc.size())), semantics::hl_scopes::instruction));

    m_src_proc.add_hl_symbol(
        token_info(range(position(m_line_no, member_start), position(m_line_no, member_start + member.size())),
            semantics::hl_scopes::operand));


    m_ctx.hlasm_ctx->set_source_position(m_collector.current_instruction().field_range.start);


    m_src_proc.process_hl_symbols(m_collector.extract_hl_symbols());

    m_text.clear();

    return std::make_shared<semantics::endevor_statement>(
        semantics::instruction_si(m_collector.current_instruction().field_range),
        std::move(m_collector.current_operands()),
        m_collector.current_remarks(),
        range(position(m_line_no, 0), position(m_line_no, remark_start + remark.size())),
        std::vector<diagnostic_op>());
}

bool endevor_statement_provider::finished() const { return m_text.empty(); }

} // namespace hlasm_plugin::parser_library::processing