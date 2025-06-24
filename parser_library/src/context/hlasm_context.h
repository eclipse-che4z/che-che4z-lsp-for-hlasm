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
#include <span>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "code_scope.h"
#include "compiler_options.h"
#include "opcode_generation.h"
#include "operation_code.h"
#include "ordinary_assembly/ordinary_assembly_context.h"
#include "protocol.h"
#include "source_context.h"
#include "tagged_index.h"

namespace hlasm_plugin::parser_library {
class library_info;
} // namespace hlasm_plugin::parser_library
namespace hlasm_plugin::parser_library::expressions {
struct evaluation_context;
class mach_expression;
} // namespace hlasm_plugin::parser_library::expressions
namespace hlasm_plugin::parser_library::context {
class using_collection;
} // namespace hlasm_plugin::parser_library::context

namespace hlasm_plugin::parser_library::context {

class id_storage;

class system_variable_map
{
    std::unordered_map<id_index, std::pair<std::shared_ptr<system_variable>, bool>> map;

public:
    void insert(std::pair<id_index, std::pair<std::shared_ptr<system_variable>, bool>> p) { map.insert(std::move(p)); }

    const decltype(map)::value_type* find(id_index name) const
    {
        auto it = map.find(name);
        if (it == map.end())
            return nullptr;
        return std::to_address(it);
    }

    auto begin() const { return map.begin(); }
    auto end() const { return map.end(); }
};

// class helping to perform semantic analysis of hlasm source code
// wraps all classes and structures needed by semantic analysis (like variable symbol tables, opsyn tables...) in one
// place contains methods that store gathered information from semantic analysis helping it to correctly evaluate parsed
// code
class hlasm_context
{
private:
    using macro_storage = std::unordered_map<id_index, std::vector<std::pair<macro_def_ptr, opcode_generation>>>;
    using copy_member_storage = std::unordered_map<id_index, copy_member_ptr>;
    using instruction_storage = std::unordered_map<id_index, opcode_t::opcode_variant>;
    using opcode_map = std::unordered_map<id_index, std::vector<std::pair<opcode_t, opcode_generation>>>;
    using global_variable_storage =
        std::unordered_map<id_index, std::variant<set_symbol<A_t>, set_symbol<B_t>, set_symbol<C_t>>>;

    // storage of global variables
    global_variable_storage globals_;
    system_variable_map system_variables;

    // storage of defined macros
    macro_storage macros_;
    std::unordered_map<id_index, macro_def_ptr> external_macros_;
    // storage of copy members
    copy_member_storage copy_members_;
    // map of OPSYN mnemonics
    opcode_map opcode_mnemo_;
    opcode_generation m_current_opcode_generation = opcode_generation::zero;

    // storage of identifiers
    std::shared_ptr<id_storage> ids_;

    // stack of nested scopes
    std::deque<code_scope> scope_stack_;
    code_scope* curr_scope();
    const code_scope* curr_scope() const;
    // stack of processed source files
    std::vector<source_context> source_stack_;

    // opencode file location
    utils::resource::resource_location opencode_file_location_;

    // Compiler options
    asm_option asm_options_;
    static constexpr alignment sectalgn = doubleword;

    // map of active instructions in HLASM
    static void init_instruction_map(opcode_map& opcodes, id_storage& ids, instruction_set_version active_instr_set);
    void add_global_system_variables(system_variable_map& sysvars);
    void add_scoped_system_variables(system_variable_map& sysvars, size_t skip_last, bool globals_only);

    std::vector<macro_data_ptr>& ensure_dynamic_ptrs_count();
    std::vector<macro_data_ptr> dynamic_ptrs_vector;

    // value of system variable SYSNDX
    unsigned long SYSNDX_ = 1;
    static constexpr unsigned long SYSNDX_limit_max = 9999999UL;
    unsigned long SYSNDX_limit = SYSNDX_limit_max;

    // last AINSERT virtual file id
    size_t m_ainsert_id = 0;
    bool m_end_reached = false;

    std::unique_ptr<using_collection> m_usings;
    std::vector<index_t<using_collection>> m_active_usings;

    long long m_statements_remaining;

    processing_frame_tree m_stack_tree;

    std::string m_title_name;

    unsigned mnote_max = 0;

    label_storage opencode_sequence_symbols;

    std::unordered_map<id_index, std::pair<id_index, processing_stack_t>> psect_registrations;

    // return variable symbol from an arbitrary scope
    variable_symbol* get_var_sym(id_index name, const code_scope& scope, const system_variable_map& sysvars) const;

    template<typename Pred, typename Proj = std::identity>
    const opcode_t* search_opcodes(id_index name, Pred p, Proj proj = Proj()) const;
    const opcode_t* search_opcodes(id_index name, opcode_generation gen) const;

public:
    static std::shared_ptr<id_storage> make_default_id_storage();

    hlasm_context(utils::resource::resource_location file_loc = utils::resource::resource_location(""),
        asm_option asm_opts = {},
        std::shared_ptr<id_storage> init_ids = make_default_id_storage());
    ~hlasm_context();

    // gets opencode file location
    const utils::resource::resource_location& opencode_location() const;

    // gets current source
    const source_context& current_source() const;
    // sets current scope according to the snapshot
    void apply_source_snapshot(source_snapshot snapshot);
    // sets current source position
    void set_source_position(position pos);
    // sets current source file indices
    void set_source_indices(size_t begin_index, size_t end_index);

    std::pair<source_position, source_snapshot> get_begin_snapshot(bool ignore_macros) const;
    std::pair<source_position, source_snapshot> get_end_snapshot() const;

    // pushes new kind of statement processing
    void push_statement_processing(const processing::processing_kind kind);
    // pushes new kind of statement processing as well as new source
    void push_statement_processing(const processing::processing_kind kind, utils::resource::resource_location file_loc);
    // pops statement processing
    void pop_statement_processing();

    // gets stack of locations of all currently processed files
    processing_stack_t processing_stack();
    processing_stack_details_t processing_stack_details();
    position current_statement_position(bool consider_macros = true);
    location current_statement_location(bool consider_macros = true);
    const utils::resource::resource_location& current_statement_source(bool consider_macros = true);

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
    id_index add_id(std::string&& s);
    id_index add_id(std::string_view s);
    std::optional<id_index> find_id(std::string_view s) const;

    // field that accessed ordinary assembly context
    ordinary_assembly_context ord_ctx;

    // performance metrics
    performance_metrics metrics;

    // return map of global set vars
    const global_variable_storage& globals() const;

    // return variable symbol in current scope
    variable_symbol* get_var_sym(id_index name) const; // testing only

    // registers sequence symbol
    void add_opencode_sequence_symbol(std::unique_ptr<opencode_sequence_symbol> seq_sym);
    // return sequence symbol in current scope
    // returns nullptr if there is none in the current scope
    const sequence_symbol* get_sequence_symbol(id_index name) const;
    // return opencode sequence symbol
    // returns nullptr if there is none
    const sequence_symbol* get_opencode_sequence_symbol(id_index name) const;

    size_t set_branch_counter(A_t value);
    A_t get_branch_counter() const;
    void decrement_branch_counter();

    // adds opsyn mnemonic
    bool add_mnemonic(id_index mnemo, id_index op_code);
    // removes opsyn mnemonic
    bool remove_mnemonic(id_index mnemo);
    const opcode_map& opcode_mnemo_storage() const;

    // checks whether the symbol is an operation code (is a valid instruction or a mnemonic)
    opcode_t get_operation_code(id_index symbol, context::id_index* ext_suggestion = nullptr) const;

    // get data attribute value of ordinary symbol
    SET_t get_attribute_value_ord(data_attr_kind attribute, id_index symbol);
    SET_t get_attribute_value_ord(data_attr_kind attribute, const symbol* symbol);

    C_t get_opcode_attr(id_index symbol, opcode_generation gen = opcode_generation::current) const;

    // gets macro storage
    const macro_storage& macros() const;
    const macro_def_ptr* find_macro(id_index name, opcode_generation gen = opcode_generation::current) const;
    macro_definition* get_macro_definition(id_index name, opcode_generation gen = opcode_generation::current) const;
    // checks whether processing is currently in macro
    bool is_in_macro() const;
    // returns macro we are currently in or empty shared_ptr if in open code
    const macro_invocation* current_macro() const;
    const location* current_macro_definition_location() const;
    // registers new macro
    macro_def_ptr add_macro(id_index name,
        id_index label_param_name,
        std::vector<macro_arg> params,
        statement_block definition,
        copy_nest_storage copy_nests,
        label_storage labels,
        location definition_location,
        std::unordered_set<copy_member_ptr> used_copy_members,
        bool external);
    void add_macro(macro_def_ptr macro, bool external);
    // enters a macro with actual params
    std::pair<const macro_invocation*, bool> enter_macro(
        macro_definition* macro_def, macro_data_ptr label_param_data, std::vector<macro_arg> params);
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

    // creates specified global set symbol
    template<typename T>
    set_symbol_base* create_global_variable(id_index id, bool is_scalar);

    // creates specified local set symbol
    template<typename T>
    set_symbol_base* create_local_variable(id_index id, bool is_scalar);

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
    void using_resolve(diagnostic_consumer&, const library_info&);
    index_t<using_collection> using_current() const;

    const using_collection& usings() const { return *m_usings; }

    using name_result = std::pair<bool, context::id_index>;
    name_result try_get_symbol_name(std::string_view symbol);
    name_result try_get_symbol_name(id_index symbol) const;

    bool next_statement() { return --m_statements_remaining >= 0; }

    const opcode_t* find_opcode_mnemo(id_index name,
        opcode_generation gen = opcode_generation::current,
        context::id_index* ext_suggestion = nullptr) const;
    const opcode_t* find_any_valid_opcode(id_index name) const;

    opcode_generation current_opcode_generation() const { return m_current_opcode_generation; }

    const std::string& get_title_name() const { return m_title_name; }
    void set_title_name(std::string name) { m_title_name = std::move(name); }

    void update_mnote_max(unsigned mnote_level);

    const auto& get_opencode_sequence_symbols() const noexcept { return opencode_sequence_symbols; }

    system_variable_map get_system_variables(const code_scope&);

    friend variable_symbol* get_var_sym(const expressions::evaluation_context& eval_ctx, id_index name);

    bool goff() const noexcept { return asm_options_.sysopt_xobject; }
    const auto& options() const noexcept { return asm_options_; }

    bool register_psect(id_index symbol, id_index psect);
    void validate_psect_registrations(diagnostic_consumer& diags);
};

bool test_symbol_for_read(const variable_symbol* var,
    std::span<const A_t> subscript,
    range symbol_range,
    diagnostic_op_consumer& diags,
    std::string_view var_name);

SET_t get_var_sym_value(
    const expressions::evaluation_context& eval_ctx, id_index name, std::span<const A_t> subscript, range symbol_range);

const code_scope& get_current_scope(const context::hlasm_context&);
variable_symbol* get_var_sym(const expressions::evaluation_context& eval_ctx, id_index name);
} // namespace hlasm_plugin::parser_library::context

#endif
