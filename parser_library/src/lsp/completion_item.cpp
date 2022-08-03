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

#include "completion_item.h"

#include <sstream>

#include "context/instruction.h"

namespace hlasm_plugin::parser_library::lsp {

completion_item_s::completion_item_s(std::string label,
    std::string detail,
    std::string insert_text,
    std::string documentation,
    completion_item_kind kind,
    bool snippet)
    : label(std::move(label))
    , detail(std::move(detail))
    , insert_text(std::move(insert_text))
    , documentation(std::move(documentation))
    , kind(kind)
    , snippet(snippet)
{}

namespace {
void process_machine_instruction(const context::machine_instruction& machine_instr,
    std::set<completion_item_s, completion_item_s::label_comparer>& items)
{
    std::stringstream doc_ss("");
    std::stringstream detail_ss(""); // operands used for hover - e.g. V,D12U(X,B)[,M]
    std::stringstream autocomplete(""); // operands used for autocomplete - e.g. V,D12U(X,B) [,M]

    size_t snippet_id = 1;
    bool first_optional = true;
    for (size_t i = 0; i < machine_instr.operands().size(); i++)
    {
        const auto& op = machine_instr.operands()[i];
        const bool is_optional = machine_instr.operands().size() - i <= machine_instr.optional_operand_count();
        if (is_optional && first_optional)
        {
            first_optional = false;
            autocomplete << "${" << snippet_id++ << ": [";
            detail_ss << "[";
        }
        if (i != 0)
        {
            autocomplete << ",";
            detail_ss << ",";
        }
        if (!is_optional)
        {
            detail_ss << op.to_string();
            autocomplete << "${" << snippet_id++ << ":" << op.to_string() << "}";
        }
        else if (machine_instr.operands().size() - i > 1)
        {
            detail_ss << op.to_string() << "[";
            autocomplete << op.to_string() << "[";
        }
        else
        {
            detail_ss << op.to_string() << std::string(machine_instr.optional_operand_count(), ']');
            autocomplete << op.to_string() << std::string(machine_instr.optional_operand_count(), ']') << "}";
        }
    }
    doc_ss << "Machine instruction "
           << "\n\n"
           << "Instruction format: " << context::instruction::mach_format_to_string(machine_instr.format());
    items.emplace(std::string(machine_instr.name()),
        "Operands: " + detail_ss.str(),
        std::string(machine_instr.name()) + " ${" + std::to_string(snippet_id++) + ":}" + autocomplete.str(),
        doc_ss.str(),
        completion_item_kind::mach_instr,
        true);
}

void process_assembler_instruction(const context::assembler_instruction& asm_instr,
    std::set<completion_item_s, completion_item_s::label_comparer>& items)
{
    std::stringstream doc_ss(" ");
    std::stringstream detail_ss("");

    detail_ss << asm_instr.name() << "   " << asm_instr.description();
    doc_ss << "Assembler instruction";
    items.emplace(std::string(asm_instr.name()),
        detail_ss.str(),
        std::string(asm_instr.name()) + "   " /*+ description*/,
        doc_ss.str(),
        completion_item_kind::asm_instr);
}

void process_mnemonic_code(
    const context::mnemonic_code& mnemonic_instr, std::set<completion_item_s, completion_item_s::label_comparer>& items)
{
    std::stringstream doc_ss("");
    std::stringstream detail_ss("");
    std::stringstream subs_ops_mnems("");
    std::stringstream subs_ops_nomnems("");
    std::stringstream subs_ops_nomnems_no_snippets("");

    // get mnemonic operands
    size_t iter_over_mnem = 0;
    size_t snippet_id = 1;
    bool first_optional = true;

    const auto& mach_operands = mnemonic_instr.instruction()->operands();
    const auto optional_count = mnemonic_instr.instruction()->optional_operand_count();
    bool first = true;
    constexpr const auto is_mnemonic_with_operand_ommited = [](std::string_view name) {
        static constexpr const std::string_view list[] = { "VNOT", "NOTR", "NOTGR" };
        return std::find(std::begin(list), std::end(list), name) != std::end(list);
    };


    auto replaces = mnemonic_instr.replaced_operands();

    for (size_t i = 0; i < mach_operands.size(); i++)
    {
        if (replaces.size() > iter_over_mnem)
        {
            auto [position, value] = replaces[iter_over_mnem];
            // can still replace mnemonics
            if (position == i)
            {
                // mnemonics can be substituted when optional_count is 1, but not 2 -> 2 not implemented
                if (optional_count == 1 && mach_operands.size() - i == 1)
                {
                    subs_ops_mnems << "[";
                    if (i != 0)
                        subs_ops_mnems << ",";
                    subs_ops_mnems << std::to_string(value) + "]";
                    continue;
                }
                // replace current for mnemonic
                if (i != 0)
                    subs_ops_mnems << ",";
                if (is_mnemonic_with_operand_ommited(mnemonic_instr.name()))
                {
                    subs_ops_mnems << mach_operands[i - 1].to_string();
                }
                else
                    subs_ops_mnems << std::to_string(value);
                iter_over_mnem++;
                continue;
            }
        }
        const bool is_optional = mach_operands.size() - i <= optional_count;
        if (is_optional && first_optional)
        {
            first_optional = false;
            subs_ops_mnems << " [";
            subs_ops_nomnems << "${" << snippet_id++ << ": [";
            subs_ops_nomnems_no_snippets << " [";
        }
        if (i != 0)
            subs_ops_mnems << ",";
        if (!first)
            subs_ops_nomnems << ",";
        if (!first)
            subs_ops_nomnems_no_snippets << ",";
        if (!is_optional)
        {
            subs_ops_mnems << mach_operands[i].to_string();
            subs_ops_nomnems << "${" << snippet_id++ << ":" << mach_operands[i].to_string() << "}";
            subs_ops_nomnems_no_snippets << mach_operands[i].to_string();
        }
        else if (mach_operands.size() - i > 1)
        {
            subs_ops_mnems << mach_operands[i].to_string() + "[";
            subs_ops_nomnems << mach_operands[i].to_string() << "[";
            subs_ops_nomnems_no_snippets << mach_operands[i].to_string() << "[";
        }
        else
        {
            subs_ops_mnems << mach_operands[i].to_string() + std::string(optional_count, ']');
            subs_ops_nomnems << mach_operands[i].to_string() << std::string(optional_count, ']') << "}";
            subs_ops_nomnems_no_snippets << mach_operands[i].to_string() << std::string(optional_count, ']');
        }
        first = false;
    }
    detail_ss << "Operands: " + subs_ops_nomnems_no_snippets.str();
    doc_ss << "Mnemonic code for " << mnemonic_instr.instruction()->name() << " instruction"
           << "\n\n"
           << "Substituted operands: " << subs_ops_mnems.str() << "\n\n"
           << "Instruction format: "
           << context::instruction::mach_format_to_string(mnemonic_instr.instruction()->format());
    items.emplace(std::string(mnemonic_instr.name()),
        detail_ss.str(),
        std::string(mnemonic_instr.name()) + " ${" + std::to_string(snippet_id++) + ":}" + subs_ops_nomnems.str(),
        doc_ss.str(),
        completion_item_kind::mach_instr,
        true);
}

void process_ca_instruction(
    const context::ca_instruction& ca_instr, std::set<completion_item_s, completion_item_s::label_comparer>& items)
{
    items.emplace(std::string(ca_instr.name()),
        "",
        std::string(ca_instr.name()),
        "Conditional Assembly",
        completion_item_kind::ca_instr);
}

} // namespace

const std::set<completion_item_s, completion_item_s::label_comparer> completion_item_s::m_instruction_completion_items =
    [] {
        std::set<completion_item_s, completion_item_s::label_comparer> result;

        for (const auto& instr : context::instruction::all_ca_instructions())
        {
            process_ca_instruction(instr, result);
        }

        for (const auto& instr : context::instruction::all_assembler_instructions())
        {
            process_assembler_instruction(instr, result);
        }

        for (const auto& instr : context::instruction::all_machine_instructions())
        {
            process_machine_instruction(instr, result);
        }

        for (const auto& instr : context::instruction::all_mnemonic_codes())
        {
            process_mnemonic_code(instr, result);
        }

        return result;
    }();

bool operator==(const completion_item_s& lhs, const completion_item_s& rhs)
{
    return lhs.label == rhs.label && lhs.detail == rhs.detail && lhs.insert_text == rhs.insert_text
        && lhs.documentation == rhs.documentation && lhs.kind == rhs.kind;
}

} // namespace hlasm_plugin::parser_library::lsp