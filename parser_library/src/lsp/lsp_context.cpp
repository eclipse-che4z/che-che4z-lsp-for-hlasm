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
#include "item_convertors.h"
#include "utils/similar.h"

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
};

const std::unordered_map<occurence_kind, document_symbol_kind> document_symbol_item_kind_mapping_macro {
    { occurence_kind::VAR, document_symbol_kind::VAR }, { occurence_kind::SEQ, document_symbol_kind::SEQ }
};
} // namespace

std::string lsp_context::find_macro_copy_id(const std::vector<context::processing_frame>& stack, unsigned long i) const
{
    assert(i != 0);
    assert(i < stack.size());
    return stack[i].member_name.empty() ? stack[i].resource_loc->get_uri() : stack[i].member_name.to_string();
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
            result.emplace_back(var.name.to_string(), document_symbol_kind::VAR, r.value_or(range(var.def_position)));
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
            result.emplace_back(
                name.to_string(), document_symbol_kind::SEQ, r.value_or(range(seq->symbol_location.pos)));
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
    return aux == nullptr || aux->name.to_string_view() != id.to_string_view();
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
                occ.name.to_string(),
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

    if (name.empty())
        return occurences_by_name;

    struct
    {
        bool operator()(const symbol_occurence* l, context::id_index r) const { return l->name < r; }
        bool operator()(context::id_index l, const symbol_occurence* r) const { return l < r->name; }
    } search_predicate;

    auto [low, high] = std::equal_range(occurences_by_name.begin(), occurences_by_name.end(), name, search_predicate);

    return std::span<const symbol_occurence* const>(low, high);
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
            for (std::optional<context::id_index> last;
                 const auto* new_occ : get_occurences_by_name(*info, context::id_index(), cache))
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
        document_symbol_item_s sym_item(sym_name.to_string(), kind, copy_occ.occurence_range);
        for (auto& item : modified)
        {
            if (item.name == copy_occ.name.to_string_view()
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
            modified.emplace_back(copy_occ.name.to_string(),
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
    i_find->children.emplace_back(id.to_string(), kind, i_find->symbol_range, std::move(children));
    --limit;
}

void lsp_context::document_symbol_opencode_ord_symbol(document_symbol_list_s& result, long long& limit) const
{
    const auto& symbol_list = m_hlasm_ctx->ord_ctx.symbols();
    std::map<const context::section*, document_symbol_list_s> children_of_sects;
    for (const auto& [id, sym_var] : symbol_list)
    {
        const auto* sym = std::get_if<context::symbol>(&sym_var);
        if (sym && sym->attributes().origin() == context::symbol_origin::SECT)
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
        if (sym.attributes().origin() == context::symbol_origin::SECT)
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
                result.emplace_back(id.to_string(),
                    document_symbol_item_kind_mapping_symbol.at(sym.attributes().origin()),
                    range(sym.symbol_location().pos));
                --limit;
            }
            else
            {
                document_symbol_symbol(result,
                    document_symbol_list_s {},
                    id,
                    sym_stack,
                    document_symbol_item_kind_mapping_symbol.at(sym.attributes().origin()),
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
                children.emplace_back(id.to_string(),
                    document_symbol_item_kind_mapping_symbol.at(sym.attributes().origin()),
                    range(sym_stack[0].pos));
                --limit;
            }
            else
            {
                document_symbol_symbol(children,
                    document_symbol_list_s {},
                    id,
                    sym_stack,
                    document_symbol_item_kind_mapping_symbol.at(sym.attributes().origin()),
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
            result.emplace_back(sect->name.to_string(),
                document_symbol_item_kind_mapping_section.at(sect->kind),
                range(sym.symbol_location().pos),
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
        if (location == name_to_location_cache.end() || location->second.empty())
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
        name_to_location.insert_or_assign(def->id.to_string_view(), info->definition_location.resource_loc);
    for (const auto& [def, info] : m_hlasm_ctx->copy_members())
        name_to_location.insert_or_assign(info->name.to_string_view(), info->definition_location.resource_loc);

    document_symbol_opencode_ord_symbol(result, limit);
    document_symbol_opencode_var_seq_symbol_aux(result, name_to_location, limit, cache);

    for (const auto& sym : m_opencode->variable_definitions)
    {
        if (limit <= 0)
            break;
        if (!belongs_to_copyfile(document_loc, sym.def_position, sym.name))
        {
            result.emplace_back(sym.name.to_string(), document_symbol_kind::VAR, range(sym.def_position));
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

void lsp_context::add_copy(context::copy_member_ptr copy, text_data_view text_data)
{
    add_file(file_info(std::move(copy), std::move(text_data)));
}

void lsp_context::add_macro(macro_info_ptr macro_i, text_data_view text_data)
{
    if (macro_i->external)
        add_file(file_info(macro_i->macro_definition, std::move(text_data)));

    m_macros[macro_i->macro_definition] = macro_i;
}

void lsp_context::add_opencode(opencode_info_ptr opencode_i, text_data_view text_data)
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

macro_info_ptr lsp_context::get_macro_info(context::id_index macro_name, context::opcode_generation gen) const
{
    // This function does not respect OPSYN, so we do not use hlasm_context::get_macro_definition
    if (auto it = m_hlasm_ctx->find_macro(macro_name, gen); !it)
        return nullptr;
    else
        return m_macros.at(*it);
}

const file_info* lsp_context::get_file_info(const utils::resource::resource_location& file_loc) const
{
    if (auto it = m_files.find(file_loc); it != m_files.end())
        return it->second.get();
    else
        return nullptr;
}

location lsp_context::definition(const utils::resource::resource_location& document_loc, position pos) const
{
    auto [occ, macro_scope] = find_occurence_with_scope(document_loc, pos);

    if (!occ)
        return { pos, document_loc };

    if (auto def = find_definition_location(*occ, macro_scope, document_loc, pos))
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

location_list lsp_context::references(const utils::resource::resource_location& document_loc, position pos) const
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

std::string lsp_context::hover(const utils::resource::resource_location& document_loc, position pos) const
{
    auto [occ, macro_scope] = find_occurence_with_scope(document_loc, pos);

    if (!occ)
        return {};

    return find_hover(*occ, macro_scope);
}

bool lsp_context::should_complete_instr(const text_data_view& text, position pos) const
{
    bool line_before_continued = pos.line > 0 ? is_continued_line(text.get_line(pos.line - 1)) : false;

    std::string_view line_so_far = text.get_line_beginning_at(pos);

    static const std::regex instruction_regex("^([^*][^*]\\S*\\s+\\S+|\\s+\\S*)");
    return !line_before_continued && std::regex_match(line_so_far.begin(), line_so_far.end(), instruction_regex);
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

    return {};
}

completion_list_source lsp_context::complete_var(const file_info& file, position pos) const
{
    auto scope = file.find_scope(pos);

    return scope ? &scope->var_definitions : &m_opencode->variable_definitions;
}

completion_list_source lsp_context::complete_seq(const file_info& file, position pos) const
{
    auto macro_i = file.find_scope(pos);

    return macro_i ? &macro_i->macro_definition->labels : &m_hlasm_ctx->current_scope().sequence_symbols;
}

completion_list_source lsp_context::complete_instr(const file_info& fi, position pos) const
{
    completion_list_source result(std::in_place_type<completion_list_instructions>);

    auto& value = std::get<completion_list_instructions>(result);

    value.completed_text = fi.data.get_line(pos.line).substr(0, pos.column);
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
    const utils::resource::resource_location& document_loc, position pos) const
{
    if (auto file = m_files.find(document_loc); file != m_files.end())
        return file->second->find_occurence_with_scope(pos);
    return std::make_pair(nullptr, nullptr);
}

location lsp_context::find_symbol_definition_location(
    const context::symbol& sym, const utils::resource::resource_location& document_loc, position pos) const
{
    std::pair<occurence_scope_t, context::processing_stack_t> top_reference;
    const auto& [top_scope, top_stack] = top_reference;

    auto stack = sym.proc_stack();
    while (!stack.empty())
    {
        auto frame = stack.frame();

        if (*frame.resource_loc == document_loc && frame.pos.line == pos.line)
        {
            top_reference = {};
            break;
        }

        auto scope = find_occurence_with_scope(*frame.resource_loc, position(frame.pos.line, 0));

        if (scope.first && scope.first->kind == lsp::occurence_kind::ORD && scope.first->name == sym.name())
            top_reference = { scope, stack };

        stack = stack.parent();
    }
    if (top_scope.first)
        return location(top_scope.first->occurence_range.start, *top_stack.frame().resource_loc);
    else
        return sym.symbol_location();
}

std::optional<location> lsp_context::find_definition_location(const symbol_occurence& occ,
    macro_info_ptr macro_scope_i,
    const utils::resource::resource_location& document_loc,
    position pos) const
{
    switch (occ.kind)
    {
        case lsp::occurence_kind::ORD: {
            if (auto sym = m_hlasm_ctx->ord_ctx.get_symbol(occ.name))
                return find_symbol_definition_location(*sym, document_loc, pos);
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

std::string lsp_context::find_hover(const symbol_occurence& occ, macro_info_ptr macro_scope_i) const
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

                // Get file, where the macro is defined
                auto mit = m_files.find(it->second->definition_location.resource_loc);
                if (mit == m_files.end())
                    return "";

                return get_macro_documentation(mit->second->data, it->second->definition_location.pos.line);
            }
            else
            {
                auto it = completion_item_s::m_instruction_completion_items.find(occ.name.to_string_view());
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
