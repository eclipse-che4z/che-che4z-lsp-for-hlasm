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

#ifndef PROCESSING_MACRODEF_PROCESSING_INFO_H
#define PROCESSING_MACRODEF_PROCESSING_INFO_H

#include "context/macro.h"

namespace hlasm_plugin::parser_library::processing {

// data to start macrodef_processor
struct macrodef_start_data
{
    bool is_external;
    context::id_index external_name;

    macrodef_start_data()
        : is_external(false)
        , external_name(context::id_storage::empty_id)
    {}
    macrodef_start_data(context::id_index external_name)
        : is_external(true)
        , external_name(external_name)
    {}
};

// data holding info about prototype statement of a macro
struct macrodef_prototype
{
    macrodef_prototype()
        : macro_name(context::id_storage::empty_id)
        , name_param(context::id_storage::empty_id)
    {}

    context::id_index macro_name;

    context::id_index name_param;
    std::vector<context::macro_arg> symbolic_params;
};

struct variable_symbol_definition
{
    // variable symbol name
    context::id_index name;

    // flag whether is macro parameter
    bool macro_param;

    // type of SET symbol
    context::SET_t_enum type;
    // flag whether SET symbol is global
    bool global;

    // statement number in macro
    size_t def_location;
    position def_position;

    // macro parm constructor
    variable_symbol_definition(context::id_index name, size_t def_location, position def_position)
        : name(name)
        , macro_param(true)
        , def_location(def_location)
        , def_position(def_position)
    {}

    // SET symbol constructor
    variable_symbol_definition(
        context::id_index name, context::SET_t_enum type, bool global, size_t def_location, position def_position)
        : name(name)
        , macro_param(false)
        , type(type)
        , global(global)
        , def_location(def_location)
        , def_position(def_position)
    {}
};

using vardef_storage = std::unordered_map<context::id_index, variable_symbol_definition>;

struct macro_slice_t
{
    size_t begin_line, end_line;
    bool inner_macro;

    macro_slice_t(size_t begin_line, bool inner_macro)
        : begin_line(begin_line)
        , end_line(begin_line)
        , inner_macro(inner_macro)
    {}
};

using macro_file_scopes_t = std::unordered_map<std::string, std::vector<macro_slice_t>>;

// result of macrodef_processor
struct macrodef_processing_result
{
    macrodef_processing_result()
        : invalid(false)
    {}

    macrodef_prototype prototype;

    context::statement_block definition;
    context::copy_nest_storage nests;
    context::label_storage sequence_symbols;

    vardef_storage variable_symbols;
    macro_file_scopes_t file_scopes;

    location definition_location;

    bool invalid;
};

} // namespace hlasm_plugin::parser_library::processing

#endif
