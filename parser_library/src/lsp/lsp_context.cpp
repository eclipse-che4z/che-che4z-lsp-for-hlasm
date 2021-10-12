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
#include <regex>
#include <sstream>
#include <string_view>
#include <unordered_map>

#include "context/instruction.h"
#include "ebcdic_encoding.h"

namespace hlasm_plugin::parser_library::lsp {
namespace {
hover_result hover_text(const context::symbol& sym)
{
    if (sym.value().value_kind() == context::symbol_value_kind::UNDEF)
        return "";
    std::string markdown = "";

    if (sym.value().value_kind() == context::symbol_value_kind::ABS)
    {
        markdown.append(std::to_string(sym.value().get_abs()));
        markdown.append("\n\n---\n\nAbsolute Symbol\n\n---\n\n");
    }
    else if (sym.value().value_kind() == context::symbol_value_kind::RELOC)
    {
        markdown.append(sym.value().get_reloc().to_string()); // move to_string method from that class to this class
        markdown.append("\n\n---\n\nRelocatable Symbol\n\n---\n\n");
    }

    const auto& attrs = sym.attributes();
    if (attrs.is_defined(context::data_attr_kind::L))
        markdown.append("L: " + std::to_string(attrs.length()) + "  \n");
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

std::string lsp_context::find_macro_copy_id(const context::processing_stack_t& stack, unsigned long i) const
{
    assert(i != 0);
    assert(i < stack.size());
    return stack[i].member_name == context::id_storage::empty_id ? stack[i].proc_location.file : *stack[i].member_name;
}

document_symbol_list_s lsp_context::document_symbol_macro(const std::string& document_uri) const
{
    auto copy_occs = copy_occurences(document_uri);
    document_symbol_list_s result;
    for (const auto& [def, info] : macros_)
    {
        if (info->definition_location.file == document_uri)
        {
            for (const auto& var : info->var_definitions)
            {
                if (belongs_to_copyfile(document_uri, var.def_position, var.name))
                {
                    modify_with_copy(result, var.name, copy_occs, document_symbol_kind::VAR);
                }
                else
                {
                    result.emplace_back(document_symbol_item_s {
                        *var.name, document_symbol_kind::VAR, { var.def_position, var.def_position } });
                }
            }
            for (const auto& [name, seq] : def->labels)
            {
                if (belongs_to_copyfile(document_uri, seq->symbol_location.pos, name))
                {
                    modify_with_copy(result, name, copy_occs, document_symbol_kind::SEQ);
                }
                else
                {
                    result.emplace_back(document_symbol_item_s {
                        *name, document_symbol_kind::SEQ, { seq->symbol_location.pos, seq->symbol_location.pos } });
                }
            }
            return result;
        }
    }
    return result;
}

document_symbol_list_s lsp_context::document_symbol_macro(const std::string& document_uri, const range& r) const
{
    auto copy_occs = copy_occurences(document_uri);
    document_symbol_list_s result;
    for (const auto& [def, info] : macros_)
    {
        if (def->definition_location.file == document_uri)
        {
            for (const auto& var : info->var_definitions)
            {
                if (!belongs_to_copyfile(document_uri, var.def_position, var.name))
                {
                    result.emplace_back(document_symbol_item_s {
                        *var.name, document_symbol_kind::VAR, { var.def_position, var.def_position } });
                    result.back().symbol_range = r;
                    result.back().symbol_selection_range = r;
                }
            }
            for (const auto& [name, seq] : def->labels)
            {
                if (!belongs_to_copyfile(document_uri, seq->symbol_location.pos, name))
                {
                    result.emplace_back(document_symbol_item_s {
                        *name, document_symbol_kind::SEQ, { seq->symbol_location.pos, seq->symbol_location.pos } });
                    result.back().symbol_range = r;
                    result.back().symbol_selection_range = r;
                }
            }
            return result;
        }
    }
    return result;
}

bool lsp_context::belongs_to_copyfile(const std::string& document_uri, position pos, const context::id_index& id) const
{
    const auto& aux = find_occurence_with_scope(document_uri, pos).first;
    return aux == nullptr || *aux->name != *id;
}

document_symbol_list_s lsp_context::document_symbol_copy(
    const std::vector<symbol_occurence>& occurence_list, const std::string& document_uri) const
{
    document_symbol_list_s result;
    for (const auto& occ : occurence_list)
    {
        if (occ.kind == occurence_kind::VAR || occ.kind == occurence_kind::SEQ)
        {
            position aux = definition(document_uri, occ.occurence_range.start).pos;
            document_symbol_item_s item = { *occ.name,
                document_symbol_item_kind_mapping_macro.at(occ.kind),
                { aux, { aux.line, aux.column + occ.occurence_range.end.column - occ.occurence_range.start.column } } };
            if (std::find(result.begin(), result.end(), item) == result.end())
            {
                result.push_back(item);
            }
        }
    }
    return result;
}

document_symbol_list_s lsp_context::document_symbol_copy(
    const std::vector<symbol_occurence>& occurence_list, const std::string& document_uri, const range& r) const
{
    document_symbol_list_s result = document_symbol_copy(occurence_list, document_uri);
    for (auto& item : result)
    {
        item.symbol_range = r;
        item.symbol_selection_range = r;
    }
    return result;
}

std::vector<std::pair<symbol_occurence, std::vector<context::id_index>>> lsp_context::copy_occurences(
    const std::string& document_uri) const
{
    const auto& file = files_.find(document_uri);
    std::vector<std::pair<symbol_occurence, std::vector<context::id_index>>> copy_occurences;
    for (const auto& [uri, info] : files_)
    {
        if (info->type == file_type::COPY)
        {
            const context::id_index name = std::get<context::copy_member_ptr>(info->owner)->name;
            for (const auto& occ : file->second->get_occurences())
            {
                if (occ.name == name)
                {
                    std::vector<context::id_index> occurences;
                    for (const auto& new_occ : info->get_occurences())
                    {
                        if (new_occ.kind == occurence_kind::VAR
                            || new_occ.kind == occurence_kind::SEQ
                                && std::find(occurences.begin(), occurences.end(), new_occ.name) == occurences.end())
                        {
                            occurences.push_back(new_occ.name);
                        }
                    }
                    copy_occurences.emplace_back(std::pair { occ, occurences });
                }
            }
        }
    }
    return copy_occurences;
}

void lsp_context::modify_with_copy(document_symbol_list_s& modified,
    const context::id_index& sym_name,
    const std::vector<std::pair<symbol_occurence, std::vector<context::id_index>>>& copy_occs,
    const document_symbol_kind kind) const
{
    if (!copy_occs.empty())
    {
        for (const auto& [copy_occ, occs] : copy_occs)
        {
            if (std::find(occs.begin(), occs.end(), sym_name) != occs.end())
            {
                bool have_already = false;
                auto sym_item = document_symbol_item_s { *sym_name, kind, copy_occ.occurence_range };
                for (auto& item : modified)
                {
                    if (item.name == *copy_occ.name
                        && std::find(item.children.begin(), item.children.end(), sym_item) == item.children.end())
                    {
                        item.children.push_back(sym_item);
                        have_already = true;
                        break;
                    }
                }
                if (!have_already)
                {
                    modified.emplace_back(document_symbol_item_s { *copy_occ.name,
                        document_symbol_kind::MACRO,
                        copy_occ.occurence_range,
                        document_symbol_list_s { sym_item } });
                }
            }
        }
    }
}

bool do_not_need_nodes(
    const context::processing_stack_t& sym, const context::processing_stack_t& sect_sym, unsigned long& i)
{
    if (sym.size() == 1)
    {
        return true;
    }
    const auto size = sym.size() < sect_sym.size() ? sym.size() : sect_sym.size();
    while (i < size)
    {
        if (sym[i].proc_location.file != sect_sym[i].proc_location.file
            || sym[i].proc_location.pos != sect_sym[i].proc_location.pos)
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
    const document_symbol_list_s& children,
    const context::id_index& id,
    const context::symbol& sym,
    document_symbol_kind kind,
    unsigned long i) const
{
    document_symbol_item_s aux = { find_macro_copy_id(sym.proc_stack(), i),
        document_symbol_kind::MACRO,
        { sym.proc_stack()[0].proc_location.pos, sym.proc_stack()[0].proc_location.pos } };
    auto i_find = document_symbol_no_children_find(modified.begin(), modified.end(), aux);
    if (i_find == modified.end())
    {
        modified.push_back(aux);
        i_find = modified.end() - 1;
    }
    i++;
    while (i < sym.proc_stack().size())
    {
        aux.name = find_macro_copy_id(sym.proc_stack(), i);
        document_symbol_list_s* aux_list = &i_find->children;
        i_find = document_symbol_no_children_find(aux_list->begin(), aux_list->end(), aux);
        if (i_find == aux_list->end())
        {
            aux_list->push_back(aux);
            i_find = aux_list->end() - 1;
        }
        i++;
    }
    document_symbol_list_s* aux_list = &i_find->children;
    aux_list->emplace_back(document_symbol_item_s { *id, kind, i_find->symbol_range, children });
}

document_symbol_list_s lsp_context::document_symbol_opencode_ord_symbol() const
{
    document_symbol_list_s result;
    const auto& symbol_list = hlasm_ctx_->ord_ctx.symbols();
    std::map<const context::section*, document_symbol_list_s> children_of_sects;
    for (const auto& [id, sym] : symbol_list)
    {
        if (sym.attributes().origin == context::symbol_origin::SECT)
        {
            auto sect = hlasm_ctx_->ord_ctx.get_section(id);
            if (sect == nullptr)
                continue;
            children_of_sects.try_emplace(sect, document_symbol_list_s {});
        }
    }

    for (const auto& [id, sym] : symbol_list)
    {
        if (sym.attributes().origin != context::symbol_origin::SECT)
        {
            const auto& sect = sym.value().value_kind() == context::symbol_value_kind::RELOC
                ? sym.value().get_reloc().bases()[0].first.owner
                : nullptr;
            if (children_of_sects.find(sect) == children_of_sects.end())
            {
                if (sym.proc_stack().size() == 1)
                {
                    result.emplace_back(document_symbol_item_s { *id,
                        document_symbol_item_kind_mapping_symbol.at(sym.attributes().origin),
                        { sym.symbol_location.pos, sym.symbol_location.pos } });
                    continue;
                }
                document_symbol_symbol(result,
                    document_symbol_list_s {},
                    id,
                    sym,
                    document_symbol_item_kind_mapping_symbol.at(sym.attributes().origin),
                    1);
                continue;
            }

            const auto& sect_sym = hlasm_ctx_->ord_ctx.get_symbol(sect->name);
            auto& children = children_of_sects.find(sect)->second;
            unsigned long i = 1;
            if (do_not_need_nodes(sym.proc_stack(), sect_sym->proc_stack(), i))
            {
                children.emplace_back(document_symbol_item_s { *id,
                    document_symbol_item_kind_mapping_symbol.at(sym.attributes().origin),
                    { sym.proc_stack()[0].proc_location.pos, sym.proc_stack()[0].proc_location.pos } });
                continue;
            }
            document_symbol_symbol(children,
                document_symbol_list_s {},
                id,
                sym,
                document_symbol_item_kind_mapping_symbol.at(sym.attributes().origin),
                i);
        }
    }

    for (const auto& [sect, children] : children_of_sects)
    {
        const auto& sym = *hlasm_ctx_->ord_ctx.get_symbol(sect->name);
        if (sym.proc_stack().size() == 1)
        {
            result.emplace_back(document_symbol_item_s { *sect->name,
                document_symbol_item_kind_mapping_section.at(sect->kind),
                { sym.symbol_location.pos, sym.symbol_location.pos },
                children });
            continue;
        }
        document_symbol_symbol(
            result, children, sect->name, sym, document_symbol_item_kind_mapping_section.at(sect->kind), 1);
    }

    return result;
}

void lsp_context::document_symbol_opencode_var_seq_symbol_aux(document_symbol_list_s& result) const
{
    for (auto& item : result)
    {
        if (item.kind == document_symbol_kind::MACRO)
        {
            std::string document_uri = "";
            for (const auto& [def, info] : macros_)
            {
                if (*def->id == item.name)
                {
                    document_uri = info->definition_location.file;
                    break;
                }
            }
            for (const auto& [def, info] : hlasm_ctx_->copy_members())
            {
                if (*info->name == item.name)
                {
                    document_uri = info->definition_location.file;
                    break;
                }
            }
            if (document_uri == "")
            {
                return;
            }
            const auto& file = files_.find(document_uri);
            if (file != files_.end())
            {
                if (file->second->type == file_type::MACRO)
                {
                    auto tmp = document_symbol_macro(file->first, item.symbol_range);
                    item.children.insert(
                        item.children.end(), std::make_move_iterator(tmp.begin()), std::make_move_iterator(tmp.end()));
                }
                if (file->second->type == file_type::COPY)
                {
                    auto tmp = document_symbol_copy(file->second->get_occurences(), file->first, item.symbol_range);
                    item.children.insert(
                        item.children.end(), std::make_move_iterator(tmp.begin()), std::make_move_iterator(tmp.end()));
                }
            }
        }
        document_symbol_opencode_var_seq_symbol_aux(item.children);
    }
}

void lsp_context::document_symbol_opencode_var_seq_symbol(
    const std::string& document_uri, document_symbol_list_s& result) const
{
    document_symbol_opencode_var_seq_symbol_aux(result);

    auto copy_occs = copy_occurences(document_uri);
    for (const auto& sym : opencode_->variable_definitions)
    {
        if (!belongs_to_copyfile(document_uri, sym.def_position, sym.name))
        {
            result.emplace_back(document_symbol_item_s {
                *sym.name, document_symbol_kind::VAR, { sym.def_position, sym.def_position } });
        }
    }
}

document_symbol_list_s lsp_context::document_symbol(const std::string& document_uri, long long limit) const
{
    const auto& file = files_.find(document_uri);
    if (file == files_.end())
    {
        return document_symbol_list_s {};
    }
    switch (file->second->type)
    {
        case file_type::MACRO:
            return document_symbol_macro(document_uri);
        case file_type::COPY:
            return document_symbol_copy(file->second->get_occurences(), document_uri);
        default:
            document_symbol_list_s result = document_symbol_opencode_ord_symbol();
            document_symbol_opencode_var_seq_symbol(document_uri, result);
            return result;
    }
}



void lsp_context::add_file(file_info file_i)
{
    std::string name = file_i.name;
    files_.try_emplace(std::move(name), std::make_unique<file_info>(std::move(file_i)));
}

lsp_context::lsp_context(std::shared_ptr<context::hlasm_context> h_ctx)
    : hlasm_ctx_(std::move(h_ctx))
{}

void lsp_context::add_copy(context::copy_member_ptr copy, text_data_ref_t text_data)
{
    add_file(file_info(std::move(copy), std::move(text_data)));
}

void lsp_context::add_macro(macro_info_ptr macro_i, text_data_ref_t text_data)
{
    if (macro_i->external)
        add_file(file_info(macro_i->macro_definition, std::move(text_data)));

    macros_[macro_i->macro_definition] = macro_i;
}

void lsp_context::add_opencode(opencode_info_ptr opencode_i, text_data_ref_t text_data)
{
    opencode_ = std::move(opencode_i);
    add_file(file_info(hlasm_ctx_->opencode_file_name(), std::move(text_data)));

    // distribute all occurences as all files are present
    for (const auto& [_, m] : macros_)
        distribute_macro_i(m);

    distribute_file_occurences(opencode_->file_occurences);
}

macro_info_ptr lsp_context::get_macro_info(context::id_index macro_name) const
{
    // This function does not respect OPSYN, so we do not use hlasm_context::get_macro_definition
    auto it = hlasm_ctx_->macros().find(macro_name);
    if (it == hlasm_ctx_->macros().end())
        return nullptr;
    else
        return macros_.at(it->second);
}

const file_info* lsp_context::get_file_info(const std::string& file_name) const
{
    if (auto it = files_.find(file_name); it != files_.end())
        return it->second.get();
    else
        return nullptr;
}

location lsp_context::definition(const std::string& document_uri, const position pos) const
{
    auto [occ, macro_scope] = find_occurence_with_scope(document_uri, pos);

    if (!occ)
        return { pos, document_uri };

    if (auto def = find_definition_location(*occ, macro_scope))
        return { def->pos, def->file };
    return { pos, document_uri };
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

location_list lsp_context::references(const std::string& document_uri, const position pos) const
{
    location_list result;

    auto [occ, macro_scope] = find_occurence_with_scope(document_uri, pos);

    if (!occ)
        return {};

    if (occ->is_scoped())
    {
        if (macro_scope)
            collect_references(result, *occ, macro_scope->file_occurences_);
        else
            collect_references(result, *occ, opencode_->file_occurences);
    }
    else
    {
        for (const auto& [_, mac_i] : macros_)
            collect_references(result, *occ, mac_i->file_occurences_);
        collect_references(result, *occ, opencode_->file_occurences);
    }

    return result;
}

hover_result lsp_context::hover(const std::string& document_uri, const position pos) const
{
    auto [occ, macro_scope] = find_occurence_with_scope(document_uri, pos);

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

completion_list_s lsp_context::completion(const std::string& document_uri,
    const position pos,
    const char trigger_char,
    completion_trigger_kind trigger_kind) const
{
    auto file_it = files_.find(document_uri);
    if (file_it == files_.end())
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
    const vardef_storage& var_defs = scope ? scope->var_definitions : opencode_->variable_definitions;
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
        macro_i ? macro_i->macro_definition->labels : hlasm_ctx_->current_scope().sequence_symbols;

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
    auto it = files_.find(m.definition_location.file);
    if (it == files_.end())
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

completion_list_s lsp_context::complete_instr(const file_info&, position) const
{
    completion_list_s result = completion_item_s::instruction_completion_items_;

    for (const auto& [_, macro_i] : macros_)
    {
        const context::macro_definition& m = *macro_i->macro_definition;

        result.emplace_back(
            *m.id, get_macro_signature(m), *m.id, get_macro_documentation(*macro_i), completion_item_kind::macro);
    }

    return result;
}


template<typename T>
bool files_present(
    const std::unordered_map<std::string, file_info_ptr>& files, const std::unordered_map<std::string, T>& scopes)
{
    bool present = true;
    for (const auto& [file, _] : scopes)
        present &= files.find(file) != files.end();
    return present;
}

void lsp_context::distribute_macro_i(macro_info_ptr macro_i)
{
    assert(files_present(files_, macro_i->file_scopes_));

    for (const auto& [file, slices] : macro_i->file_scopes_)
        files_[file]->update_slices(file_slice_t::transform_slices(slices, macro_i));

    distribute_file_occurences(macro_i->file_occurences_);
}

void lsp_context::distribute_file_occurences(const file_occurences_t& occurences)
{
    assert(files_present(files_, occurences));

    for (const auto& [file, occs] : occurences)
        files_[file]->update_occurences(occs);
}

occurence_scope_t lsp_context::find_occurence_with_scope(const std::string& document_uri, const position pos) const
{
    if (auto file = files_.find(document_uri); file != files_.end())
        return file->second->find_occurence_with_scope(pos);
    return std::make_pair(nullptr, nullptr);
}

std::optional<location> lsp_context::find_definition_location(
    const symbol_occurence& occ, macro_info_ptr macro_scope_i) const
{
    switch (occ.kind)
    {
        case lsp::occurence_kind::ORD: {
            auto sym = hlasm_ctx_->ord_ctx.get_symbol(occ.name);
            if (sym)
                return sym->symbol_location;
            break;
        }
        case lsp::occurence_kind::SEQ: {
            const context::label_storage& seq_syms =
                macro_scope_i ? macro_scope_i->macro_definition->labels : hlasm_ctx_->current_scope().sequence_symbols;
            if (auto sym = seq_syms.find(occ.name); sym != seq_syms.end())
                return sym->second->symbol_location;
            break;
        }
        case lsp::occurence_kind::VAR: {
            const vardef_storage& var_syms =
                macro_scope_i ? macro_scope_i->var_definitions : opencode_->variable_definitions;

            auto sym = std::find_if(
                var_syms.begin(), var_syms.end(), [&occ](const auto& var) { return var.name == occ.name; });

            if (sym != var_syms.end())
            {
                if (macro_scope_i)
                    return location(sym->def_position,
                        macro_scope_i->macro_definition->copy_nests[sym->def_location].back().loc.file);
                return location(sym->def_position, sym->file);
            }
            break;
        }
        case lsp::occurence_kind::INSTR: {
            if (occ.opcode)
            {
                if (auto it = macros_.find(occ.opcode); it != macros_.end())
                    return it->second->definition_location;
            }
            break;
        }
        case lsp::occurence_kind::COPY_OP: {
            auto copy = std::find_if(files_.begin(), files_.end(), [&](const auto& f) {
                return f.second->type == file_type::COPY
                    && std::get<context::copy_member_ptr>(f.second->owner)->name == occ.name;
            });
            if (copy != files_.end())
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
            auto sym = hlasm_ctx_->ord_ctx.get_symbol(occ.name);
            if (sym)
                return hover_text(*sym);
            break;
        }
        case lsp::occurence_kind::SEQ:
            return "Sequence symbol";

        case lsp::occurence_kind::VAR: {
            const vardef_storage& var_syms =
                macro_scope_i ? macro_scope_i->var_definitions : opencode_->variable_definitions;

            auto sym =
                std::find_if(var_syms.begin(), var_syms.end(), [&](const auto& var) { return var.name == occ.name; });
            if (sym != var_syms.end())
                return hover_text(*sym);
            break;
        }
        case lsp::occurence_kind::INSTR: {
            if (occ.opcode)
            {
                auto it = macros_.find(occ.opcode);
                assert(it != macros_.end());
                return get_macro_documentation(*it->second);
            }
            else
            {
                auto it = std::find_if(completion_item_s::instruction_completion_items_.begin(),
                    completion_item_s::instruction_completion_items_.end(),
                    [&occ](const auto& item) { return item.label == *occ.name; });
                if (it == completion_item_s::instruction_completion_items_.end())
                    return "";
                return it->detail + "  \n" + it->documentation;
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
