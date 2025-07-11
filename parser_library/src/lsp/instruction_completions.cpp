/*
 * Copyright (c) 2024 Broadcom.
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

#include "instruction_completions.h"

#include <algorithm>
#include <bitset>
#include <cassert>
#include <numeric>
#include <tuple>

#include "instructions/instruction.h"
#include "utils/concat.h"

namespace hlasm_plugin::parser_library::lsp {

namespace {

constexpr instructions::instruction_set_affiliation all_sets {
    instructions::z_arch_affiliation::ZOP, instructions::z_arch_affiliation::LAST, 1, 1, 1, 1, 1
};

struct operand_formatter
{
    std::string result;
    bool first = true;

    void start_operand()
    {
        if (first)
            first = false;
        else
            result.append(",");
    }
    operand_formatter& append(std::string_view s)
    {
        result.append(s);
        return *this;
    }
    operand_formatter& append(size_t count, char ch)
    {
        result.append(count, ch);
        return *this;
    }
    operand_formatter& append(int i)
    {
        result.append(std::to_string(i));
        return *this;
    }
    operand_formatter& append_imm(unsigned short i)
    {
        if (i >= 0x100)
        {
            char buffer[(std::numeric_limits<decltype(i)>::digits + 3) / 4];
            auto* b = std::end(buffer);
            while (i)
            {
                *--b = "0123456789ABCDEF"[i & 0xf];
                i >>= 4;
            }
            result.append("X'").append(b, std::end(buffer)).append("'");
        }
        else if (i & 0x80)
        {
            result.append("X'80'");
            i &= ~0x80;
            if (i)
                result.append("+").append(std::to_string(i));
        }
        else
            result.append(std::to_string(i));

        return *this;
    }

    std::string take() { return std::move(result); }
};


std::string generate_cc_explanation(const instructions::condition_code_explanation& cc)
{
    const auto qual = cc.cc_qualification();
    std::string result =
        qual.empty() ? std::string("\n\nCondition Code: ") : utils::concat("\n\nCondition Code (", qual, "): ");

    const auto cc_translate = [&cc](int v) {
        auto r = cc.tranlate_cc(static_cast<instructions::condition_code>(v));
        if (r.empty())
            r = "\\--"; // otherwise a horizontal line is generated
        return r;
    };

    if (cc.has_single_explanation())
        result.append(cc_translate(0));
    else
    {
        for (int i = 0; i < 4; ++i)
            result.append("\n- ").append(cc_translate(i));
    }

    return result;
}

std::string_view get_privileged_status_text(instructions::privilege_status p)
{
    using enum instructions::privilege_status;
    switch (p)
    {
        case not_privileged:
            return "";
        case privileged:
            return "\n\nPrivileged instruction";
        case conditionally_privileged:
            return "\n\nConditionally privileged instruction";
    }
    assert(false);
}

std::string get_page_text(size_t pageno)
{
    if (pageno == 0)
        return {};
    else
    {
        const auto page_text = std::to_string(pageno);

        return utils::concat("\n\nDetails on [page ",
            page_text,
            "](",
            "https://www.ibm.com/docs/en/module_1678991624569/pdf/SA22-7832-14.pdf#page=",
            page_text,
            " \"Principles of Operations (SA22-7832-14)\")");
    }
}

std::string_view get_implicit_parameters_text(bool has_some)
{
    if (has_some)
        return " (has additional implicit operands)";
    else
        return "";
}

std::string to_string(instructions::parameter p)
{
    using enum instructions::machine_operand_type;
    std::string ret_val = "";
    switch (p.type)
    {
        case MASK:
            return "M";
        case REG:
            return "R";
        case IMM: {
            ret_val = "I";
            break;
        }
        case NONE:
            return "";
        case DISP: {
            ret_val = "D";
            break;
        }
        case DISP_IDX: {
            ret_val = "DX";
            break;
        }
        case BASE:
            return "B";
        case LENGTH: {
            ret_val = "L";
            break;
        }
        case RELOC_IMM: {
            ret_val = "RI";
            break;
        }
        case VEC_REG:
            return "V";
        case IDX_REG:
            return "X";
    }
    ret_val += std::to_string(p.size);
    if (p.is_signed)
        ret_val += "S";
    else
        ret_val += "U";
    return ret_val;
}

std::string to_string(instructions::machine_operand_format f, size_t i)
{
    const auto index = std::to_string(i);
    std::string ret_val = to_string(f.identifier) + index;
    if (!f.first.is_empty() || !f.second.is_empty())
    {
        ret_val.append("(");
        if (!f.first.is_empty()) // only second cannot be empty
            ret_val.append(to_string(f.first)).append(index).append(",");
        ret_val.append(to_string(f.second)).append(index).append(")");
    }
    return ret_val;
}

void process_machine_instruction(const instructions::machine_instruction& machine_instr,
    std::vector<std::pair<completion_item, instructions::instruction_set_affiliation>>& items)
{
    operand_formatter detail; // operands used for hover - e.g. V,D12U(X,B)[,M]
    operand_formatter autocomplete; // operands used for autocomplete - e.g. V,D12U(X,B) [,M]

    int snippet_id = 1;
    bool first_optional = true;
    for (size_t i = 0; const auto& op : machine_instr.operands())
    {
        if (op.optional)
        {
            if (first_optional)
            {
                first_optional = false;
                autocomplete.append("${").append(snippet_id++).append(": ");
            }
            detail.append("[");
            autocomplete.append("[");
        }
        autocomplete.start_operand();
        detail.start_operand();

        if (!op.optional)
            autocomplete.append("${").append(snippet_id++).append(":");

        const auto formatted_op = to_string(op, ++i);
        detail.append(formatted_op);
        autocomplete.append(formatted_op);

        if (!op.optional)
            autocomplete.append("}");
    }
    if (auto opt = machine_instr.optional_operand_count())
    {
        detail.append(opt, ']');
        autocomplete.append(opt, ']').append("}");
    }

    auto operands = detail.take();

    items.emplace_back(std::piecewise_construct,
        std::forward_as_tuple(std::string(machine_instr.name()),
            std::move(operands),
            utils::concat(machine_instr.name(), " ${", snippet_id++, ":}", autocomplete.take()),
            utils::concat("**",
                machine_instr.fullname(),
                "**",
                "\n\nMachine instruction, format: ",
                instructions::mach_format_to_string(machine_instr.format()),
                "\n\nOperands: ",
                operands,
                get_implicit_parameters_text(machine_instr.has_parameter_list()),
                get_privileged_status_text(machine_instr.privileged()),
                generate_cc_explanation(machine_instr.cc_explanation()),
                get_page_text(machine_instr.page_in_pop())),
            completion_item_kind::mach_instr,
            true),
        std::forward_as_tuple(machine_instr.instr_set_affiliation()));
}

void process_assembler_instruction(const instructions::assembler_instruction& asm_instr,
    std::vector<std::pair<completion_item, instructions::instruction_set_affiliation>>& items)
{
    items.emplace_back(std::piecewise_construct,
        std::forward_as_tuple(std::string(asm_instr.name()),
            utils::concat(asm_instr.name(), "   ", asm_instr.description()),
            std::string(asm_instr.name()) + "   " /*+ description*/,
            "Assembler instruction",
            completion_item_kind::asm_instr),
        std::forward_as_tuple(all_sets));
}

std::array<unsigned char, instructions::machine_instruction::max_operand_count> compute_corrected_ids(
    std::span<const instructions::mnemonic_transformation> ts)
{
    std::array<unsigned char, instructions::machine_instruction::max_operand_count> r;
    std::iota(r.begin(), r.end(), (unsigned char)0);

    unsigned char correction = 0;
    auto it = r.begin();
    for (const auto& t : ts)
    {
        for (auto cnt = t.skip; cnt; --cnt)
            *it++ += correction;
        correction += t.insert;
    }
    std::for_each(it, r.end(), [correction](auto& x) { x += correction; });

    return r;
}

void process_mnemonic_code(const instructions::mnemonic_code& mnemonic_instr,
    std::vector<std::pair<completion_item, instructions::instruction_set_affiliation>>& items)
{
    operand_formatter subs_ops_mnems;
    operand_formatter subs_ops_nomnems;
    operand_formatter subs_ops_nomnems_no_snippets;

    // get mnemonic operands
    int snippet_id = 1;
    bool first_optional = true;

    const auto& mach_operands = mnemonic_instr.instruction().operands();
    const auto optional_count = mnemonic_instr.instruction().optional_operand_count();

    auto transforms = mnemonic_instr.operand_transformations();

    std::bitset<instructions::machine_instruction::max_operand_count> ops_used_by_replacement;
    const auto ids = compute_corrected_ids(transforms);

    for (const auto& r : transforms)
        if (r.has_source())
            ops_used_by_replacement.set(r.source);

    size_t brackets = 0;
    for (size_t i = 0, processed = 0; i < mach_operands.size(); i++)
    {
        if (!transforms.empty())
        {
            auto replacement = transforms.front();
            // can still replace mnemonics
            if (replacement.skip == processed)
            {
                transforms = transforms.subspan(1);
                processed = 0;
                // replace current for mnemonic
                subs_ops_mnems.start_operand();
                using enum instructions::mnemonic_transformation_kind;
                switch (replacement.type)
                {
                    case value:
                        subs_ops_mnems.append_imm(replacement.value);
                        break;
                    case copy:
                        break;
                    case or_with:
                        subs_ops_mnems.append_imm(replacement.value).append("|");
                        break;
                    case add_to:
                        subs_ops_mnems.append_imm(replacement.value).append("+");
                        break;
                    case subtract_from:
                        subs_ops_mnems.append_imm(replacement.value).append("-");
                        break;
                    case complement:
                        subs_ops_mnems.append("-");
                        break;
                }
                if (replacement.has_source())
                {
                    const auto op_string = to_string(mach_operands[replacement.source], 1 + ids[replacement.source]);
                    subs_ops_mnems.append(op_string);
                    if (ops_used_by_replacement.test(replacement.source))
                    {
                        ops_used_by_replacement.reset(replacement.source);

                        subs_ops_nomnems.start_operand();
                        subs_ops_nomnems_no_snippets.start_operand();

                        subs_ops_nomnems.append("${").append(snippet_id++).append(":").append(op_string).append("}");
                        subs_ops_nomnems_no_snippets.append(op_string);
                    }
                }

                continue;
            }
        }
        ++processed;
        ops_used_by_replacement.reset(i);

        const bool is_optional = mach_operands.size() - i <= optional_count;
        if (is_optional && first_optional)
        {
            first_optional = false;
            subs_ops_mnems.append("[");
            subs_ops_nomnems.append("${").append(snippet_id++).append(": [");
            subs_ops_nomnems_no_snippets.append("[");
            ++brackets;
        }

        subs_ops_mnems.start_operand();
        subs_ops_nomnems.start_operand();
        subs_ops_nomnems_no_snippets.start_operand();

        const auto op_string = to_string(mach_operands[i], i + 1);
        if (!is_optional)
        {
            subs_ops_mnems.append(op_string);
            subs_ops_nomnems.append("${").append(snippet_id++).append(":").append(op_string).append("}");
            subs_ops_nomnems_no_snippets.append(op_string);
        }
        else if (mach_operands.size() - i > 1)
        {
            subs_ops_mnems.append(op_string).append("[");
            subs_ops_nomnems.append(op_string).append("[");
            subs_ops_nomnems_no_snippets.append(op_string).append("[");
            ++brackets;
        }
        else
        {
            subs_ops_mnems.append(op_string);
            subs_ops_nomnems.append(op_string);
            subs_ops_nomnems_no_snippets.append(op_string);
        }
    }
    if (brackets)
    {
        subs_ops_mnems.append(brackets, ']');
        subs_ops_nomnems.append(brackets, ']').append("}");
        subs_ops_nomnems_no_snippets.append(brackets, ']');
    }
    items.emplace_back(std::piecewise_construct,
        std::forward_as_tuple(std::string(mnemonic_instr.name()),
            subs_ops_nomnems_no_snippets.take(),
            utils::concat(mnemonic_instr.name(), " ${", snippet_id++, ":}", subs_ops_nomnems.take()),
            utils::concat("**",
                mnemonic_instr.instruction().fullname(),
                "**",
                "\n\nMnemonic code for ",
                mnemonic_instr.instruction().name(),
                " instruction, format: ",
                instructions::mach_format_to_string(mnemonic_instr.instruction().format()),
                "\n\nSubstituted operands: ",
                subs_ops_mnems.take(),
                get_implicit_parameters_text(mnemonic_instr.instruction().has_parameter_list()),
                get_privileged_status_text(mnemonic_instr.instruction().privileged()),
                generate_cc_explanation(mnemonic_instr.instruction().cc_explanation()),
                get_page_text(mnemonic_instr.instruction().page_in_pop())),
            completion_item_kind::mach_instr,
            true),
        std::forward_as_tuple(mnemonic_instr.instr_set_affiliation()));
}

void process_ca_instruction(const instructions::ca_instruction& ca_instr,
    std::vector<std::pair<completion_item, instructions::instruction_set_affiliation>>& items)
{
    items.emplace_back(std::piecewise_construct,
        std::forward_as_tuple(std::string(ca_instr.name()),
            "",
            std::string(ca_instr.name()),
            "Conditional Assembly",
            completion_item_kind::ca_instr),
        std::forward_as_tuple(all_sets));
}

using instruction_completion_items_t =
    std::vector<std::pair<completion_item, instructions::instruction_set_affiliation>>;

} // namespace

const instruction_completion_items_t instruction_completion_items = []() {
    std::vector<std::pair<completion_item, instructions::instruction_set_affiliation>> result;

    result.reserve(instructions::get_instruction_sizes().total());

    for (const auto& instr : instructions::all_ca_instructions())
    {
        process_ca_instruction(instr, result);
    }

    for (const auto& instr : instructions::all_assembler_instructions())
    {
        process_assembler_instruction(instr, result);
    }

    for (const auto& instr : instructions::all_machine_instructions())
    {
        process_machine_instruction(instr, result);
    }

    for (const auto& instr : instructions::all_mnemonic_codes())
    {
        process_mnemonic_code(instr, result);
    }

    std::ranges::sort(result, {}, [](const auto& e) -> decltype(auto) { return e.first.label; });

    return result;
}();

} // namespace hlasm_plugin::parser_library::lsp
