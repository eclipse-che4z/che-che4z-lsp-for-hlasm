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

#include <unordered_set>
#include <vector>

#include "context/copy_member.h"
#include "context/hlasm_statement.h"
#include "context/id_index.h"
#include "context/macro.h"
#include "location.h"
#include "lsp/macro_info.h"

namespace hlasm_plugin::parser_library::processing {

// data to start macrodef_processor
struct macrodef_start_data
{
    bool is_external;
    context::id_index external_name;

    macrodef_start_data()
        : is_external(false)
    {}
    macrodef_start_data(context::id_index external_name)
        : is_external(true)
        , external_name(external_name)
    {}
};

// data holding info about prototype statement of a macro
struct macrodef_prototype
{
    context::id_index macro_name;
    range macro_name_range;

    context::id_index name_param;
    std::vector<context::macro_arg> symbolic_params;
};

// result of macrodef_processor
struct macrodef_processing_result
{
    macrodef_prototype prototype;

    context::statement_block definition;
    context::copy_nest_storage nests;
    context::macro_label_storage sequence_symbols;
    std::unordered_set<context::copy_member_ptr> used_copy_members;

    lsp::vardef_storage variable_symbols;
    lsp::file_scopes_t file_scopes;

    location definition_location;

    bool external = false;
    bool invalid = false;
};

} // namespace hlasm_plugin::parser_library::processing

#endif
