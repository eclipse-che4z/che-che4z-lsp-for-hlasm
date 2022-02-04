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
    completion_item_kind kind)
    : label(std::move(label))
    , detail(std::move(detail))
    , insert_text(std::move(insert_text))
    , documentation(std::move(documentation))
    , kind(kind)
{}

const std::set<completion_item_s, completion_item_s::label_comparer> completion_item_s::instruction_completion_items_ =
    [] {
        using namespace context;

        std::set<completion_item_s, completion_item_s::label_comparer> result;


        for (const auto& machine_instr : instruction::all_machine_instructions())
        {
            std::stringstream doc_ss(" ");
            std::stringstream detail_ss(""); // operands used for hover - e.g. V,D12U(X,B)[,M]
            std::stringstream autocomplete(""); // operands used for autocomplete - e.g. V,D12U(X,B) [,M]

            autocomplete << machine_instr.name() << "   ";

            for (size_t i = 0; i < machine_instr.operands().size(); i++)
            {
                const auto& op = machine_instr.operands()[i];
                if (machine_instr.optional_operand_count() == 1 && machine_instr.operands().size() - i == 1)
                {
                    autocomplete << " [";
                    detail_ss << "[";
                    if (i != 0)
                    {
                        autocomplete << ",";
                        detail_ss << ",";
                    }
                    detail_ss << op.to_string() << "]";
                    autocomplete << op.to_string() << "]";
                }
                else if (machine_instr.optional_operand_count() == 2 && machine_instr.operands().size() - i == 2)
                {
                    autocomplete << " [";
                    detail_ss << "[";
                    if (i != 0)
                    {
                        autocomplete << ",";
                        detail_ss << ",";
                    }
                    detail_ss << op.to_string() << "]";
                    autocomplete << op.to_string() << "[,";
                }
                else if (machine_instr.optional_operand_count() == 2 && machine_instr.operands().size() - i == 1)
                {
                    detail_ss << op.to_string() << "]]";
                    autocomplete << op.to_string() << "]]";
                }
                else
                {
                    if (i != 0)
                    {
                        autocomplete << ",";
                        detail_ss << ",";
                    }
                    detail_ss << op.to_string();
                    autocomplete << op.to_string();
                }
            }
            doc_ss << "Machine instruction " << std::endl
                   << "Instruction format: " << instruction::mach_format_to_string(machine_instr.format());
            result.emplace(std::string(machine_instr.name()),
                "Operands: " + detail_ss.str(),
                autocomplete.str(),
                doc_ss.str(),
                completion_item_kind::mach_instr);
        }

        for (const auto& asm_instr : instruction::all_assembler_instructions())
        {
            std::stringstream doc_ss(" ");
            std::stringstream detail_ss("");

            // int min_op = asm_instr.second.min_operands;
            // int max_op = asm_instr.second.max_operands;

            detail_ss << asm_instr.name() << "   " << asm_instr.description();
            doc_ss << "Assembler instruction";
            result.emplace(std::string(asm_instr.name()),
                detail_ss.str(),
                std::string(asm_instr.name()) + "   " /*+ description*/,
                doc_ss.str(),
                completion_item_kind::asm_instr);
        }

        for (const auto& mnemonic_instr : instruction::all_mnemonic_codes())
        {
            std::stringstream doc_ss(" ");
            std::stringstream detail_ss("");
            std::stringstream subs_ops_mnems(" ");
            std::stringstream subs_ops_nomnems(" ");

            // get mnemonic operands
            size_t iter_over_mnem = 0;

            const auto& mach_operands = mnemonic_instr.instruction()->operands();
            auto no_optional = mnemonic_instr.instruction()->optional_operand_count();
            bool first = true;
            std::vector<std::string> mnemonic_with_operand_ommited = { "VNOT", "NOTR", "NOTGR" };


            auto replaces = mnemonic_instr.replaced_operands();

            for (size_t i = 0; i < mach_operands.size(); i++)
            {
                if (replaces.size() > iter_over_mnem)
                {
                    auto [position, value] = replaces[iter_over_mnem];
                    // can still replace mnemonics
                    if (position == i)
                    {
                        // mnemonics can be substituted when no_optional is 1, but not 2 -> 2 not implemented
                        if (no_optional == 1 && mach_operands.size() - i == 1)
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
                        if (std::find(mnemonic_with_operand_ommited.begin(),
                                mnemonic_with_operand_ommited.end(),
                                mnemonic_instr.name())
                            != mnemonic_with_operand_ommited.end())
                        {
                            subs_ops_mnems << mach_operands[i - 1].to_string();
                        }
                        else
                            subs_ops_mnems << std::to_string(value);
                        iter_over_mnem++;
                        continue;
                    }
                }
                // do not replace by a mnemonic
                std::string curr_op_with_mnem = "";
                std::string curr_op_without_mnem = "";
                if (no_optional == 0)
                {
                    if (i != 0)
                        curr_op_with_mnem += ",";
                    if (!first)
                        curr_op_without_mnem += ",";
                    curr_op_with_mnem += mach_operands[i].to_string();
                    curr_op_without_mnem += mach_operands[i].to_string();
                }
                else if (no_optional == 1 && mach_operands.size() - i == 1)
                {
                    curr_op_with_mnem += "[";
                    curr_op_without_mnem += "[";
                    if (i != 0)
                        curr_op_with_mnem += ",";
                    if (!first)
                        curr_op_without_mnem += ",";
                    curr_op_with_mnem += mach_operands[i].to_string() + "]";
                    curr_op_without_mnem += mach_operands[i].to_string() + "]";
                }
                else if (no_optional == 2 && mach_operands.size() - i == 1)
                {
                    curr_op_with_mnem += mach_operands[i].to_string() + "]]";
                    curr_op_without_mnem += mach_operands[i].to_string() + "]]";
                }
                else if (no_optional == 2 && mach_operands.size() - i == 2)
                {
                    curr_op_with_mnem += "[";
                    curr_op_without_mnem += "[";
                    if (i != 0)
                        curr_op_with_mnem += ",";
                    if (!first)
                        curr_op_without_mnem += ",";
                    curr_op_with_mnem += mach_operands[i].to_string() + "[,";
                    curr_op_without_mnem += mach_operands[i].to_string() + "[,";
                }
                subs_ops_mnems << curr_op_with_mnem;
                subs_ops_nomnems << curr_op_without_mnem;
                first = false;
            }
            detail_ss << "Operands: " + subs_ops_nomnems.str();
            doc_ss << "Mnemonic code for " << mnemonic_instr.instruction()->name() << " instruction" << std::endl
                   << "Substituted operands: " << subs_ops_mnems.str() << std::endl
                   << "Instruction format: "
                   << instruction::mach_format_to_string(mnemonic_instr.instruction()->format());
            result.emplace(std::string(mnemonic_instr.name()),
                detail_ss.str(),
                std::string(mnemonic_instr.name()) + "   " + subs_ops_nomnems.str(),
                doc_ss.str(),
                completion_item_kind::mach_instr);
        }

        for (const auto& ca_instr : instruction::all_ca_instructions())
        {
            result.emplace(std::string(ca_instr.name()),
                "",
                std::string(ca_instr.name()),
                "Conditional Assembly",
                completion_item_kind::ca_instr);
        }
        return result;
    }();



bool operator==(const completion_item_s& lhs, const completion_item_s& rhs)
{
    return lhs.label == rhs.label && lhs.detail == rhs.detail && lhs.insert_text == rhs.insert_text
        && lhs.documentation == rhs.documentation && lhs.kind == rhs.kind;
}


} // namespace hlasm_plugin::parser_library::lsp