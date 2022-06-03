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

#ifndef LSP_MACRO_INFO_H
#define LSP_MACRO_INFO_H

#include "context/macro.h"
#include "symbol_occurence.h"

namespace hlasm_plugin::parser_library::lsp {

struct variable_symbol_definition
{
    // variable symbol name
    context::id_index name;

    // flag whether is macro parameter
    bool macro_param;

    // type of SET symbol
    context::SET_t_enum type = context::SET_t_enum::UNDEF_TYPE;

    // flag whether SET symbol is global
    bool global = false;

    // statement number in macro
    size_t def_location = 0;
    // file in opencode
    utils::resource::resource_location file;

    position def_position;

    // macro parm constructor
    variable_symbol_definition(context::id_index name, size_t def_location, position def_position)
        : name(name)
        , macro_param(true)
        , def_location(def_location)
        , def_position(def_position)
    {}

    // in-macro SET symbol constructor
    variable_symbol_definition(
        context::id_index name, context::SET_t_enum type, bool global, size_t def_location, position def_position)
        : name(name)
        , macro_param(false)
        , type(type)
        , global(global)
        , def_location(def_location)
        , def_position(def_position)
    {}

    // in-opencode SET symbol constructor
    variable_symbol_definition(context::id_index name,
        context::SET_t_enum type,
        bool global,
        utils::resource::resource_location file,
        position def_position)
        : name(name)
        , macro_param(false)
        , type(type)
        , global(global)
        , file(std::move(file))
        , def_position(def_position)
    {}
};

using vardef_storage = std::vector<variable_symbol_definition>;

struct macro_slice_t
{
    size_t begin_statement, end_statement;
    bool inner_macro;

    macro_slice_t(size_t begin_statement, bool inner_macro)
        : begin_statement(begin_statement)
        , end_statement(begin_statement)
        , inner_macro(inner_macro)
    {}

    macro_slice_t(size_t begin_statement, size_t end_statement, bool inner_macro)
        : begin_statement(begin_statement)
        , end_statement(end_statement)
        , inner_macro(inner_macro)
    {}
};

using file_scopes_t = std::unordered_map<utils::resource::resource_location,
    std::vector<lsp::macro_slice_t>,
    utils::resource::resource_location_hasher>;
using file_occurences_t = std::
    unordered_map<utils::resource::resource_location, occurence_storage, utils::resource::resource_location_hasher>;

class lsp_context;

struct macro_info
{
    bool external;
    location definition_location;
    context::macro_def_ptr macro_definition;
    vardef_storage var_definitions;
    file_scopes_t file_scopes_;
    file_occurences_t file_occurences_;

    macro_info(bool external,
        location definition_location,
        context::macro_def_ptr macro_definition,
        vardef_storage var_definitions,
        file_scopes_t file_scopes,
        file_occurences_t file_occurences)
        : external(external)
        , definition_location(std::move(definition_location))
        , macro_definition(std::move(macro_definition))
        , var_definitions(std::move(var_definitions))
        , file_scopes_(std::move(file_scopes))
        , file_occurences_(std::move(file_occurences))
    {}
};

using macro_info_ptr = std::shared_ptr<macro_info>;

} // namespace hlasm_plugin::parser_library::lsp

#endif