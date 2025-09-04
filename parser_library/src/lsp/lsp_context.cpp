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

#include "lsp_context.h"

#include <cassert>
#include <ranges>
#include <string_view>
#include <unordered_map>

#include "completion_trigger_kind.h"
#include "context/hlasm_context.h"
#include "context/macro.h"
#include "context/ordinary_assembly/section.h"
#include "context/using.h"
#include "document_symbol_item.h"
#include "instructions/instruction.h"
#include "item_convertors.h"
#include "lsp/instruction_completions.h"
#include "lsp/macro_info.h"
#include "parse_lib_provider.h"
#include "utils/string_operations.h"
#include "utils/unicode_text.h"

namespace hlasm_plugin::parser_library::lsp {
namespace {
const std::unordered_map<context::symbol_origin, document_symbol_kind> document_symbol_item_kind_mapping_symbol {
    { context::symbol_origin::DAT, document_symbol_kind::DAT },
    { context::symbol_origin::EQU, document_symbol_kind::EQU },
    { context::symbol_origin::MACH, document_symbol_kind::MACH },
    { context::symbol_origin::ASM, document_symbol_kind::ASM },
    { context::symbol_origin::UNKNOWN, document_symbol_kind::UNKNOWN }
};

const std::unordered_map<context::section_kind, document_symbol_kind> document_symbol_item_kind_mapping_section {
    { context::section_kind::COMMON, document_symbol_kind::COMMON },
    { context::section_kind::DUMMY, document_symbol_kind::DUMMY },
    { context::section_kind::EXECUTABLE, document_symbol_kind::EXECUTABLE },
    { context::section_kind::READONLY, document_symbol_kind::READONLY },
    { context::section_kind::EXTERNAL, document_symbol_kind::EXTERNAL },
    { context::section_kind::WEAK_EXTERNAL, document_symbol_kind::WEAK_EXTERNAL },
    { context::section_kind::EXTERNAL_DSECT, document_symbol_kind::EXTERNAL_DSECT },
};

constexpr bool expand_block(const document_symbol_item& item) { return item.kind != document_symbol_kind::MACRO; }
} // namespace

std::vector<document_symbol_item> lsp_context::document_symbol(
    const utils::resource::resource_location& document_loc) const
{
    using enum document_symbol_kind;
    static constexpr auto block = [](auto f, auto l) {
        return range(position(f, 0), position(l, position::max_value));
    };
    static constexpr auto one_line = [](auto line) {
        return range(position(line, 0), position(line, position::max_value));
    };
    static constexpr auto end_before = [](position& p, position s) {
        if (p.line < s.line)
            p.line = s.line - 1;
    };

    auto dl_it = m_files.find(document_loc);
    if (dl_it == m_files.end())
        return {};

    const auto& dl = dl_it->first;

    std::vector<document_symbol_item> result;

    for (const auto& [title, stack] : m_titles)
    {
        const position* p = nullptr;

        for (auto s = stack; !s.empty(); s = s.parent())
        {
            if (s.frame().resource_loc == dl)
                p = &s.frame().pos;
            if (s.frame().proc_type == context::file_processing_type::MACRO)
                p = nullptr;
        }

        if (p)
            result.emplace_back(title, TITLE, one_line(p->line));
    }

    for (const auto& [def, info] : m_macros)
    {
        if (info->definition_location.resource_loc != dl)
            continue;

        const auto first_line = info->definition_location.pos.line;
        auto last_line = first_line;
        if (!def->copy_nests.empty() && !def->copy_nests.back().empty())
            last_line = def->copy_nests.back().front().loc.pos.line;

        auto& m = result.emplace_back(def->id.to_string(), MACRO, block(first_line, last_line));
        for (const auto& [name, seq] : def->labels)
        {
            if (seq.symbol_location.resource_loc != dl)
                continue;

            m.children.emplace_back("." + name.to_string(), SEQ, one_line(seq.symbol_location.pos.line));
        }
        if (m.children.empty())
            continue;

        std::ranges::sort(m.children, {}, [](const auto& e) { return e.symbol_range.start.line; });

        m.children.back().symbol_range.end.line = last_line;
        m.children.back().symbol_selection_range.end.line = last_line;
        for (auto it = m.children.begin(); it != std::prev(m.children.end()); ++it)
        {
            end_before(it->symbol_range.end, std::next(it)->symbol_range.start);
            end_before(it->symbol_selection_range.end, std::next(it)->symbol_selection_range.start);
        }
    }

    const auto macro_map = dl_it->second.macro_map();

    if (auto it = m_opencode->file_occurrences.find(dl); it != m_opencode->file_occurrences.end())
    {
        for (const auto& o : it->second.symbols)
        {
            if (o.occurrence_range.start.column != 0)
                continue;
            if (o.name.empty())
                continue;

            std::string prefix;
            document_symbol_kind kind = UNKNOWN;

            switch (o.kind)
            {
                case occurrence_kind::ORD:
                    if (const auto* sym = m_hlasm_ctx->ord_ctx.get_symbol(o.name))
                    {
                        if (auto origin = sym->attributes().origin(); origin != context::symbol_origin::SECT)
                            kind = document_symbol_item_kind_mapping_symbol.at(origin);
                        else if (const auto* sect = m_hlasm_ctx->ord_ctx.get_section(o.name))
                            kind = document_symbol_item_kind_mapping_section.at(sect->kind);
                    }
                    break;
                case occurrence_kind::SEQ:
                    prefix = ".";
                    kind = SEQ;
                    break;
                default:
                    continue;
            }
            auto l = o.occurrence_range.start.line;
            if (l >= macro_map.size() || !macro_map[l])
                result.emplace_back(prefix + o.name.to_string(), kind, one_line(o.occurrence_range.start.line));
        }
    }

    std::ranges::stable_sort(result, [](const auto& l, const auto& r) {
        if (const auto c = l.symbol_range.start.line <=> r.symbol_range.start.line; c != 0)
            return c < 0;

        const bool l_title = l.kind == TITLE;
        const bool r_title = r.kind == TITLE;

        return l_title > r_title;
    });

    if (!result.empty() && expand_block(result.back()))
    {
        const auto lines = dl_it->second.data.get_number_of_lines();
        end_before(result.back().symbol_range.end, position(lines, 0));
        end_before(result.back().symbol_selection_range.end, position(lines, 0));
    }

    static constexpr auto same_symbol = [](const auto& l, const auto& r) {
        return l.name == r.name && l.kind == r.kind;
    };
    for (auto it = result.begin(); it != result.end();)
    {
        auto next = std::next(it);

        while (next != result.end() && same_symbol(*it, *next))
            ++next;

        if (next != result.end() && expand_block(*it))
        {
            end_before(it->symbol_range.end, next->symbol_range.start);
            end_before(it->symbol_selection_range.end, next->symbol_selection_range.start);
        }
        else
        {
            auto last = std::prev(next);
            it->symbol_range.end = last->symbol_range.end;
            it->symbol_selection_range.end = last->symbol_selection_range.end;
        }

        it = next;
    }

    result.erase(std::ranges::unique(result, same_symbol).begin(), result.end());

    static constexpr auto is_title = [](const auto& s) { return s.kind == TITLE; };

    for (auto it = result.rbegin(); it != result.rend();)
    {
        const auto title = std::find_if(it, result.rend(), is_title);

        if (title == result.rend())
            break;

        const auto next = std::next(title);

        title->children.insert(
            title->children.end(), std::make_move_iterator(title.base()), std::make_move_iterator(it.base()));
        result.erase(title.base(), it.base());

        it = next;
    }

    for (auto it = std::ranges::find_if(result, is_title); it != result.end();)
    {
        auto next = std::next(it);

        if (expand_block(*it))
        {
            if (next == result.end())
            {
                const auto lines = dl_it->second.data.get_number_of_lines();
                end_before(it->symbol_range.end, position(lines, 0));
                end_before(it->symbol_selection_range.end, position(lines, 0));
            }
            else
            {
                end_before(it->symbol_range.end, next->symbol_range.start);
                end_before(it->symbol_selection_range.end, next->symbol_selection_range.start);
            }
        }

        it = next;
    }

    return result;
}

lsp_context::lsp_context(std::shared_ptr<context::hlasm_context> h_ctx)
    : m_hlasm_ctx(std::move(h_ctx))
{}

void lsp_context::add_copy(context::copy_member_ptr copy, text_data_view text_data)
{
    m_files.try_emplace(copy->definition_location.resource_loc, std::move(copy), std::move(text_data));
}

void lsp_context::add_macro(macro_info_ptr macro_i, text_data_view text_data)
{
    if (macro_i->external)
        m_files.try_emplace(macro_i->definition_location.resource_loc, macro_i->macro_definition, std::move(text_data));

    auto [_, inserted] = m_macros.try_emplace(macro_i->macro_definition.get(), std::move(macro_i));
    assert(inserted);
}

void lsp_context::add_opencode(opencode_info_ptr opencode_i, text_data_view text_data, parse_lib_provider& libs)
{
    m_opencode = std::move(opencode_i);
    m_files.try_emplace(m_hlasm_ctx->opencode_location(), std::move(text_data));

    file_info::distribute_macro_slices(m_macros, m_files);

    for (const auto& [_, m] : m_macros)
        distribute_file_occurrences(m->file_occurrences);
    distribute_file_occurrences(m_opencode->file_occurrences);

    for (auto& [_, file] : m_files)
        file.process_occurrences();

    for (const auto& [_, file] : m_files)
        file.collect_instruction_like_references(m_instr_like);

    std::erase_if(m_instr_like, [this](const auto& e) { return have_suggestions_for_instr_like(e.first); });
    for (auto& [key, value] : m_instr_like)
    {
        libs.has_library(key.to_string_view(), &value);
    }
    std::erase_if(m_instr_like, [](const auto& e) { return e.second.empty(); });
}

void lsp_context::add_title(std::string title, context::processing_stack_t stack)
{
    m_titles.emplace_back(std::move(title), std::move(stack));
}

macro_info_ptr lsp_context::get_macro_info(context::id_index macro_name, context::opcode_generation gen) const
{
    // This function does not respect OPSYN, so we do not use hlasm_context::get_macro_definition
    if (auto it = m_hlasm_ctx->find_macro(macro_name, gen); !it)
        return nullptr;
    else
        return m_macros.at(it->get());
}

const file_info* lsp_context::get_file_info(const utils::resource::resource_location& file_loc) const
{
    if (auto it = m_files.find(file_loc); it != m_files.end())
        return &it->second;
    else
        return nullptr;
}

location lsp_context::definition(const utils::resource::resource_location& document_loc, position pos) const
{
    auto [occ, macro_scope] = find_occurrence_with_scope(document_loc, pos);

    if (!occ)
        return { pos, document_loc };

    if (auto def = find_definition_location(*occ, macro_scope, document_loc, pos))
        return { def->pos, def->resource_loc };
    return { pos, document_loc };
}

void collect_references(std::vector<location>& refs, const symbol_occurrence& occ, const file_occurrences_t& file_occs)
{
    for (const auto& [file, occs] : file_occs)
    {
        auto file_refs = file_info::find_references(occ, occs.symbols);
        for (auto&& ref : file_refs)
            refs.emplace_back(std::move(ref), file);
    }
}

std::vector<location> lsp_context::references(
    const utils::resource::resource_location& document_loc, position pos) const
{
    std::vector<location> result;

    auto [occ, macro_scope] = find_occurrence_with_scope(document_loc, pos);

    if (!occ)
        return {};

    if (occ->is_scoped())
    {
        if (macro_scope)
            collect_references(result, *occ, macro_scope->file_occurrences);
        else
            collect_references(result, *occ, m_opencode->file_occurrences);
    }
    else
    {
        for (const auto& [_, mac_i] : m_macros)
            collect_references(result, *occ, mac_i->file_occurrences);
        collect_references(result, *occ, m_opencode->file_occurrences);
    }

    return result;
}

std::string lsp_context::hover(const utils::resource::resource_location& document_loc, position pos) const
{
    auto [occ, macro_scope] = find_occurrence_with_scope(document_loc, pos);

    if (!occ)
        return {};

    return find_hover(*occ,
        macro_scope,
        find_line_details(document_loc, occ->occurrence_range.start.line),
        find_definition_location(*occ, macro_scope, document_loc, pos));
}

bool lsp_context::should_complete_instr(const text_data_view& text, position pos) const
{
    if (pos.line > 0 && is_continued_line(text.get_line(pos.line - 1)))
        return false;

    std::string_view line_so_far = text.get_line_beginning_at(pos);

    if (line_so_far.empty() || line_so_far.starts_with("*") || line_so_far.starts_with(".*"))
        return false;

    if (line_so_far.front() != ' ')
    {
        auto next_space = line_so_far.find(' ');
        if (next_space == std::string_view::npos)
            return false;
        line_so_far.remove_prefix(next_space);
    }
    utils::trim_left(line_so_far);
    return line_so_far.find(' ') == std::string_view::npos;
}

std::vector<std::pair<const context::section*, context::id_index>> gather_reachable_sections(
    context::hlasm_context& ctx, std::pair<const context::section*, index_t<context::using_collection>> reachables)
{
    const auto& [sect, usings] = reachables;

    std::vector<std::pair<const context::section*, context::id_index>> reachable_sections;
    if (sect)
        reachable_sections.emplace_back(sect, context::id_index());
    if (usings)
    {
        for (const auto& u : ctx.usings().describe(usings))
        {
            if (!u.section)
                continue;
            if (const auto* s = ctx.ord_ctx.get_section(*u.section))
                reachable_sections.emplace_back(s, u.label);
        }
    }
    std::ranges::sort(reachable_sections);
    reachable_sections.erase(std::ranges::unique(reachable_sections).begin(), reachable_sections.end());

    return reachable_sections;
}

std::vector<std::pair<const context::symbol*, context::id_index>> compute_reachable_symbol_set(
    const std::vector<std::pair<const context::section*, context::id_index>>& reachable_sections,
    const context::ordinary_assembly_context& ord_ctx,
    bool include_all_sections)
{
    std::vector<std::pair<const context::symbol*, context::id_index>> reachable_symbols;

    for (const auto& [name, symv] : ord_ctx.symbols())
    {
        const auto* sym = std::get_if<context::symbol>(&symv);
        if (!sym)
            continue;

        if (sym->kind() == context::symbol_value_kind::ABS)
        {
            reachable_symbols.emplace_back(sym, context::id_index());
            continue;
        }

        if (sym->kind() != context::symbol_value_kind::RELOC)
            continue;

        const auto& reloc = sym->value().get_reloc();
        if (!reloc.is_simple())
            continue;

        for (const auto& [sect, label] : reachable_sections)
        {
            if (sect != reloc.bases().front().owner)
                continue;
            reachable_symbols.emplace_back(sym, label);
        }
    }

    if (include_all_sections)
    {
        for (const auto& sp : ord_ctx.sections())
        {
            using enum context::section_kind;
            if (sp->kind != DUMMY && sp->kind != READONLY && sp->kind != EXECUTABLE)
                continue;

            if (const auto* sym = ord_ctx.get_symbol(sp->name))
                reachable_symbols.emplace_back(sym, context::id_index());
        }
    }

    std::ranges::sort(reachable_symbols);
    reachable_symbols.erase(std::ranges::unique(reachable_symbols).begin(), reachable_symbols.end());

    return reachable_symbols;
}

completion_list_source lsp_context::completion(const utils::resource::resource_location& document_uri,
    position pos,
    const char trigger_char,
    completion_trigger_kind trigger_kind) const
{
    const auto* file_info = get_file_info(document_uri);
    if (!file_info)
        return {};
    const text_data_view& text = file_info->data;

    char last_char =
        (trigger_kind == completion_trigger_kind::trigger_character) ? trigger_char : text.get_character_before(pos);

    if (last_char == '&')
        return complete_var(*file_info, pos);
    else if (last_char == '.')
        return complete_seq(*file_info, pos);
    else if (should_complete_instr(text, pos))
        return complete_instr(*file_info, pos);
    else
    {
        auto instr = file_info->find_closest_instruction(pos);
        const auto is_using = instr && instr->name == context::id_index("USING");
        auto [section, usings] = file_info->find_reachable_sections(pos);

        if (!section) // use private section if exists
            section = m_hlasm_ctx->ord_ctx.get_section(context::id_index());

        auto reachable_sections = gather_reachable_sections(*m_hlasm_ctx, { section, usings });

        auto reachable_symbols = compute_reachable_symbol_set(reachable_sections, m_hlasm_ctx->ord_ctx, is_using);

        return std::pair(instr ? instr->opcode : nullptr, std::move(reachable_symbols));
    }
}

completion_list_source lsp_context::complete_var(const file_info& file, position pos) const
{
    const auto* scope = file.find_scope(pos);

    return scope ? &scope->var_definitions : &m_opencode->variable_definitions;
}

completion_list_source lsp_context::complete_seq(const file_info& file, position pos) const
{
    const auto* macro_i = file.find_scope(pos);

    if (macro_i)
        return &macro_i->macro_definition->labels;
    else
        return &m_hlasm_ctx->get_opencode_sequence_symbols();
}

completion_list_source lsp_context::complete_instr(const file_info& fi, position pos) const
{
    completion_list_source result(std::in_place_type<completion_list_instructions>);

    auto& value = std::get<completion_list_instructions>(result);

    value.completed_text = utils::utf8_substr<false>(fi.data.get_line(pos.line), 0, pos.column).str;
    value.lsp_ctx = this;

    if (auto completion_start = value.completed_text.rfind(' '); completion_start == std::string_view::npos)
        value.completed_text_start_column = pos.column;
    else
    {
        value.completed_text_start_column = completion_start + 2; // after and turn to column
        value.completed_text.remove_prefix(completion_start + 1);
    }

    value.macros = &m_macros;

    return result;
}

void lsp_context::distribute_file_occurrences(const file_occurrences_t& occurrences)
{
    for (auto& [file, occs] : occurrences)
        m_files.at(file).update_occurrences(occs.symbols, occs.line_details);
}

occurrence_scope_t lsp_context::find_occurrence_with_scope(
    const utils::resource::resource_location& document_loc, position pos) const
{
    if (auto file = m_files.find(document_loc); file != m_files.end())
        return file->second.find_occurrence_with_scope(pos);
    return std::make_pair(nullptr, nullptr);
}

const line_occurence_details* lsp_context::find_line_details(
    const utils::resource::resource_location& document_loc, size_t l) const
{
    if (auto file = m_files.find(document_loc); file != m_files.end())
        return file->second.get_line_details(l);
    return nullptr;
}

location lsp_context::find_symbol_definition_location(
    const context::symbol& sym, const utils::resource::resource_location& document_loc, position pos) const
{
    std::pair<occurrence_scope_t, context::processing_stack_t> top_reference;
    const auto& [top_scope, top_stack] = top_reference;

    auto stack = sym.proc_stack();
    while (!stack.empty())
    {
        const auto& frame = stack.frame();

        if (frame.resource_loc == document_loc && frame.pos.line == pos.line)
        {
            top_reference = {};
            break;
        }

        auto scope = find_occurrence_with_scope(frame.resource_loc, position(frame.pos.line, 0));

        if (scope.first && scope.first->kind == lsp::occurrence_kind::ORD && scope.first->name == sym.name())
            top_reference = { scope, stack };

        stack = stack.parent();
    }
    if (top_scope.first)
        return location(top_scope.first->occurrence_range.start, top_stack.frame().resource_loc);
    else
        return sym.symbol_location();
}

std::optional<location> find_symbol_location(const auto& seq_syms, context::id_index name)
{
    if (const auto sym = seq_syms.find(name); sym != seq_syms.end())
        return sym->second.symbol_location;
    return std::nullopt;
}

std::optional<location> lsp_context::find_definition_location(const symbol_occurrence& occ,
    const macro_info* macro_scope_i,
    const utils::resource::resource_location& document_loc,
    position pos) const
{
    switch (occ.kind)
    {
        case lsp::occurrence_kind::ORD: {
            if (auto sym = m_hlasm_ctx->ord_ctx.get_symbol(occ.name))
                return find_symbol_definition_location(*sym, document_loc, pos);
            break;
        }
        case lsp::occurrence_kind::SEQ:
            if (macro_scope_i)
                return find_symbol_location(macro_scope_i->macro_definition->labels, occ.name);
            else
                return find_symbol_location(m_hlasm_ctx->get_opencode_sequence_symbols(), occ.name);

        case lsp::occurrence_kind::VAR: {
            const vardef_storage& var_syms =
                macro_scope_i ? macro_scope_i->var_definitions : m_opencode->variable_definitions;

            auto sym = std::ranges::find(var_syms, occ.name, &variable_symbol_definition::name);

            if (sym != var_syms.end())
            {
                if (macro_scope_i)
                    return location(sym->def_position,
                        macro_scope_i->macro_definition->get_copy_nest(sym->def_location).back().loc.resource_loc);
                return location(sym->def_position, sym->file);
            }
            break;
        }
        case lsp::occurrence_kind::INSTR: {
            if (occ.opcode)
            {
                if (auto it = m_macros.find(occ.opcode); it != m_macros.end())
                    return it->second->definition_location;
            }
            break;
        }
        case lsp::occurrence_kind::INSTR_LIKE: {
            if (auto it = m_macros.find(occ.opcode); it != m_macros.end())
                return it->second->definition_location;
            if (auto op = m_hlasm_ctx->find_any_valid_opcode(occ.name); op && op->is_macro())
            {
                return op->get_macro_details()->definition_location;
            }
            if (auto it = m_instr_like.find(occ.name); it != m_instr_like.end())
                return location(position(), it->second);
            break;
        }
        case lsp::occurrence_kind::COPY_OP: {
            auto copy = std::ranges::find_if(m_files, [&](const auto& f) {
                return f.second.type == file_type::COPY
                    && std::get<context::copy_member_ptr>(f.second.owner)->name == occ.name;
            });
            if (copy != m_files.end())
                return std::get<context::copy_member_ptr>(copy->second.owner)->definition_location;
            break;
        }
        default:
            break;
    }
    return std::nullopt;
}

std::string lsp_context::hover_for_macro(const macro_info& macro) const
{
    // Get file, where the macro is defined
    auto mit = m_files.find(macro.definition_location.resource_loc);
    if (mit == m_files.end())
        return "";

    return get_macro_documentation(mit->second.data, macro.definition_location.pos.line);
}
std::string lsp_context::hover_for_instruction(context::id_index name) const
{
    const auto iset = m_hlasm_ctx->options().instr_set;
    const auto iname = name.to_string_view();
    const auto& ici = instruction_completion_items;

    static constexpr auto label = [](const auto& e) -> decltype(auto) { return e.first.label; };

    for (auto it = std::ranges::lower_bound(ici, iname, {}, label); it != ici.end(); ++it)
    {
        if (it->first.label != iname)
            return "";
        if (instructions::instruction_available(it->second, iset))
            return it->first.documentation;
    }

    return "";
}

bool lsp_context::have_suggestions_for_instr_like(context::id_index name) const
{
    return m_hlasm_ctx->find_any_valid_opcode(name);
}

std::string lsp_context::find_hover(const symbol_occurrence& occ,
    const macro_info* macro_scope_i,
    const line_occurence_details* ld,
    std::optional<location> definition) const
{
    const auto prefix_using = [this, ld](std::string s) {
        auto u = ld ? hover_text(m_hlasm_ctx->usings().describe(ld->active_using)) : "";

        if (u.empty())
            return s;

        if (!s.empty())
            u.append("\n---\n").append(s);

        return u;
    };
    switch (occ.kind)
    {
        case lsp::occurrence_kind::ORD: {
            std::string result;
            auto sym = m_hlasm_ctx->ord_ctx.get_symbol(occ.name);
            if (sym)
            {
                result = hover_text(*sym);
            }
            if (const file_info * fi; definition && (fi = get_file_info(definition->resource_loc)) != nullptr)
            {
                if (auto text = get_logical_line(fi->data, definition->pos.line); !text.empty())
                {
                    if (!result.empty())
                        result.push_back('\n');
                    result.append(text);
                }
            }
            return result;
        }
        case lsp::occurrence_kind::SEQ:
            return "Sequence symbol";

        case lsp::occurrence_kind::VAR: {
            const vardef_storage& var_syms =
                macro_scope_i ? macro_scope_i->var_definitions : m_opencode->variable_definitions;

            if (const auto sym = std::ranges::find(var_syms, occ.name, &variable_symbol_definition::name);
                sym != var_syms.end())
                return hover_text(*sym);
            break;
        }
        case lsp::occurrence_kind::INSTR: {
            if (occ.opcode)
            {
                auto it = m_macros.find(occ.opcode);
                assert(it != m_macros.end());

                return prefix_using(hover_for_macro(*it->second));
            }
            else
            {
                return prefix_using(hover_for_instruction(occ.name));
            }
        }
        case lsp::occurrence_kind::INSTR_LIKE: {
            if (auto it = m_macros.find(occ.opcode); it != m_macros.end())
                return hover_for_macro(*it->second);
            if (auto op = m_hlasm_ctx->find_any_valid_opcode(occ.name))
            {
                if (op->is_macro())
                    return prefix_using(hover_for_macro(*m_macros.at(op->get_macro_details())));
                else
                    return prefix_using(hover_for_instruction(op->opcode));
            }
            if (m_instr_like.contains(occ.name))
                return "Statement not executed, macro with matching name available";
            break;
        }
        case lsp::occurrence_kind::COPY_OP:
            return "";

        default:
            break;
    }
    return {};
}


std::vector<branch_info> lsp_context::get_opencode_branch_info() const
{
    std::vector<branch_info> result;

    auto file = m_files.find(m_hlasm_ctx->opencode_location());
    if (file == m_files.end())
        return result;

    const auto& details = file->second.get_line_details();
    for (size_t i = 0; i < details.size(); ++i)
    {
        const auto& ld = details[i];
        using enum branch_direction;
        auto dir = none;

        if (ld.branches_up)
            dir = dir | up;
        if (ld.branches_down)
            dir = dir | down;
        if (ld.branches_somewhere)
            dir = dir | somewhere;

        if (dir != none)
            result.emplace_back(i, ld.offset_to_jump_opcode, dir);
    }

    return result;
}

} // namespace hlasm_plugin::parser_library::lsp
