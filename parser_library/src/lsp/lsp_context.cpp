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
#include <limits>
#include <regex>
#include <sstream>
#include <string_view>
#include <type_traits>
#include <unordered_map>

#include "context/instruction.h"
#include "ebcdic_encoding.h"
#include "utils/similar.h"

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

hover_result hover_text(const context::symbol& sym)
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
            if (*base.owner->name == "" || d == 0)
                continue;

            bool was_first = std::exchange(first, false);
            if (d < 0)
                markdown.append(was_first ? "-" : " - ");
            else if (!was_first)
                markdown.append(" + ");

            if (d != 1 && d != -1)
                markdown.append(std::to_string(-(unsigned)d)).append("*");

            if (base.qualifier)
                markdown.append(*base.qualifier).append(".");
            markdown.append(*base.owner->name);
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

hover_result hover_text(const variable_symbol_definition& sym)
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
} // namespace

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
};

const std::unordered_map<occurence_kind, document_symbol_kind> document_symbol_item_kind_mapping_macro {
    { occurence_kind::VAR, document_symbol_kind::VAR }, { occurence_kind::SEQ, document_symbol_kind::SEQ }
};

std::string lsp_context::find_macro_copy_id(const std::vector<context::processing_frame>& stack, unsigned long i) const
{
    assert(i != 0);
    assert(i < stack.size());
    return stack[i].member_name == context::id_storage::empty_id ? stack[i].resource_loc->get_uri()
                                                                 : *stack[i].member_name;
}

void lsp_context::document_symbol_macro(document_symbol_list_s& result,
    const utils::resource::resource_location& document_loc,
    std::optional<range> r,
    long long& limit,
    document_symbol_cache& cache) const
{
    auto m = std::find_if(m_macros.begin(), m_macros.end(), [&document_loc](const auto& mac) {
        return mac.first->definition_location.resource_loc == document_loc;
    });
    if (m == m_macros.end())
        return;

    const auto& [def, info] = *m;

    const auto& copy_occs = copy_occurences(document_loc, cache);

    for (const auto& var : info->var_definitions)
    {
        if (limit <= 0)
            break;
        if (!belongs_to_copyfile(document_loc, var.def_position, var.name))
        {
            result.emplace_back(*var.name, document_symbol_kind::VAR, r.value_or(range(var.def_position)));
            --limit;
        }
        else if (!r.has_value())
            modify_with_copy(result, var.name, copy_occs, document_symbol_kind::VAR, limit);
    }
    for (const auto& [name, seq] : def->labels)
    {
        if (limit <= 0)
            break;
        if (!belongs_to_copyfile(document_loc, seq->symbol_location.pos, name))
        {
            result.emplace_back(*name, document_symbol_kind::SEQ, r.value_or(range(seq->symbol_location.pos)));
            --limit;
        }
        else if (!r.has_value())
            modify_with_copy(result, name, copy_occs, document_symbol_kind::SEQ, limit);
    }
}

bool lsp_context::belongs_to_copyfile(
    const utils::resource::resource_location& document_loc, position pos, context::id_index id) const
{
    const auto* aux = find_occurence_with_scope(document_loc, pos).first;
    return aux == nullptr || *aux->name != *id;
}

void lsp_context::document_symbol_copy(document_symbol_list_s& result,
    const std::vector<symbol_occurence>& occurence_list,
    const utils::resource::resource_location& document_loc,
    std::optional<range> r,
    long long& limit) const
{
    for (const auto& occ : occurence_list)
    {
        if (limit <= 0)
            return;
        if (occ.kind == occurence_kind::VAR || occ.kind == occurence_kind::SEQ)
        {
            position aux = definition(document_loc, occ.occurence_range.start).pos;
            document_symbol_item_s item = {
                *occ.name,
                document_symbol_item_kind_mapping_macro.at(occ.kind),
                r.value_or(range(aux,
                    position(aux.line, aux.column + occ.occurence_range.end.column - occ.occurence_range.start.column)))
            };
            if (std::none_of(result.begin(), result.end(), utils::is_similar_to(item)))
            {
                result.push_back(item);
                --limit;
            }
        }
    }
}

std::span<const symbol_occurence* const> lsp_context::get_occurences_by_name(
    const file_info& document, context::id_index name, document_symbol_cache& cache) const
{
    auto [it, inserted] = cache.occurences_by_name.try_emplace(&document);

    auto& occurences_by_name = it->second;
    if (inserted)
    {
        const auto& occurences = document.get_occurences();
        occurences_by_name.reserve(occurences.size());
        std::transform(
            occurences.begin(), occurences.end(), std::back_inserter(occurences_by_name), [](const auto& occ) {
                return &occ;
            });
        std::sort(occurences_by_name.begin(), occurences_by_name.end(), [](const auto* l, const auto* r) {
            return l->name < r->name;
        });
    }

    if (name == nullptr)
        return occurences_by_name;

    struct
    {
        bool operator()(const symbol_occurence* l, context::id_index r) const { return l->name < r; }
        bool operator()(context::id_index l, const symbol_occurence* r) const { return l < r->name; }
    } search_predicate;

    auto [low, high] = std::equal_range(occurences_by_name.begin(), occurences_by_name.end(), name, search_predicate);

    if (low == high) // missing c++20 ctor in libc++ 12 and broken std::to_address
        return std::span<const symbol_occurence* const>();
    else
        return std::span<const symbol_occurence* const>(&*low, std::distance(low, high));
}

void lsp_context::fill_cache(
    std::vector<std::pair<symbol_occurence, lsp_context::vector_set<context::id_index>>>& copy_occurences,
    const utils::resource::resource_location& document_loc,
    document_symbol_cache& cache) const
{
    const auto& document = *m_files.at(document_loc);
    for (const auto& [_, info] : m_files)
    {
        if (info->type != file_type::COPY)
            continue;

        for (const auto* occ :
            get_occurences_by_name(document, std::get<context::copy_member_ptr>(info->owner)->name, cache))
        {
            lsp_context::vector_set<context::id_index> occurences;
            for (context::id_index last = nullptr; const auto* new_occ : get_occurences_by_name(*info, nullptr, cache))
            {
                if (last == new_occ->name)
                    continue;
                last = new_occ->name;

                if (new_occ->kind == occurence_kind::VAR || new_occ->kind == occurence_kind::SEQ)
                    occurences.data.push_back(new_occ->name);
            }
            copy_occurences.emplace_back(*occ, std::move(occurences));
        }
    }
}

const std::vector<std::pair<symbol_occurence, lsp_context::vector_set<context::id_index>>>&
lsp_context::copy_occurences(const utils::resource::resource_location& document_loc, document_symbol_cache& cache) const
{
    auto [it, inserted] = cache.occurences.try_emplace(document_loc);

    if (inserted)
        fill_cache(it->second, document_loc, cache);

    return it->second;
}

void lsp_context::modify_with_copy(document_symbol_list_s& modified,
    context::id_index sym_name,
    const std::vector<std::pair<symbol_occurence, lsp_context::vector_set<context::id_index>>>& copy_occs,
    const document_symbol_kind kind,
    long long& limit) const
{
    for (const auto& [copy_occ, occs] : copy_occs)
    {
        if (limit <= 0)
            return;
        if (!occs.contains(sym_name))
            continue;

        bool have_already = false;
        document_symbol_item_s sym_item(*sym_name, kind, copy_occ.occurence_range);
        for (auto& item : modified)
        {
            if (item.name == *copy_occ.name
                && std::none_of(item.children.begin(), item.children.end(), utils::is_similar_to(sym_item)))
            {
                item.children.push_back(sym_item);
                have_already = true;
                --limit;
                break;
            }
        }
        if (!have_already)
        {
            modified.emplace_back(*copy_occ.name,
                document_symbol_kind::MACRO,
                copy_occ.occurence_range,
                document_symbol_list_s { std::move(sym_item) });
            --limit;
        }
    }
}

bool do_not_need_nodes(const std::vector<context::processing_frame>& sym,
    const std::vector<context::processing_frame>& sect_sym,
    unsigned long& i)
{
    if (sym.size() == 1)
    {
        return true;
    }
    const auto size = sym.size() < sect_sym.size() ? sym.size() : sect_sym.size();
    while (i < size)
    {
        if (sym[i].pos != sect_sym[i].pos || sym[i].resource_loc != sect_sym[i].resource_loc)
        {
            if (i + 1 == sym.size())
            {
                return true;
            }
            if (i < sym.size() - 1)
            {
                i++;
            }
            return false;
        }
        i++;
    }
    return false;
}

void lsp_context::document_symbol_symbol(document_symbol_list_s& modified,
    document_symbol_list_s children,
    context::id_index id,
    const std::vector<context::processing_frame>& sym_stack,
    document_symbol_kind kind,
    unsigned long i,
    long long& limit) const
{
    document_symbol_item_s aux(find_macro_copy_id(sym_stack, i), document_symbol_kind::MACRO, range(sym_stack[0].pos));

    const auto comp_aux = [&aux](const auto& e) {
        return aux.name == e.name && aux.kind == e.kind && aux.symbol_range == e.symbol_range
            && aux.symbol_selection_range == e.symbol_selection_range;
    };

    auto i_find = std::find_if(modified.begin(), modified.end(), comp_aux);
    if (i_find == modified.end())
    {
        modified.push_back(aux);
        --limit;
        i_find = modified.end() - 1;
    }
    i++;
    while (i < sym_stack.size())
    {
        aux.name = find_macro_copy_id(sym_stack, i);
        document_symbol_list_s* aux_list = &i_find->children;
        i_find = std::find_if(aux_list->begin(), aux_list->end(), comp_aux);
        if (i_find == aux_list->end())
        {
            aux_list->push_back(aux);
            --limit;
            i_find = aux_list->end() - 1;
        }
        i++;
    }
    i_find->children.emplace_back(*id, kind, i_find->symbol_range, std::move(children));
    --limit;
}

void lsp_context::document_symbol_opencode_ord_symbol(document_symbol_list_s& result, long long& limit) const
{
    const auto& symbol_list = m_hlasm_ctx->ord_ctx.symbols();
    std::map<const context::section*, document_symbol_list_s> children_of_sects;
    for (const auto& [id, sym_var] : symbol_list)
    {
        const auto* sym = std::get_if<context::symbol>(&sym_var);
        if (sym && sym->attributes().origin == context::symbol_origin::SECT)
        {
            if (auto sect = m_hlasm_ctx->ord_ctx.get_section(id))
            {
                children_of_sects.try_emplace(sect, document_symbol_list_s {});
                --limit;
            }
        }
    }

    std::vector<context::processing_frame> sym_stack;
    std::vector<context::processing_frame> sect_sym_stack;

    for (const auto& [id, sym_var] : symbol_list)
    {
        if (limit <= 0)
            break;
        if (!std::holds_alternative<context::symbol>(sym_var))
            continue;
        const auto& sym = std::get<context::symbol>(sym_var);
        if (sym.attributes().origin == context::symbol_origin::SECT)
            continue;

        sym.proc_stack().to_vector(sym_stack);

        const auto* sect =
            sym.value().value_kind() == context::symbol_value_kind::RELOC && sym.value().get_reloc().bases().size() == 1
            ? sym.value().get_reloc().bases().front().first.owner
            : nullptr;
        if (sect == nullptr || children_of_sects.find(sect) == children_of_sects.end())
        {
            if (sym_stack.size() == 1)
            {
                result.emplace_back(*id,
                    document_symbol_item_kind_mapping_symbol.at(sym.attributes().origin),
                    range(sym.symbol_location.pos));
                --limit;
            }
            else
            {
                document_symbol_symbol(result,
                    document_symbol_list_s {},
                    id,
                    sym_stack,
                    document_symbol_item_kind_mapping_symbol.at(sym.attributes().origin),
                    1,
                    limit);
            }
        }
        else
        {
            const auto* sect_sym = m_hlasm_ctx->ord_ctx.get_symbol(sect->name);

            sect_sym->proc_stack().to_vector(sect_sym_stack);

            auto& children = children_of_sects.find(sect)->second;
            unsigned long i = 1;
            if (do_not_need_nodes(sym_stack, sect_sym_stack, i))
            {
                children.emplace_back(
                    *id, document_symbol_item_kind_mapping_symbol.at(sym.attributes().origin), range(sym_stack[0].pos));
                --limit;
            }
            else
            {
                document_symbol_symbol(children,
                    document_symbol_list_s {},
                    id,
                    sym_stack,
                    document_symbol_item_kind_mapping_symbol.at(sym.attributes().origin),
                    i,
                    limit);
            }
        }
    }

    for (auto&& [sect, children] : children_of_sects)
    {
        const auto& sym = *m_hlasm_ctx->ord_ctx.get_symbol(sect->name);

        sym.proc_stack().to_vector(sym_stack);

        if (sym_stack.size() == 1)
        {
            result.emplace_back(*sect->name,
                document_symbol_item_kind_mapping_section.at(sect->kind),
                range(sym.symbol_location.pos),
                std::move(children));
        }
        else
        {
            ++limit; // already counted
            document_symbol_symbol(result,
                std::move(children),
                sect->name,
                sym_stack,
                document_symbol_item_kind_mapping_section.at(sect->kind),
                1,
                limit);
        }
    }
}

void lsp_context::document_symbol_opencode_var_seq_symbol_aux(document_symbol_list_s& result,
    const std::unordered_map<std::string_view, utils::resource::resource_location>& name_to_location_cache,
    long long& limit,
    document_symbol_cache& cache) const
{
    for (auto& item : result)
    {
        if (item.kind != document_symbol_kind::MACRO)
            continue;

        auto location = name_to_location_cache.find(item.name);
        if (location == name_to_location_cache.end() || location->second.get_uri().empty())
            return;

        if (const auto& file = m_files.find(location->second); file != m_files.end())
        {
            if (file->second->type == file_type::MACRO)
                document_symbol_macro(item.children, file->first, item.symbol_range, limit, cache);
            else if (file->second->type == file_type::COPY)
                document_symbol_copy(
                    item.children, file->second->get_occurences(), file->first, item.symbol_range, limit);
        }
        document_symbol_opencode_var_seq_symbol_aux(item.children, name_to_location_cache, limit, cache);
    }
}

void lsp_context::document_symbol_other(document_symbol_list_s& result,
    const utils::resource::resource_location& document_loc,
    long long& limit,
    document_symbol_cache& cache) const
{
    std::unordered_map<std::string_view, utils::resource::resource_location> name_to_location;
    for (const auto& [def, info] : m_macros)
        name_to_location.insert_or_assign(*def->id, info->definition_location.resource_loc);
    for (const auto& [def, info] : m_hlasm_ctx->copy_members())
        name_to_location.insert_or_assign(*info->name, info->definition_location.resource_loc);

    document_symbol_opencode_ord_symbol(result, limit);
    document_symbol_opencode_var_seq_symbol_aux(result, name_to_location, limit, cache);

    for (const auto& sym : m_opencode->variable_definitions)
    {
        if (limit <= 0)
            break;
        if (!belongs_to_copyfile(document_loc, sym.def_position, sym.name))
        {
            result.emplace_back(*sym.name, document_symbol_kind::VAR, range(sym.def_position));
            --limit;
        }
    }
}

document_symbol_list_s lsp_context::document_symbol(
    const utils::resource::resource_location& document_loc, long long limit) const
{
    document_symbol_list_s result;
    const auto& file = m_files.find(document_loc);
    if (file == m_files.end() || limit <= 0)
        return result;

    document_symbol_cache cache;

    switch (file->second->type)
    {
        case file_type::MACRO:
            document_symbol_macro(result, document_loc, std::nullopt, limit, cache);
            break;

        case file_type::COPY:
            document_symbol_copy(result, file->second->get_occurences(), document_loc, std::nullopt, limit);
            break;

        default:
            document_symbol_other(result, document_loc, limit, cache);
            break;
    }
    if (limit <= 0)
        result.emplace(result.begin(), "Outline may be truncated", document_symbol_kind::DUMMY, range());

    return result;
}



void lsp_context::add_file(file_info file_i)
{
    m_files.try_emplace(file_i.location, std::make_unique<file_info>(file_i));
}

lsp_context::lsp_context(std::shared_ptr<context::hlasm_context> h_ctx)
    : m_hlasm_ctx(std::move(h_ctx))
{}

void lsp_context::add_copy(context::copy_member_ptr copy, text_data_ref_t text_data)
{
    add_file(file_info(std::move(copy), std::move(text_data)));
}

void lsp_context::add_macro(macro_info_ptr macro_i, text_data_ref_t text_data)
{
    if (macro_i->external)
        add_file(file_info(macro_i->macro_definition, std::move(text_data)));

    m_macros[macro_i->macro_definition] = macro_i;
}

void lsp_context::add_opencode(opencode_info_ptr opencode_i, text_data_ref_t text_data)
{
    m_opencode = std::move(opencode_i);
    add_file(file_info(m_hlasm_ctx->opencode_location(), std::move(text_data)));

    // distribute all occurrences as all files are present
    for (const auto& [_, m] : m_macros)
        distribute_macro_i(m);

    distribute_file_occurences(m_opencode->file_occurences);

    for (const auto& [_, file] : m_files)
        file->process_occurrences();
}

macro_info_ptr lsp_context::get_macro_info(context::id_index macro_name) const
{
    // This function does not respect OPSYN, so we do not use hlasm_context::get_macro_definition
    auto it = m_hlasm_ctx->macros().find(macro_name);
    if (it == m_hlasm_ctx->macros().end())
        return nullptr;
    else
        return m_macros.at(it->second);
}

const file_info* lsp_context::get_file_info(const utils::resource::resource_location& file_loc) const
{
    if (auto it = m_files.find(file_loc); it != m_files.end())
        return it->second.get();
    else
        return nullptr;
}

location lsp_context::definition(const utils::resource::resource_location& document_loc, const position pos) const
{
    auto [occ, macro_scope] = find_occurence_with_scope(document_loc, pos);

    if (!occ)
        return { pos, document_loc };

    if (auto def = find_definition_location(*occ, macro_scope))
        return { def->pos, def->resource_loc };
    return { pos, document_loc };
}

void collect_references(location_list& refs, const symbol_occurence& occ, const file_occurences_t& file_occs)
{
    for (const auto& [file, occs] : file_occs)
    {
        auto file_refs = file_info::find_references(occ, occs);
        for (auto&& ref : file_refs)
            refs.emplace_back(std::move(ref), file);
    }
}

location_list lsp_context::references(const utils::resource::resource_location& document_loc, const position pos) const
{
    location_list result;

    auto [occ, macro_scope] = find_occurence_with_scope(document_loc, pos);

    if (!occ)
        return {};

    if (occ->is_scoped())
    {
        if (macro_scope)
            collect_references(result, *occ, macro_scope->file_occurences_);
        else
            collect_references(result, *occ, m_opencode->file_occurences);
    }
    else
    {
        for (const auto& [_, mac_i] : m_macros)
            collect_references(result, *occ, mac_i->file_occurences_);
        collect_references(result, *occ, m_opencode->file_occurences);
    }

    return result;
}

hover_result lsp_context::hover(const utils::resource::resource_location& document_loc, const position pos) const
{
    auto [occ, macro_scope] = find_occurence_with_scope(document_loc, pos);

    if (!occ)
        return {};

    return find_hover(*occ, macro_scope);
}

size_t constexpr continuation_column = 71;

bool lsp_context::is_continued_line(std::string_view line) const
{
    return line.size() > continuation_column && !isspace((unsigned char)line[continuation_column]);
}

bool lsp_context::should_complete_instr(const text_data_ref_t& text, const position pos) const
{
    bool line_before_continued = pos.line > 0 ? is_continued_line(text.get_line(pos.line - 1)) : false;

    std::string_view line_so_far = text.get_line_beginning_at(pos);

    static const std::regex instruction_regex("^([^*][^*]\\S*\\s+\\S+|\\s+\\S*)");
    return !line_before_continued && std::regex_match(line_so_far.begin(), line_so_far.end(), instruction_regex);
}

completion_list_s lsp_context::completion(const utils::resource::resource_location& document_uri,
    const position pos,
    const char trigger_char,
    completion_trigger_kind trigger_kind) const
{
    auto file_it = m_files.find(document_uri);
    if (file_it == m_files.end())
        return completion_list_s();
    const text_data_ref_t& text = file_it->second->data;

    char last_char =
        (trigger_kind == completion_trigger_kind::trigger_character) ? trigger_char : text.get_character_before(pos);

    if (last_char == '&')
        return complete_var(*file_it->second, pos);
    else if (last_char == '.')
        return complete_seq(*file_it->second, pos);
    else if (should_complete_instr(text, pos))
        return complete_instr(*file_it->second, pos);

    return completion_list_s();
}

completion_list_s lsp_context::complete_var(const file_info& file, position pos) const
{
    auto scope = file.find_scope(pos);


    completion_list_s items;
    const vardef_storage& var_defs = scope ? scope->var_definitions : m_opencode->variable_definitions;
    for (const auto& vardef : var_defs)
    {
        items.emplace_back(
            "&" + *vardef.name, hover_text(vardef), "&" + *vardef.name, "", completion_item_kind::var_sym);
    }

    return items;
}

completion_list_s lsp_context::complete_seq(const file_info& file, position pos) const
{
    auto macro_i = file.find_scope(pos);

    const context::label_storage& seq_syms =
        macro_i ? macro_i->macro_definition->labels : m_hlasm_ctx->current_scope().sequence_symbols;

    completion_list_s items;
    for (const auto& [_, sym] : seq_syms)
    {
        std::string label = "." + *sym->name;
        items.emplace_back(label, "Sequence symbol", label, "", completion_item_kind::seq_sym);
    }
    return items;
}

std::string get_macro_signature(const context::macro_definition& m)
{
    std::stringstream signature;
    if (*m.get_label_param_name() != "")
        signature << "&" << *m.get_label_param_name() << " ";
    signature << *m.id << " ";

    bool first = true;
    const auto& pos_params = m.get_positional_params();
    // First positional parameter is always label, even when empty
    for (size_t i = 1; i < pos_params.size(); ++i)
    {
        if (pos_params[i] == nullptr)
            continue;
        if (!first)
            signature << ",";
        else
            first = false;

        signature << "&" << *pos_params[i]->id;
    }
    for (const auto& param : m.get_keyword_params())
    {
        if (!first)
            signature << ",";
        else
            first = false;
        signature << "&" << *param->id << "=" << param->default_data->get_value();
    }
    return signature.str();
}


bool is_comment(std::string_view line) { return line.substr(0, 1) == "*" || line.substr(0, 2) == ".*"; }


std::string lsp_context::get_macro_documentation(const macro_info& m) const
{
    // Get file, where the macro is defined
    auto it = m_files.find(m.definition_location.resource_loc);
    if (it == m_files.end())
        return "";
    const text_data_ref_t& text = it->second->data;

    // We start at line where the name of the macro is written
    size_t MACRO_line = m.definition_location.pos.line - 1;
    // Skip over MACRO statement
    size_t doc_before_begin_line = MACRO_line - 1;
    // Find the beginning line of documentation written in front of macro definition
    while (doc_before_begin_line != (size_t)-1 && is_comment(text.get_line(doc_before_begin_line)))
        --doc_before_begin_line;
    ++doc_before_begin_line;

    std::string_view doc_before = text.get_range_content({ { doc_before_begin_line, 0 }, { MACRO_line, 0 } });

    // Find the end line of macro definition
    size_t macro_def_end_line = m.definition_location.pos.line;
    while (macro_def_end_line < text.get_number_of_lines() && is_continued_line(text.get_line(macro_def_end_line)))
        ++macro_def_end_line;
    ++macro_def_end_line;

    std::string_view macro_def =
        text.get_range_content({ { m.definition_location.pos.line, 0 }, { macro_def_end_line, 0 } });

    // Find the end line of documentation that comes after the macro definition
    size_t doc_after_end_line = macro_def_end_line;

    while (doc_after_end_line < text.get_number_of_lines() && is_comment(text.get_line(doc_after_end_line)))
        ++doc_after_end_line;

    std::string_view doc_after = text.get_range_content({ { macro_def_end_line, 0 }, { doc_after_end_line, 0 } });

    std::string result = "```\n";
    result.append(macro_def);
    result.append(doc_before);
    result.append(doc_after);
    result.append("\n```\n");

    return result;
}

completion_list_s lsp_context::complete_instr(const file_info& fi, position pos) const
{
    completion_list_s result;

    auto completion_start = fi.data.get_line(pos.line).substr(0, pos.column).rfind(' ');
    if (completion_start == std::string_view::npos)
        completion_start = pos.column;
    else
        completion_start += 2; // after and turn to column

    // Store only instructions from the currently active instruction set
    for (const auto& instr : completion_item_s::m_instruction_completion_items)
    {
        auto id = m_hlasm_ctx->ids().find(instr.label);

        auto it = m_hlasm_ctx->instruction_map().find(id);
        if (it != m_hlasm_ctx->instruction_map().end())
        {
            auto& i = result.emplace_back(instr);
            if (auto space = i.insert_text.find(' '); space != std::string::npos)
            {
                if (completion_start + space < 15)
                    i.insert_text.insert(i.insert_text.begin() + space, 15 - (completion_start + space), ' ');
            }
        }
    }

    for (const auto& [_, macro_i] : m_macros)
    {
        const context::macro_definition& m = *macro_i->macro_definition;

        result.emplace_back(
            *m.id, get_macro_signature(m), *m.id, get_macro_documentation(*macro_i), completion_item_kind::macro);
    }

    return result;
}

template<typename T>
bool files_present(const std::unordered_map<utils::resource::resource_location,
                       file_info_ptr,
                       utils::resource::resource_location_hasher>& files,
    const std::unordered_map<utils::resource::resource_location, T, utils::resource::resource_location_hasher>& scopes)
{
    bool present = true;
    for (const auto& [file, _] : scopes)
        present &= files.find(file) != files.end();
    return present;
}

void lsp_context::distribute_macro_i(macro_info_ptr macro_i)
{
    assert(files_present(m_files, macro_i->file_scopes_));

    for (const auto& [file, slices] : macro_i->file_scopes_)
        m_files[file]->update_slices(file_slice_t::transform_slices(slices, macro_i));

    distribute_file_occurences(macro_i->file_occurences_);
}

void lsp_context::distribute_file_occurences(const file_occurences_t& occurences)
{
    assert(files_present(m_files, occurences));

    for (const auto& [file, occs] : occurences)
        m_files[file]->update_occurences(occs);
}

occurence_scope_t lsp_context::find_occurence_with_scope(
    const utils::resource::resource_location& document_loc, const position pos) const
{
    if (auto file = m_files.find(document_loc); file != m_files.end())
        return file->second->find_occurence_with_scope(pos);
    return std::make_pair(nullptr, nullptr);
}

std::optional<location> lsp_context::find_definition_location(
    const symbol_occurence& occ, macro_info_ptr macro_scope_i) const
{
    switch (occ.kind)
    {
        case lsp::occurence_kind::ORD: {
            auto sym = m_hlasm_ctx->ord_ctx.get_symbol(occ.name);
            if (sym)
                return sym->symbol_location;
            break;
        }
        case lsp::occurence_kind::SEQ: {
            const context::label_storage& seq_syms =
                macro_scope_i ? macro_scope_i->macro_definition->labels : m_hlasm_ctx->current_scope().sequence_symbols;
            if (auto sym = seq_syms.find(occ.name); sym != seq_syms.end())
                return sym->second->symbol_location;
            break;
        }
        case lsp::occurence_kind::VAR: {
            const vardef_storage& var_syms =
                macro_scope_i ? macro_scope_i->var_definitions : m_opencode->variable_definitions;

            auto sym = std::find_if(
                var_syms.begin(), var_syms.end(), [&occ](const auto& var) { return var.name == occ.name; });

            if (sym != var_syms.end())
            {
                if (macro_scope_i)
                    return location(sym->def_position,
                        macro_scope_i->macro_definition->copy_nests[sym->def_location].back().loc.resource_loc);
                return location(sym->def_position, sym->file);
            }
            break;
        }
        case lsp::occurence_kind::INSTR: {
            if (occ.opcode)
            {
                if (auto it = m_macros.find(occ.opcode); it != m_macros.end())
                    return it->second->definition_location;
            }
            break;
        }
        case lsp::occurence_kind::COPY_OP: {
#ifdef __cpp_lib_ranges
            auto copy = std::ranges::find_if(m_files, [&](const auto& f) {
                return f.second->type == file_type::COPY
                    && std::get<context::copy_member_ptr>(f.second->owner)->name == occ.name;
            });
#else
            auto copy = std::find_if(m_files.begin(), m_files.end(), [&](const auto& f) {
                return f.second->type == file_type::COPY
                    && std::get<context::copy_member_ptr>(f.second->owner)->name == occ.name;
            });
#endif

            if (copy != m_files.end())
                return std::get<context::copy_member_ptr>(copy->second->owner)->definition_location;
            break;
        }
        default:
            break;
    }
    return std::nullopt;
}

hover_result lsp_context::find_hover(const symbol_occurence& occ, macro_info_ptr macro_scope_i) const
{
    switch (occ.kind)
    {
        case lsp::occurence_kind::ORD: {
            auto sym = m_hlasm_ctx->ord_ctx.get_symbol(occ.name);
            if (sym)
                return hover_text(*sym);
            break;
        }
        case lsp::occurence_kind::SEQ:
            return "Sequence symbol";

        case lsp::occurence_kind::VAR: {
            const vardef_storage& var_syms =
                macro_scope_i ? macro_scope_i->var_definitions : m_opencode->variable_definitions;

            auto sym =
                std::find_if(var_syms.begin(), var_syms.end(), [&](const auto& var) { return var.name == occ.name; });
            if (sym != var_syms.end())
                return hover_text(*sym);
            break;
        }
        case lsp::occurence_kind::INSTR: {
            if (occ.opcode)
            {
                auto it = m_macros.find(occ.opcode);
                assert(it != m_macros.end());
                return get_macro_documentation(*it->second);
            }
            else
            {
                auto it = completion_item_s::m_instruction_completion_items.find(*occ.name);
                if (it == completion_item_s::m_instruction_completion_items.end())
                    return "";
                return it->detail + "\n\n" + it->documentation;
            }
        }
        case lsp::occurence_kind::COPY_OP:
            return "";

        default:
            break;
    }
    return {};
}

} // namespace hlasm_plugin::parser_library::lsp
