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

#include <cassert>
#include <deque>
#include <memory>
#include <set>
#include <vector>

#include "code_scope.h"
#include "compiler_options.h"
#include "operation_code.h"
#include "ordinary_assembly/ordinary_assembly_context.h"
#include "source_context.h"
#include "tagged_index.h"

namespace hlasm_plugin::parser_library::expressions {
class mach_expression;
} // namespace hlasm_plugin::parser_library::expressions
namespace hlasm_plugin::parser_library::context {
class using_collection;
} // namespace hlasm_plugin::parser_library::context

namespace hlasm_plugin::parser_library::context {

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
    using global_variable_storage = std::unordered_map<id_index, var_sym_ptr>;

    // storage of global variables
    global_variable_storage globals_;

    // storage of defined macros
    macro_storage macros_;
    // storage of copy members
    copy_member_storage copy_members_;
    // map of OPSYN mnemonics
    opcode_map opcode_mnemo_;
    // storage of identifiers
    std::shared_ptr<id_storage> ids_;

    // stack of nested scopes
    std::deque<code_scope> scope_stack_;
    code_scope* curr_scope();
    const code_scope* curr_scope() const;
    // stack of processed source files
    std::vector<source_context> source_stack_;

    // path to the opencode
    std::string opencode_file_name_;
    // all files processes via macro or copy member invocation
    std::set<std::string> visited_files_;

    // Compiler options
    asm_option asm_options_;
    static constexpr alignment sectalgn = doubleword;

    // map of all instruction in HLASM
    const instruction_storage instruction_map_;
    static instruction_storage init_instruction_map(id_storage& ids);

    // value of system variable SYSNDX
    unsigned long SYSNDX_ = 1;
    static constexpr unsigned long SYSNDX_limit_max = 9999999UL;
    unsigned long SYSNDX_limit = SYSNDX_limit_max;

    // last AINSERT virtual file id
    size_t m_ainsert_id = 0;
    bool m_end_reached = false;

    void add_global_system_var_to_scope(id_storage& ids, std::string name, code_scope& scope) const;

    void add_system_vars_to_scope(code_scope& scope);
    void add_global_system_vars(code_scope& scope);

    bool is_opcode(id_index symbol) const;

    std::unique_ptr<using_collection> m_usings;
    std::vector<index_t<using_collection>> m_active_usings;

public:
    hlasm_context(std::string file_name = "",
        asm_option asm_opts = {},
        std::shared_ptr<id_storage> init_ids = std::make_shared<id_storage>());
    ~hlasm_context();

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
    // gets top level (opencode) copy stack
    const std::vector<copy_member_invocation>& opencode_copy_stack() const;
    std::vector<copy_member_invocation>& opencode_copy_stack();
    // is open code being processed
    bool in_opencode() const { return source_stack_.size() == 1; }
    bool get_end_reached() const { return m_end_reached; };
    void end_reached();
    // gets names of whole copy nest
    std::vector<id_index> whole_copy_stack() const;

    const code_scope& current_scope() const;

    // index storage
    id_storage& ids();
    std::shared_ptr<id_storage> ids_ptr();

    // map of instructions
    const instruction_storage& instruction_map() const;

    // field that accessed ordinary assembly context
    ordinary_assembly_context ord_ctx;

    // performance metrics
    performance_metrics metrics;

    void fill_metrics_files();
    // return map of global set vars
    const global_variable_storage& globals() const;

    // return variable symbol in current scope
    // returns empty shared_ptr if there is none in the current scope
    var_sym_ptr get_var_sym(id_index name) const;

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
    const opcode_map& opcode_mnemo_storage() const;

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
        location definition_location,
        std::unordered_set<copy_member_ptr> used_copy_members);
    void add_macro(macro_def_ptr macro);
    // enters a macro with actual params
    macro_invo_ptr enter_macro(id_index name, macro_data_ptr label_param_data, std::vector<macro_arg> params);
    // leaves current macro
    void leave_macro();

    // gets copy member storage
    const copy_member_storage& copy_members();
    // registers new copy member
    copy_member_ptr add_copy_member(id_index member, statement_block definition, location definition_location);
    void add_copy_member(copy_member_ptr member);
    copy_member_ptr get_copy_member(id_index member) const;
    // enters a copy member
    void enter_copy_member(id_index member);
    // leaves current copy member
    void leave_copy_member();

    // register preprocessor dependency
    void add_preprocessor_dependency(const std::string& file);

    // creates specified global set symbol
    template<typename T>
    set_sym_ptr create_global_variable(id_index id, bool is_scalar)
    {
        auto* scope = curr_scope();

        if (auto tmp = scope->variables.find(id); tmp != scope->variables.end())
            return tmp->second;

        if (auto glob = globals_.find(id); glob != globals_.end())
        {
            set_sym_ptr var = std::dynamic_pointer_cast<set_symbol<T>>(glob->second);
            assert(var);

            scope->variables.insert({ id, var });
            return var;
        }

        auto val = std::make_shared<set_symbol<T>>(id, is_scalar, true);

        globals_.insert({ id, val });
        scope->variables.insert({ id, val });

        return val;
    }

    // creates specified local set symbol
    template<typename T>
    set_sym_ptr create_local_variable(id_index id, bool is_scalar)
    {
        auto* scope = curr_scope();

        auto tmp = scope->variables.find(id);
        if (tmp != scope->variables.end())
            return tmp->second;


        set_sym_ptr val(std::make_shared<set_symbol<T>>(id, is_scalar, false));

        scope->variables.insert({ id, val });

        return val;
    }

    unsigned long next_sysndx() const { return SYSNDX_; }
    void sysndx_limit(unsigned long limit)
    {
        assert(limit <= SYSNDX_limit_max);
        SYSNDX_limit = limit;
    }
    unsigned long sysndx_limit() const { return SYSNDX_limit; }
    static constexpr unsigned long sysndx_limit_max() { return SYSNDX_limit_max; }

    alignment section_alignment() const { return sectalgn; }

    size_t current_ainsert_id() const { return m_ainsert_id; }
    size_t obtain_ainsert_id() { return ++m_ainsert_id; }

    void using_add(id_index label,
        std::unique_ptr<expressions::mach_expression> begin,
        std::unique_ptr<expressions::mach_expression> end,
        std::vector<std::unique_ptr<expressions::mach_expression>> arguments,
        dependency_evaluation_context eval_ctx,
        processing_stack_t stack);
    void using_remove(std::vector<std::unique_ptr<expressions::mach_expression>> arguments,
        dependency_evaluation_context eval_ctx,
        processing_stack_t stack);
    void using_push();
    bool using_pop();
    void using_resolve(diagnostic_s_consumer&);
    index_t<using_collection> using_current() const;

    using name_result = std::pair<bool, context::id_index>;
    name_result try_get_symbol_name(const std::string& symbol);
};

} // namespace hlasm_plugin::parser_library::context

#endif
