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

#ifndef CONTEXT_HLASM_CONTEXT_H
#define CONTEXT_HLASM_CONTEXT_H

#include <compiler_options.h>
#include <deque>
#include <memory>
#include <set>
#include <vector>

#include "code_scope.h"
#include "operation_code.h"
#include "ordinary_assembly/ordinary_assembly_context.h"
#include "processing_context.h"


namespace hlasm_plugin::parser_library::context {

class hlasm_context;
using hlasm_ctx_ptr = std::shared_ptr<hlasm_context>;

// class helping to perform semantic analysis of hlasm source code
// wraps all classes and structures needed by semantic analysis (like variable symbol tables, opsyn tables...) in one
// place contains methods that store gathered information from semantic analysis helping it to correctly evaluate parsed
// code
class hlasm_context
{
    using macro_storage = std::unordered_map<id_index, macro_def_ptr>;
    using copy_member_storage = std::unordered_map<id_index, copy_member_ptr>;
    using instruction_storage = std::unordered_map<id_index, opcode_t::opcode_variant>;
    using opcode_map = std::unordered_map<id_index, opcode_t>;

    // storage of global variables
    code_scope::set_sym_storage globals_;
    // storage of defined macros
    macro_storage macros_;
    // storage of copy members
    copy_member_storage copy_members_;
    // map of OPSYN mnemonics
    opcode_map opcode_mnemo_;
    // storage of identifiers
    id_storage ids_;

    // stack of nested scopes
    std::deque<code_scope> scope_stack_;
    code_scope* curr_scope();
    const code_scope* curr_scope() const;
    // stack of statement processings
    std::vector<processing_context> proc_stack_;
    // stack of processed source files
    std::vector<source_context> source_stack_;
    // stack of nested copy member invocations
    std::vector<copy_member_invocation> copy_stack_;

    // path to the opencode
    std::string opencode_file_name_;
    // all files processes via macro or copy member invocation
    std::set<std::string> visited_files_;

    // Compiler options
    asm_option asm_options_;

    // map of all instruction in HLASM
    const instruction_storage instruction_map_;
    instruction_storage init_instruction_map();

    // value of system variable SYSNDX
    size_t SYSNDX_;
    void add_system_vars_to_scope();
    void add_global_system_vars();

    bool is_opcode(id_index symbol) const;

public:
    hlasm_context(std::string file_name = "", asm_option asm_opts = {});

    // gets name of file where is open-code located
    const std::string& opencode_file_name() const;
    // accesses visited files
    const std::set<std::string>& get_visited_files();

    // gets current source
    const source_context& current_source() const;
    // sets current scope according to the snapshot
    void apply_source_snapshot(source_snapshot snapshot);
    // sets current source position
    void set_source_position(position pos);
    // sets current source file indices
    void set_source_indices(size_t begin_index, size_t end_index, size_t end_line);

    std::pair<source_position, source_snapshot> get_begin_snapshot(bool ignore_macros) const;
    std::pair<source_position, source_snapshot> get_end_snapshot() const;

    // pushes new kind of statement processing
    void push_statement_processing(const processing::processing_kind kind);
    // pushes new kind of statement processing as well as new source
    void push_statement_processing(const processing::processing_kind kind, std::string file_name);
    // pops statement processing
    void pop_statement_processing();

    // gets stack of locations of all currently processed files
    processing_stack_t processing_stack() const;
    location current_statement_location() const;
    // gets macro nest
    const std::deque<code_scope>& scope_stack() const;
    // gets copy nest of current statement processing
    const std::vector<copy_member_invocation>& current_copy_stack() const;
    std::vector<copy_member_invocation>& current_copy_stack();
    // gets names of whole copy nest
    std::vector<id_index> whole_copy_stack() const;

    const code_scope& current_scope() const;

    // index storage
    id_storage& ids();

    // map of instructions
    const instruction_storage& instruction_map() const;

    // field that accessed ordinary assembly context
    ordinary_assembly_context ord_ctx;

    // performance metrics
    performance_metrics metrics;

    void fill_metrics_files();
    // return map of global set vars
    const code_scope::set_sym_storage& globals() const;

    // return variable symbol in current scope
    // returns empty shared_ptr if there is none in the current scope
    var_sym_ptr get_var_sym(id_index name);

    // registers sequence symbol
    void add_sequence_symbol(sequence_symbol_ptr seq_sym);
    // return sequence symbol in current scope
    // returns nullptr if there is none in the current scope
    const sequence_symbol* get_sequence_symbol(id_index name) const;
    // return opencode sequence symbol
    // returns nullptr if there is none
    const sequence_symbol* get_opencode_sequence_symbol(id_index name) const;

    void set_branch_counter(A_t value);
    A_t get_branch_counter() const;
    void decrement_branch_counter();

    // adds opsyn mnemonic
    void add_mnemonic(id_index mnemo, id_index op_code);
    // removes opsyn mnemonic
    void remove_mnemonic(id_index mnemo);

    // checks wheter the symbol is an operation code (is a valid instruction or a mnemonic)
    opcode_t get_operation_code(id_index symbol) const;

    // get data attribute value of variable symbol
    SET_t get_attribute_value_ca(data_attr_kind attribute, var_sym_ptr var_symbol, std::vector<size_t> offset);
    // get data attribute value of ordinary symbol
    SET_t get_attribute_value_ca(data_attr_kind attribute, id_index symbol);
    SET_t get_attribute_value_ca(data_attr_kind attribute, const symbol* symbol);

    C_t get_type_attr(var_sym_ptr var_symbol, const std::vector<size_t>& offset);
    C_t get_opcode_attr(id_index symbol);

    // gets macro storage
    const macro_storage& macros() const;
    macro_def_ptr get_macro_definition(id_index name) const;
    // checks whether processing is currently in macro
    bool is_in_macro() const;
    // returns macro we are currently in or empty shared_ptr if in open code
    macro_invo_ptr this_macro() const;
    // registers new macro
    macro_def_ptr add_macro(id_index name,
        id_index label_param_name,
        std::vector<macro_arg> params,
        statement_block definition,
        copy_nest_storage copy_nests,
        label_storage labels,
        location definition_location);
    // enters a macro with actual params
    macro_invo_ptr enter_macro(id_index name, macro_data_ptr label_param_data, std::vector<macro_arg> params);
    // leaves current macro
    void leave_macro();

    // gets copy member storage
    const copy_member_storage& copy_members();
    // registers new copy member
    copy_member_ptr add_copy_member(id_index member, statement_block definition, location definition_location);
    // enters a copy member
    void enter_copy_member(id_index member);
    // leaves current copy member
    void leave_copy_member();

    // creates specified global set symbol
    template<typename T>
    set_sym_ptr create_global_variable(id_index id, bool is_scalar)
    {
        auto tmp = curr_scope()->variables.find(id);
        if (tmp != curr_scope()->variables.end())
            return tmp->second;

        auto glob = globals_.find(id);
        if (glob != globals_.end())
        {
            curr_scope()->variables.insert({ id, glob->second });
            return glob->second;
        }

        auto val = std::make_shared<set_symbol<T>>(id, is_scalar, true);

        globals_.insert({ id, val });
        curr_scope()->variables.insert({ id, val });

        return val;
    }

    // creates specified local set symbol
    template<typename T>
    set_sym_ptr create_local_variable(id_index id, bool is_scalar)
    {
        auto tmp = curr_scope()->variables.find(id);
        if (tmp != curr_scope()->variables.end())
            return tmp->second;


        set_sym_ptr val(std::make_shared<set_symbol<T>>(id, is_scalar, false));

        curr_scope()->variables.insert({ id, val });

        return val;
    }
};

} // namespace hlasm_plugin::parser_library::context

#endif
