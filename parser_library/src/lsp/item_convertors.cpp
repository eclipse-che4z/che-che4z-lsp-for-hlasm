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

#include "item_convertors.h"

#include <limits>

#include "completion_item.h"
#include "context/ordinary_assembly/symbol.h"
#include "context/sequence_symbol.h"
#include "ebcdic_encoding.h"
#include "file_info.h"
#include "lsp/lsp_context.h"
#include "lsp/macro_info.h"
#include "text_data_view.h"
#include "utils/concat.h"
#include "utils/string_operations.h"

namespace hlasm_plugin::parser_library::lsp {
namespace {

template</*std::integral*/ typename T>
std::string& append_hex_and_dec(std::string& t, T value)
{
    using UT = std::make_unsigned_t<T>;
    char buffer[(std::numeric_limits<UT>::digits + 3) / 4];
    char* p = std::end(buffer);
    auto convert = (UT)value;

    do
    {
        *--p = "0123456789ABCDEF"[convert & 0xf];
        convert >>= 4;
    } while (convert);

    t.append("X'");
    t.append(p, std::end(buffer));
    t.append("'");
    t.push_back(' ');
    t.append("(").append(std::to_string(value)).append(")");
    return t;
}

} // namespace

std::string hover_text(const context::symbol& sym)
{
    if (sym.value().value_kind() == context::symbol_value_kind::UNDEF)
        return "";
    std::string markdown = "";

    if (sym.value().value_kind() == context::symbol_value_kind::ABS)
    {
        append_hex_and_dec(markdown, sym.value().get_abs());
        markdown.append("\n\n---\n\nAbsolute Symbol\n\n---\n\n");
    }
    else if (sym.value().value_kind() == context::symbol_value_kind::RELOC)
    {
        bool first = true;
        const auto& reloc = sym.value().get_reloc();
        for (const auto& [base, d] : reloc.bases())
        {
            if (base.owner->name.empty() || d == 0)
                continue;

            bool was_first = std::exchange(first, false);
            if (d < 0)
                markdown.append(was_first ? "-" : " - ");
            else if (!was_first)
                markdown.append(" + ");

            if (d != 1 && d != -1)
                markdown.append(std::to_string(-(unsigned)d)).append("*");

            if (!base.qualifier.empty())
                markdown.append(base.qualifier.to_string_view()).append(".");
            markdown.append(base.owner->name.to_string_view());
        }
        if (!first)
            markdown.append(" + ");
        append_hex_and_dec(markdown, reloc.offset());
        markdown.append("\n\n---\n\nRelocatable Symbol\n\n---\n\n");
    }

    const auto& attrs = sym.attributes();
    if (attrs.is_defined(context::data_attr_kind::L))
    {
        markdown.append("L: ");
        append_hex_and_dec(markdown, attrs.length());
        markdown.append("  \n");
    }
    if (attrs.is_defined(context::data_attr_kind::I))
        markdown.append("I: " + std::to_string(attrs.integer()) + "  \n");
    if (attrs.is_defined(context::data_attr_kind::S))
        markdown.append("S: " + std::to_string(attrs.scale()) + "  \n");
    if (attrs.is_defined(context::data_attr_kind::T))
        markdown.append("T: " + ebcdic_encoding::to_ascii((unsigned char)attrs.type()) + "  \n");

    return markdown;
}

std::string hover_text(const variable_symbol_definition& sym)
{
    if (sym.macro_param)
        return "MACRO parameter";
    else
    {
        switch (sym.type)
        {
            case context::SET_t_enum::A_TYPE:
                return "SETA variable";
            case context::SET_t_enum::B_TYPE:
                return "SETB variable";
            case context::SET_t_enum::C_TYPE:
                return "SETC variable";
            default:
                return "";
        }
    }
}

size_t constexpr continuation_column = 71;

bool is_continued_line(std::string_view line)
{
    return line.size() > continuation_column && !utils::isblank32(line[continuation_column]);
}

namespace {
bool is_comment(std::string_view line) { return line.substr(0, 1) == "*" || line.substr(0, 2) == ".*"; }
} // namespace


std::string get_macro_signature(const context::macro_definition& m)
{
    std::string result;
    if (!m.get_label_param_name().empty())
        result.append("&").append(m.get_label_param_name().to_string_view()).append(" ");
    result.append(m.id.to_string_view()).append(" ");

    bool first = true;
    const auto& pos_params = m.get_positional_params();
    // First positional parameter is always label, even when empty
    for (size_t i = 1; i < pos_params.size(); ++i)
    {
        if (pos_params[i] == nullptr)
            continue;
        if (!first)
            result.append(",");
        else
            first = false;

        result.append("&").append(pos_params[i]->id.to_string_view());
    }
    for (const auto& param : m.get_keyword_params())
    {
        if (!first)
            result.append(",");
        else
            first = false;
        result.append("&").append(param->id.to_string_view()).append("=").append(param->default_data->get_value());
    }
    return result;
}

std::string get_macro_documentation(const text_data_view& text, size_t definition_line)
{
    // We start at line where the name of the macro is written
    size_t MACRO_line = definition_line - 1;
    // Skip over MACRO statement
    size_t doc_before_begin_line = MACRO_line - 1;
    // Find the beginning line of documentation written in front of macro definition
    while (doc_before_begin_line != (size_t)-1 && is_comment(text.get_line(doc_before_begin_line)))
        --doc_before_begin_line;
    ++doc_before_begin_line;

    std::string_view doc_before = text.get_range_content({ { doc_before_begin_line, 0 }, { MACRO_line, 0 } });

    // Find the end line of macro definition
    size_t macro_def_end_line = definition_line;
    while (macro_def_end_line < text.get_number_of_lines() && is_continued_line(text.get_line(macro_def_end_line)))
        ++macro_def_end_line;
    ++macro_def_end_line;

    std::string_view macro_def = text.get_range_content({ { definition_line, 0 }, { macro_def_end_line, 0 } });

    // Find the end line of documentation that comes after the macro definition
    size_t doc_after_end_line = macro_def_end_line;

    while (doc_after_end_line < text.get_number_of_lines() && is_comment(text.get_line(doc_after_end_line)))
        ++doc_after_end_line;

    std::string_view doc_after = text.get_range_content({ { macro_def_end_line, 0 }, { doc_after_end_line, 0 } });

    return utils::concat("```\n", macro_def, doc_before, doc_after, "\n```\n");
}

completion_item_s generate_completion_item(const context::sequence_symbol& sym)
{
    std::string label = "." + sym.name.to_string();
    return completion_item_s(label, "Sequence symbol", label, "", completion_item_kind::seq_sym);
}

completion_item_s generate_completion_item(const variable_symbol_definition& vardef)
{
    return completion_item_s("&" + vardef.name.to_string(),
        hover_text(vardef),
        "&" + vardef.name.to_string(),
        "",
        completion_item_kind::var_sym);
}

completion_item_s generate_completion_item(const macro_info& sym, const file_info* info)
{
    const context::macro_definition& m = *sym.macro_definition;

    return completion_item_s(m.id.to_string(),
        get_macro_signature(m),
        m.id.to_string(),
        info ? get_macro_documentation(info->data, sym.definition_location.pos.line) : "",
        completion_item_kind::macro);
}


completion_list_s generate_completion(const completion_list_source& cls,
    const std::function<std::vector<std::string>(std::string_view)>& instruction_suggestions)
{
    return std::visit(
        [&instruction_suggestions](auto v) { return generate_completion(v, instruction_suggestions); }, cls);
}

completion_list_s generate_completion(std::monostate, const std::function<std::vector<std::string>(std::string_view)>&)
{
    return completion_list_s();
}

completion_list_s generate_completion(
    const vardef_storage* var_defs, const std::function<std::vector<std::string>(std::string_view)>&)
{
    completion_list_s items;
    for (const auto& vardef : *var_defs)
    {
        items.emplace_back(generate_completion_item(vardef));
    }

    return items;
}

completion_list_s generate_completion(
    const context::label_storage* seq_syms, const std::function<std::vector<std::string>(std::string_view)>&)
{
    completion_list_s items;
    items.reserve(seq_syms->size());
    for (const auto& [_, sym] : *seq_syms)
    {
        items.emplace_back(generate_completion_item(*sym));
    }
    return items;
}

completion_list_s generate_completion(const completion_list_instructions& cli,
    const std::function<std::vector<std::string>(std::string_view)>& instruction_suggestions)
{
    assert(cli.lsp_ctx);

    const auto& hlasm_ctx = cli.lsp_ctx->get_related_hlasm_context();

    auto suggestions = [&instruction_suggestions](std::string_view ct) {
        std::vector<std::pair<std::string, bool>> result;
        if (ct.empty() || !instruction_suggestions)
            return result;
        auto raw_suggestions = instruction_suggestions(ct);
        result.reserve(raw_suggestions.size());
        for (auto&& s : raw_suggestions)
            result.emplace_back(std::move(s), false);
        return result;
    }(cli.completed_text);
    const auto locate_suggestion = [&s = suggestions](std::string_view text) {
        auto it = std::find_if(s.begin(), s.end(), [text](const auto& e) { return e.first == text; });
        return it == s.end() ? nullptr : std::to_address(it);
    };

    completion_list_s result;

    // Store only instructions from the currently active instruction set
    for (const auto& instr : completion_item_s::m_instruction_completion_items)
    {
        auto id = hlasm_ctx.ids().find(instr.label);
        // TODO: we could provide more precise results here if actual generation is provided
        if (id.has_value() && hlasm_ctx.find_opcode_mnemo(id.value(), context::opcode_generation::zero))
        {
            auto& i = result.emplace_back(instr);
            if (auto space = i.insert_text.find(' '); space != std::string::npos)
            {
                if (auto col_pos = cli.completed_text_start_column + space; col_pos < 15)
                    i.insert_text.insert(i.insert_text.begin() + space, 15 - col_pos, ' ');
            }
            if (auto* suggestion = locate_suggestion(i.label))
            {
                i.suggestion_for = cli.completed_text;
                suggestion->second = true;
            }
        }
    }

    for (const auto& [_, macro_i] : *cli.macros)
    {
        auto& i = result.emplace_back(
            generate_completion_item(*macro_i, cli.lsp_ctx->get_file_info(macro_i->definition_location.resource_loc)));
        if (auto* suggestion = locate_suggestion(i.label))
        {
            i.suggestion_for = cli.completed_text;
            suggestion->second = true;
        }
    }

    for (const auto& [suggestion, used] : suggestions)
    {
        if (used)
            continue;
        result.emplace_back(
            suggestion, "", suggestion, "", completion_item_kind::macro, false, std::string(cli.completed_text));
    }

    return result;
}


} // namespace hlasm_plugin::parser_library::lsp
