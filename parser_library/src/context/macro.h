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

#ifndef CONTEXT_MACRO_H
#define CONTEXT_MACRO_H

#include <string>
#include <unordered_map>

#include "common_types.h"
#include "copy_member.h"
#include "sequence_symbol.h"
#include "statement_cache.h"
#include "variables/macro_param.h"

namespace hlasm_plugin::parser_library::context {

// struct wrapping macro call args
struct macro_arg
{
    id_index id;
    macro_data_ptr data;

    macro_arg(macro_data_ptr data)
        : id(nullptr)
        , data(std::move(data))
    {}
    macro_arg(macro_data_ptr data, id_index name)
        : id(name)
        , data(std::move(data))
    {}
};

class macro_definition;
struct macro_invocation;
using macro_invo_ptr = std::shared_ptr<macro_invocation>;
using macro_def_ptr = std::shared_ptr<macro_definition>;
using label_storage = std::unordered_map<id_index, sequence_symbol_ptr>;
using copy_nest_storage = std::vector<std::vector<location>>;

// class representing macro definition
// contains info about keyword, positional parameters of HLASM macro as well as list of statements
// has the 'call' method to represent macro instruction call
// serves as prototype for creating macro_invocation objects
class macro_definition
{
    using macro_param_ptr = std::shared_ptr<macro_param_base>;

    std::vector<std::unique_ptr<positional_param>> positional_params_;
    std::vector<std::unique_ptr<keyword_param>> keyword_params_;
    std::unordered_map<id_index, const macro_param_base*> named_params_;
    const id_index label_param_name_;

public:
    // identifier of macro
    const id_index id;
    // params of macro
    const std::unordered_map<id_index, const macro_param_base*>& named_params() const;
    // vector of statements representing macro definition
    std::vector<statement_cache> cached_definition;
    // vector assigning each statement its copy nest
    const copy_nest_storage copy_nests;
    // storage of sequence symbols in the macro
    const label_storage labels;
    // location of the macro definition in code
    const location definition_location;
    const std::unordered_set<copy_member_ptr> used_copy_members;
    // initializes macro with its name and params - positional or keyword
    macro_definition(id_index name,
        id_index label_param_name,
        std::vector<macro_arg> params,
        statement_block definition,
        copy_nest_storage copy_nests,
        label_storage labels,
        location definition_location,
        std::unordered_set<copy_member_ptr> used_copy_members);

    // returns object with parameters' data set to actual parameters in macro call
    macro_invo_ptr call(macro_data_ptr label_param_data, std::vector<macro_arg> actual_params, id_index syslist_name);

    // satifying unordered_map needs
    bool operator=(const macro_definition& m);

    const std::vector<std::unique_ptr<positional_param>>& get_positional_params() const;
    const std::vector<std::unique_ptr<keyword_param>>& get_keyword_params() const;
    const id_index& get_label_param_name() const;
};

// represent macro instruction call
// contains parameters with set values provided with the call
struct macro_invocation
{
public:
    // identifier of macro
    const id_index id;
    // params of macro
    const std::unordered_map<id_index, macro_param_ptr> named_params;
    // vector of statements representing macro definition
    cached_block& cached_definition;
    // vector assigning each statement its copy nest
    const copy_nest_storage& copy_nests;
    // storage of sequence symbols in the macro
    const label_storage& labels;
    // location of the macro definition in code
    const location& definition_location;
    // index to definition vector
    int current_statement;

    macro_invocation(id_index name,
        cached_block& cached_definition,
        const copy_nest_storage& copy_nests,
        const label_storage& labels,
        std::unordered_map<id_index, macro_param_ptr> named_params,
        const location& definition_location);
};

} // namespace hlasm_plugin::parser_library::context

#endif
