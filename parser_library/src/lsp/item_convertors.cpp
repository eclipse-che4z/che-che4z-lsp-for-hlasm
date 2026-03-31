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

#include <concepts>
#include <format>
#include <limits>

#include "completion_item.h"
#include "context/hlasm_context.h"
#include "context/ordinary_assembly/section.h"
#include "context/ordinary_assembly/symbol.h"
#include "context/sequence_symbol.h"
#include "context/using.h"
#include "ebcdic_encoding.h"
#include "file_info.h"
#include "lsp/instruction_completions.h"
#include "lsp/lsp_context.h"
#include "lsp/macro_info.h"
#include "text_data_view.h"
#include "utils/projectors.h"
#include "utils/string_operations.h"
#include "utils/text_convertor.h"
#include "utils/unicode_text.h"

namespace hlasm_plugin::parser_library::lsp {
namespace {

template<std::integral T>
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

struct string_appender
{
    utils::conversion_helper convert;
    std::string& target;

    const string_appender& append(std::string_view s) const
    {
        convert.append_to(target, s);
        return *this;
    }

    const string_appender& append(context::id_index id) const
    {
        convert.append_to(target, id.to_string_view());
        return *this;
    }
};

std::string as_id(context::id_index id, const utils::text_convertor* tc)
{
    return utils::conversion_helper(tc).convert_to(id.to_string_view());
}

std::string as_var(context::id_index id, const utils::text_convertor* tc)
{
    return utils::conversion_helper(tc).convert_to("&", id.to_string_view());
}

std::string as_seq_sym(context::id_index id, const utils::text_convertor* tc)
{
    return utils::conversion_helper(tc).convert_to(".", id.to_string_view());
}


} // namespace

std::string hover_text(const context::symbol& sym, const utils::text_convertor* tc)
{
    if (sym.value().value_kind() == context::symbol_value_kind::UNDEF)
        return "";
    std::string markdown = "";
    const utils::conversion_helper convertor { tc };
    const string_appender md_appender { utils::conversion_helper(tc), markdown };

    if (sym.value().value_kind() == context::symbol_value_kind::ABS)
    {
        append_hex_and_dec(markdown, sym.value().get_abs());
        markdown.append("\n\n---\n\nAbsolute Symbol\n\n---\n\n");
    }
    else if (sym.value().value_kind() == context::symbol_value_kind::RELOC)
    {
        bool first = true;
        const auto& reloc = sym.value().get_reloc();
        auto bases = std::vector<context::address::base_entry>(reloc.bases().begin(), reloc.bases().end());
        std::ranges::sort(bases, {}, [](const auto& e) { return std::tie(e.owner->name, e.qualifier); });
        for (const auto& [qualifier, owner, d] : bases)
        {
            if (owner->name.empty() || d == 0)
                continue;

            bool was_first = std::exchange(first, false);
            if (d < 0)
                markdown.append(was_first ? "-" : " - ");
            else if (!was_first)
                markdown.append(" + ");

            if (d != 1 && d != -1)
                markdown.append(std::to_string(d < 0 ? -(unsigned)d : (unsigned)d)).append("*");

            if (!qualifier.empty())
                md_appender.append(qualifier).append(".");
            md_appender.append(owner->name);
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
        markdown.append("I: ").append(std::to_string(attrs.integer())).append("  \n");
    if (attrs.is_defined(context::data_attr_kind::S))
        markdown.append("S: ").append(std::to_string(attrs.scale())).append("  \n");
    if (attrs.is_defined(context::data_attr_kind::T))
    {
        markdown.append("T: ");
        md_appender.append(ebcdic_encoding::to_ascii((unsigned char)attrs.type()));
        markdown.append("  \n");
    }
    if (const auto p = attrs.prog_type(); p.valid)
    {
        std::string_view val(p.ebcdic_value, sizeof(p.ebcdic_value));
        std::string valstr;
        const auto replaced =
            utils::append_utf8_sanitized(valstr, convertor.convert_to(ebcdic_encoding::to_ascii(std::string(val))));
        markdown.append("P: '");
        for (auto c : valstr)
        {
            if (c == ' ')
                markdown.append((const char*)u8"\U00002003");
            else
                markdown.push_back(c);
        }
        markdown.push_back('\'');
        if (replaced == utils::character_replaced::yes)
        {
            markdown.push_back(' ');
            uint32_t bin = 0;
            for (unsigned char c : val)
                bin = c | bin << 8;
            append_hex_and_dec(markdown, bin);
        }

        markdown.append("  \n");
    }
    if (const auto a = attrs.asm_type(); a != context::symbol_attributes::assembler_type::NONE)
    {
        markdown.append("A: ");
        md_appender.append(context::assembler_type_to_string(a));
        markdown.append("  \n");
    }

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
    if (line.size() <= continuation_column)
        return false;
    auto c = utils::utf8_substr(line, continuation_column).str;
    return !c.empty() && !utils::isblank32(c.front());
}

namespace {
bool is_comment(std::string_view line) { return line.substr(0, 1) == "*" || line.substr(0, 2) == ".*"; }

constexpr std::string_view prolog("```hlasm");
constexpr std::string_view epilog("\n```\n");

void add_line(std::string& str, std::string_view line, const utils::text_convertor* tc)
{
    auto append = utils::utf8_substr(line, 0, 72).str;
    str.push_back('\n');
    size_t trim_pos = append.find_last_not_of(" \n\r") + 1;
    string_appender { utils::conversion_helper(tc), str }.append(append.substr(0, trim_pos - (trim_pos == 0)));
}
} // namespace


std::string get_macro_signature(const context::macro_definition& m, const utils::text_convertor* tc)
{
    std::string result;
    const string_appender appender { utils::conversion_helper(tc), result };
    if (!m.get_label_param_name().empty())
    {
        appender.append("&").append(m.get_label_param_name());
        result.append(" ");
    }
    appender.append(m.id);
    result.append(" ");

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

        appender.append("&").append(pos_params[i]->id);
    }
    for (const auto& param : m.get_keyword_params())
    {
        if (!first)
            result.append(",");
        else
            first = false;
        appender.append("&").append(param->id).append("=").append(param->default_data->get_value());
    }
    return result;
}

std::string get_macro_documentation(const text_data_view& text, size_t definition_line, const utils::text_convertor* tc)
{
    // We start at line where the name of the macro is written
    size_t MACRO_line = definition_line - 1;
    // Skip over MACRO statement
    size_t doc_before_begin_line = MACRO_line - 1;
    // Find the beginning line of documentation written in front of macro definition
    while (doc_before_begin_line != (size_t)-1 && is_comment(text.get_line(doc_before_begin_line)))
        --doc_before_begin_line;
    ++doc_before_begin_line;

    // Find the end line of macro definition
    size_t macro_def_end_line = definition_line;
    while (macro_def_end_line < text.get_number_of_lines() && is_continued_line(text.get_line(macro_def_end_line)))
        ++macro_def_end_line;
    ++macro_def_end_line;

    // Find the end line of documentation that comes after the macro definition
    size_t doc_after_end_line = macro_def_end_line;

    while (doc_after_end_line < text.get_number_of_lines() && is_comment(text.get_line(doc_after_end_line)))
        ++doc_after_end_line;

    // There is a limit editor.maxTokenizationLineLength which seems to be applied a bit strangely...
    // Breaking the content into two blocks ensures that at least the first one is likely highlighted correctly

    std::string result;
    /* 2x(prolog + epilog) + line count * (72 columns, newline, reserve 1 byte per line for weird chars) */
    result.reserve(2 * (prolog.size() + epilog.size()) + (72 + 1 + 1) * (doc_after_end_line - doc_before_begin_line));

    result.append(prolog);
    for (auto i = definition_line; i < macro_def_end_line; ++i)
        add_line(result, text.get_line(i), tc);
    result.append(epilog);

    if (MACRO_line - doc_before_begin_line + doc_after_end_line - macro_def_end_line > 0)
    {
        size_t doc_lines = 0;
        constexpr size_t doc_limit = 1024;

        result.append(prolog);
        for (auto i = doc_before_begin_line; i < MACRO_line && doc_lines < doc_limit; ++i, ++doc_lines)
            add_line(result, text.get_line(i), tc);
        for (auto i = macro_def_end_line; i < doc_after_end_line && doc_lines < doc_limit; ++i, ++doc_lines)
            add_line(result, text.get_line(i), tc);
        result.append(epilog);

        if (doc_lines >= doc_limit)
            result.append("Documentation truncated...");
    }

    return result;
}

std::string get_logical_line(const text_data_view& text, size_t definition_line, const utils::text_convertor* tc)
{
    size_t end_line = definition_line;
    for (size_t doc_limit = 20; doc_limit && end_line < text.get_number_of_lines(); --doc_limit)
    {
        if (!is_continued_line(text.get_line(end_line)))
            break;
        ++end_line;
    }
    ++end_line;

    std::string result;
    /* (prolog + epilog) + line count * (72 columns, newline, reserve 1 byte per line for weird chars) */
    result.reserve((prolog.size() + epilog.size()) + (72 + 1 + 1) * (end_line - definition_line));

    result.append(prolog);
    for (auto i = definition_line; i < end_line; ++i)
    {
        add_line(result, text.get_line(i), tc);
    }
    result.append(epilog);

    return result;
}

completion_item generate_completion_item_seq(context::id_index name, const utils::text_convertor* tc)
{
    std::string label = as_seq_sym(name, tc);
    return completion_item(label, "Sequence symbol", label, "", completion_item_kind::seq_sym);
}
completion_item generate_completion_item(
    context::id_index name, const context::opencode_sequence_symbol&, const utils::text_convertor* tc)
{
    return generate_completion_item_seq(name, tc);
}
completion_item generate_completion_item(
    context::id_index name, const context::macro_sequence_symbol&, const utils::text_convertor* tc)
{
    return generate_completion_item_seq(name, tc);
}

completion_item generate_completion_item(const variable_symbol_definition& vardef, const utils::text_convertor* tc)
{
    const auto varname = as_var(vardef.name, tc);
    return completion_item(varname, hover_text(vardef), varname, "", completion_item_kind::var_sym);
}

completion_item generate_completion_item(const macro_info& sym, const file_info* info, const utils::text_convertor* tc)
{
    const context::macro_definition& m = *sym.macro_definition;
    const auto id = as_id(m.id, tc);

    return completion_item(id,
        get_macro_signature(m, tc),
        id,
        info ? get_macro_documentation(info->data, sym.definition_location.pos.line, tc) : "",
        completion_item_kind::macro);
}


std::vector<completion_item> generate_completion(const completion_list_source& cls, const utils::text_convertor* tc)
{
    return std::visit([tc](auto v) { return generate_completion(v, tc); }, cls);
}

std::vector<completion_item> generate_completion(std::monostate, const utils::text_convertor*)
{
    return std::vector<completion_item>();
}

std::vector<completion_item> generate_completion(const vardef_storage* var_defs, const utils::text_convertor* tc)
{
    std::vector<completion_item> items;
    for (const auto& vardef : *var_defs)
    {
        items.emplace_back(generate_completion_item(vardef, tc));
    }

    return items;
}

std::vector<completion_item> generate_completion(
    const std::unordered_map<context::id_index, context::opencode_sequence_symbol>* seq_syms,
    const utils::text_convertor* tc)
{
    std::vector<completion_item> items;
    items.reserve(seq_syms->size());
    for (const auto& [name, sym] : *seq_syms)
    {
        items.emplace_back(generate_completion_item(name, sym, tc));
    }
    return items;
}

std::vector<completion_item> generate_completion(
    const context::macro_label_storage* seq_syms, const utils::text_convertor* tc)
{
    std::vector<completion_item> items;
    items.reserve(seq_syms->size());
    for (const auto& [name, sym] : *seq_syms)
    {
        items.emplace_back(generate_completion_item(name, sym, tc));
    }
    return items;
}

std::vector<completion_item> generate_completion(
    const completion_list_instructions& cli, const utils::text_convertor* tc)
{
    assert(cli.lsp_ctx);

    const utils::conversion_helper convertor { tc };

    const auto& hlasm_ctx = cli.lsp_ctx->get_related_hlasm_context();
    const auto instruction_set = hlasm_ctx.options().instr_set;

    std::vector<std::pair<std::string, bool>> suggestions;
    suggestions.reserve(cli.additional_instructions.size());
    for (const auto& s : cli.additional_instructions)
        suggestions.emplace_back(convertor.convert_to(s), false);

    const auto locate_suggestion = [&s = suggestions](std::string_view text) {
        auto it = std::ranges::find(s, text, utils::first_element);
        return it == s.end() ? nullptr : std::to_address(it);
    };

    std::vector<completion_item> result;
    const auto completed_text = convertor.convert_to(cli.completed_text);

    // Store only instructions from the currently active instruction set
    for (const auto& [instr, aff] : instruction_completion_items)
    {
        if (!instructions::instruction_available(aff, instruction_set))
            continue;
        // Coversion should not be needed
        auto& i = result.emplace_back(instr);
        if (auto space = i.insert_text.find(' '); space != std::string::npos)
        {
            if (auto col_pos = cli.completed_text_start_column + space; col_pos < 15)
                i.insert_text.insert(i.insert_text.begin() + space, 15 - col_pos, ' ');
        }
        if (auto* suggestion = locate_suggestion(i.label);
            suggestion && !suggestion->first.starts_with(cli.completed_text))
        {
            i.suggestion_for = completed_text;
            suggestion->second = true;
        }
    }

    for (const auto& [_, macro_i] : *cli.macros)
    {
        auto& i = result.emplace_back(generate_completion_item(
            *macro_i, cli.lsp_ctx->get_file_info(macro_i->definition_location.resource_loc), tc));
        if (auto* suggestion = locate_suggestion(i.label);
            suggestion && !suggestion->first.starts_with(cli.completed_text))
        {
            i.suggestion_for = completed_text;
            suggestion->second = true;
        }
    }

    for (const auto& [suggestion, used] : suggestions)
    {
        if (used)
            continue;
        const auto cs = convertor.convert_to(suggestion);
        result.emplace_back(cs, "", cs, "", completion_item_kind::macro, false, completed_text);
    }

    return result;
}

std::string_view ordinal_suffix(size_t i)
{
    static constexpr std::string_view suffixes[] { "th", "st", "nd", "rd" };
    return suffixes[i < std::size(suffixes) ? i : 0];
}

std::vector<completion_item> generate_completion(
    std::vector<completion_item>& result, const context::macro_definition* md, const utils::text_convertor* tc)
{
    const utils::conversion_helper convertor { tc };

    for (const auto& positional : md->get_positional_params())
    {
        if (!positional || positional->position == 0 || positional->id.empty()) // label parameter or invalid
            continue;

        const auto var = as_var(positional->id, tc);
        result.emplace_back(var,
            std::format(
                "{} ({}{} positional argument)", var, positional->position, ordinal_suffix(positional->position)),
            "$0", // workaround - vscode does not support empty insertText
            "",
            completion_item_kind::var_sym,
            true);
    }
    for (const auto& keyword : md->get_keyword_params())
    {
        if (keyword->id.empty()) // invalid
            continue;

        const auto var = as_var(keyword->id, tc);
        result.emplace_back(var,
            std::format("{} (keyword argument)", var),
            convertor.convert_to(keyword->id.to_string_view(), "="),
            std::format("```hlasm\n {} {}{}\n```\n",
                as_id(md->id, tc),
                convertor.convert_to("&", keyword->id.to_string_view(), "="),
                convertor.convert_to(keyword->default_data->get_value())),
            completion_item_kind::var_sym);
    }
    return result;
}

std::vector<completion_item> generate_completion(
    const std::pair<const context::macro_definition*,
        std::vector<std::pair<const context::symbol*, context::id_index>>>& args,
    const utils::text_convertor* tc)
{
    const auto& [md, symbols] = args;

    std::vector<completion_item> result;

    if (md)
        generate_completion(result, md, tc);

    for (const auto& [symbol, label] : symbols)
    {
        std::string name;
        const string_appender appender { utils::conversion_helper(tc), name };
        appender.append(label);
        if (!name.empty())
            appender.append(".");
        appender.append(symbol->name());

        result.emplace_back(name,
            name
                + (symbol->value().value_kind() == context::symbol_value_kind::ABS ? " (absolute symbol)"
                                                                                   : " (relocatable symbol)"),
            name,
            hover_text(*symbol, tc),
            completion_item_kind::ord_sym);
    }

    return result;
}

namespace {
auto to_hex(unsigned long long n)
{
    std::string s;
    do
    {
        s.push_back("0123456789ABCDEF"[n & 15]);
        n >>= 4;
    } while (n);
    std::ranges::reverse(s);
    return s;
};
} // namespace

void append_hover_text(std::string& text, const context::using_context_description& u, const utils::text_convertor* tc)
{
    static constexpr std::string_view private_csect("(PC)");

    bool named = !u.label.empty() || u.section.has_value();

    const string_appender appender { utils::conversion_helper(tc), text };

    if (named)
    {
        text.append("**");

        if (!u.label.empty())
            appender.append(u.label).append(".");

        if (u.section.has_value() && !u.section->empty())
            appender.append(*u.section);
        else if (u.section.has_value() && u.section->empty())
            appender.append(private_csect);

        text.append("**");
    }

    if (u.offset)
    {
        if (u.offset < 0)
            text.append("-");
        else if (named)
            text.append("+");

        text.append("X'").append(to_hex(std::abs((long long)u.offset))).append("'");
    }

    if (u.length != u.regs.size() * 0x1000)
        text.append("(X'").append(to_hex(u.length)).append("')");

    for (auto ro = u.reg_offset; auto reg : u.regs)
    {
        text.append(",R").append(std::to_string(reg));
        if (ro)
            text.append(ro > 0 ? "+" : "-")
                .append("X'")
                .append(to_hex(std::abs((long long)std::exchange(ro, 0))))
                .append("'");
    }
}

std::string hover_text(std::span<const context::using_context_description> usings, const utils::text_convertor* tc)
{
    std::string text;

    if (usings.empty())
        return text;

    text.append("Active USINGs:");

    for (const auto& u : usings)
    {
        text.append(" ");
        append_hover_text(text, u, tc);
    }

    text.append("\n");

    return text;
}

} // namespace hlasm_plugin::parser_library::lsp
