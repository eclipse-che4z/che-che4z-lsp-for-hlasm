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

#include "ebcdic_encoding.h"

namespace hlasm_plugin::parser_library::lsp {

void lsp_context::add_file(file_info file_i)
{
    std::string name = file_i.name;
    files_.try_emplace(std::move(name), std::make_unique<file_info>(std::move(file_i)));
}

void lsp_context::add_copy(context::copy_member_ptr copy) { add_file(file_info(std::move(copy))); }

void lsp_context::add_macro(macro_info_ptr macro_i)
{
    if (macro_i->external)
        add_file(file_info(macro_i->macro_definition));
    distribute_macro_i(macro_i);
}

void lsp_context::add_opencode(opencode_info_ptr opencode_i)
{
    opencode_ = std::move(opencode_i);
    distribute_file_occurences(opencode_->file_occurences);
}

void lsp_context::update_file_info(const std::string& name, const occurence_storage& occurences)
{
    assert(files_.find(name) != files_.end());
    files_[name]->update_occurences(occurences);
}

position_uri lsp_context::definition(const char* document_uri, const position pos)
{
    auto [occ, macro_scope] = find_occurence_with_scope(document_uri, pos);

    if (!occ)
        return { pos, document_uri };

    auto def = find_definition_location(*occ, macro_scope);

    if (def)
        return { def->pos, def->file };
    return { pos, document_uri };
}

void collect_references(position_uris& refs, const symbol_occurence& occ, const file_occurences_t& file_occs)
{
    for (const auto& [file, occs] : file_occs)
    {
        auto file_refs = file_info::find_references(occ, occs);
        for (auto&& ref : file_refs)
            refs.push_back({ std::move(ref), file });
    }
}

position_uris lsp_context::references(const char* document_uri, const position pos)
{
    position_uris result;
    result.push_back({ pos, document_uri });

    auto [occ, macro_scope] = find_occurence_with_scope(document_uri, pos);

    if (!occ)
        return result;

    auto def = find_definition_location(*occ, macro_scope);

    if (!def)
        return result;
    result.push_back({ def->pos, def->file });

    std::vector<location> scoped_result;

    if (occ->is_scoped())
    {
        if (macro_scope)
            collect_references(result, *occ, macro_scope->file_occurences_);
        else
            collect_references(result, *occ, opencode_->file_occurences);
    }
    else
    {
        for (const auto& mac_i : macros_)
            collect_references(result, *occ, mac_i.second->file_occurences_);
        collect_references(result, *occ, opencode_->file_occurences);
    }

    return result;
}

string_array lsp_context::hover(const char* document_uri, const position pos)
{
    string_array result;
    auto [occ, macro_scope] = find_occurence_with_scope(document_uri, pos);

    if (!occ)
        return result;

    return find_hover(*occ, macro_scope);
}

completion_list lsp_context::completion(
    const char* document_uri, const position pos, const char trigger_char, int trigger_kind)
{}

template<typename T>
bool files_present(
    const std::unordered_map<std::string, file_info_ptr>& files, const std::unordered_map<std::string, T>& scopes)
{
    bool present = true;
    for (const auto& [file, _] : scopes)
        present |= files.find(file) != files.end();
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

occurence_scope_t lsp_context::find_occurence_with_scope(const char* document_uri, const position pos) const
{
    if (auto file = files_.find(document_uri); file != files_.end())
        return file->second->find_occurence_with_scope(pos);
    return std::make_pair(nullptr, nullptr);
}


template<lsp::occurence_kind kind, typename T>
T find_definition(const symbol_occurence& occ, macro_info_ptr macro_i, const opencode_info& opencode)
{
    if constexpr (kind == lsp::occurence_kind::ORD)
    {
        return opencode.hlasm_ctx.ord_ctx.get_symbol(occ.name);
    }
    if constexpr (kind == lsp::occurence_kind::SEQ)
    {
        const context::sequence_symbol* retval = nullptr;
        if (macro_i)
        {
            auto sym = macro_i->macro_definition->labels.find(occ.name);
            if (sym != macro_i->macro_definition->labels.end())
                retval = sym->second.get();
        }
        else
        {
            auto sym = opencode.hlasm_ctx.current_scope().sequence_symbols.find(occ.name);
            if (sym != opencode.hlasm_ctx.current_scope().sequence_symbols.end())
                retval = sym->second.get();
        }
        return retval;
    }
    if constexpr (kind == lsp::occurence_kind::VAR)
    {
        const variable_symbol_definition* retval = nullptr;
        if (macro_i)
        {
            auto sym = std::find_if(macro_i->var_definitions.begin(),
                macro_i->var_definitions.end(),
                [&](const auto& var) { return var.name == occ.name; });

            if (sym != macro_i->var_definitions.end())
                retval = &*sym;
        }
        else
        {
            auto sym = std::find_if(opencode.variable_definitions.begin(),
                opencode.variable_definitions.end(),
                [&](const auto& var) { return var.name == occ.name; });
            if (sym != opencode.variable_definitions.end())
                retval = &*sym;
        }
        return retval;
    }
    if constexpr (kind == lsp::occurence_kind::INSTR)
    {
        context::opcode_t retval;
        retval.machine_opcode = occ.name;
        retval.macro_opcode = occ.opcode;
        return retval;
    }
}

std::optional<location> lsp_context::find_definition_location(const symbol_occurence& occ, macro_info_ptr macro_i) const
{
    switch (occ.kind)
    {
        case lsp::occurence_kind::ORD: {
            auto sym = find_definition<lsp::occurence_kind::ORD, const context::symbol*>(occ, macro_i, *opencode_);
            if (sym)
                return sym->symbol_location;
            break;
        }
        case lsp::occurence_kind::SEQ: {
            auto sym =
                find_definition<lsp::occurence_kind::SEQ, const context::sequence_symbol*>(occ, macro_i, *opencode_);
            if (sym)
                return sym->symbol_location;
            break;
        }
        case lsp::occurence_kind::VAR: {
            auto sym =
                find_definition<lsp::occurence_kind::VAR, const variable_symbol_definition*>(occ, macro_i, *opencode_);
            if (sym)
            {
                if (macro_i)
                    return location(
                        sym->def_position, macro_i->macro_definition->copy_nests[sym->def_location].back().file);
                return location(sym->def_position, sym->file);
            }
            break;
        }
        case lsp::occurence_kind::INSTR:
            if (occ.opcode)
                return occ.opcode->definition_location;
            break;
        default:
            break;
    }
    return std::nullopt;
}

string_array lsp_context::find_hover(const symbol_occurence& occ, macro_info_ptr macro_i) const
{
    switch (occ.kind)
    {
        case lsp::occurence_kind::ORD: {
            auto sym = find_definition<lsp::occurence_kind::ORD, const context::symbol*>(occ, macro_i, *opencode_);
            if (sym)
                return hover(*sym);
            break;
        }
        case lsp::occurence_kind::SEQ: {
            auto sym =
                find_definition<lsp::occurence_kind::SEQ, const context::sequence_symbol*>(occ, macro_i, *opencode_);
            if (sym)
                return hover(*sym);
            break;
        }
        case lsp::occurence_kind::VAR: {
            auto sym =
                find_definition<lsp::occurence_kind::VAR, const variable_symbol_definition*>(occ, macro_i, *opencode_);
            if (sym)
                return hover(*sym);
            break;
        }
        case lsp::occurence_kind::INSTR: {
            auto sym = find_definition<lsp::occurence_kind::INSTR, const context::opcode_t>(occ, macro_i, *opencode_);
            return hover(sym);
        }
        default:
            break;
    }
    return {};
}

string_array lsp_context::hover(const context::symbol& sym) const
{
    string_array result;
    if (sym.value().value_kind() == context::symbol_value_kind::UNDEF)
        return result;

    if (sym.value().value_kind() == context::symbol_value_kind::ABS)
    {
        result = { std::to_string(sym.value().get_abs()) };
        result.push_back("Absolute Symbol");
    }
    else if (sym.value().value_kind() == context::symbol_value_kind::RELOC)
    {
        result = { sym.value().get_reloc().to_string() }; // move to_string method from that class to this class
        result.push_back("Relocatable Symbol");
    }

    const auto& attrs = sym.attributes();
    if (attrs.is_defined(context::data_attr_kind::L))
        result.push_back("L: " + std::to_string(attrs.length()));
    if (attrs.is_defined(context::data_attr_kind::I))
        result.push_back("I: " + std::to_string(attrs.integer()));
    if (attrs.is_defined(context::data_attr_kind::S))
        result.push_back("S: " + std::to_string(attrs.scale()));
    if (attrs.is_defined(context::data_attr_kind::T))
        result.push_back("T: " + ebcdic_encoding::to_ascii((unsigned char)attrs.type()));

    return result;
}

string_array lsp_context::hover(const context::sequence_symbol& sym) const { return string_array(); }

string_array lsp_context::hover(const variable_symbol_definition& sym) const
{
    string_array result;

    if (sym.macro_param)
        result.push_back("MACRO parameter");
    else
        switch (sym.type)
        {
            case context::SET_t_enum::A_TYPE:
                result.push_back("SETA variable");
                break;
            case context::SET_t_enum::B_TYPE:
                result.push_back("SETB variable");
                break;
            case context::SET_t_enum::C_TYPE:
                result.push_back("SETC variable");
                break;
            default:
                break;
        }

    return result;
}

string_array lsp_context::hover(const context::opcode_t& sym) const { return string_array(); }

} // namespace hlasm_plugin::parser_library::lsp
