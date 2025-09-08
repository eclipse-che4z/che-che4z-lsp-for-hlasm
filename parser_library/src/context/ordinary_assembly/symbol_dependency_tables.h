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

#ifndef SEMANTICS_SYMBOL_DEPENDENCY_TABLES_H
#define SEMANTICS_SYMBOL_DEPENDENCY_TABLES_H

#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "address.h"
#include "address_resolver.h"
#include "context/opcode_generation.h"
#include "dependable.h"
#include "dependant.h"
#include "diagnostic_consumer.h"
#include "postponed_statement.h"
#include "tagged_index.h"
#include "utils/filter_vector.h"

namespace hlasm_plugin::parser_library {
class library_info;
} // namespace hlasm_plugin::parser_library

namespace hlasm_plugin::parser_library::context {

class ordinary_assembly_context;
class using_collection;

struct dependency_evaluation_context
{
    std::optional<address> loctr_address;
    size_t literal_pool_generation = 0;
    size_t unique_id = 0;
    index_t<using_collection> active_using;
    opcode_generation opcode_gen;

    explicit dependency_evaluation_context(opcode_generation opcode_gen)
        : opcode_gen(opcode_gen)
    {}

    explicit dependency_evaluation_context(std::optional<address> loctr_address,
        size_t literal_pool_generation,
        size_t unique_id,
        index_t<using_collection> active_using,
        opcode_generation opcode_gen)
        : loctr_address(std::move(loctr_address))
        , literal_pool_generation(literal_pool_generation)
        , unique_id(unique_id)
        , active_using(active_using)
        , opcode_gen(opcode_gen)
    {}
};

using postponed_statements_t = std::vector<std::pair<post_stmt_ptr, dependency_evaluation_context>>;

class dependency_adder;
// class holding data about dependencies between symbols
class symbol_dependency_tables
{
    friend struct resolve_dependant_visitor;
    struct dependency_value
    {
        const resolvable* m_resolvable;
        dependency_evaluation_context m_dec;
        size_t m_last_dependencies;

        index_t<postponed_statements_t> related_statement_id;
        addr_res_ptr related_source_addr;

        dependency_value(const resolvable* r, dependency_evaluation_context dec, size_t last_dependencies)
            : m_resolvable(r)
            , m_dec(std::move(dec))
            , m_last_dependencies(last_dependencies)
        {}
    };

    // actual dependecies of symbol or space
    std::unordered_map<dependant, dependency_value> m_dependencies;

    std::vector<std::unordered_map<dependant, dependency_value>::iterator> m_dependencies_iterators;
    utils::filter_vector<uint32_t> m_dependencies_filters;
    std::vector<bool> m_dependencies_has_t_attr;
    std::vector<bool> m_dependencies_space_ptr_type;

    dependency_value& insert_depenency(
        dependant target, const resolvable* dependency_source, const dependency_evaluation_context& dep_ctx);

    void delete_dependency(std::unordered_map<dependant, dependency_value>::iterator it);

    class dep_value;
    class dep_reference;
    class dep_iterator;
    friend void swap(dep_reference l, dep_reference r) noexcept;
    dep_iterator dependency_iterator(size_t idx);
    dep_iterator dep_begin();
    dep_iterator dep_end();
    size_t m_dependencies_skip_index = 0;

    // list of statements containing dependencies that can not be checked yet
    postponed_statements_t m_postponed_stmts;
    std::vector<size_t> m_postponed_stmts_references;
    std::vector<index_t<postponed_statements_t>> m_postponed_stmts_free;

    ordinary_assembly_context& m_sym_ctx;

    template<typename T>
    index_t<postponed_statements_t> add_postponed(post_stmt_ptr, T&&);
    void delete_postponed(index_t<postponed_statements_t>);

    bool has_cycle(dependant target, std::vector<dependant> dependencies, const library_info& li);
    bool has_cycle(space_ptr target, const library_info& li);

    void resolve_dependant(dependant target,
        const resolvable* dep_src,
        diagnostic_consumer* diag_consumer,
        const dependency_evaluation_context& dep_ctx,
        const library_info& li);
    void resolve_dependant_default(const dependant& target);
    void resolve_loop(diagnostic_consumer* diag_consumer, const library_info& li);

    const dependency_value* find_dependency_value(const dependant& target) const;

    std::vector<dependant> extract_dependencies(
        const resolvable* dependency_source, const dependency_evaluation_context& dep_ctx, const library_info& li);
    bool update_dependencies(const dependency_value& v, const library_info& li);

    dependency_value* add_dependency_with_cycle_check(id_index target,
        const resolvable* dependency_source,
        const dependency_evaluation_context& dep_ctx,
        const library_info& li);
    dependency_value* add_dependency_with_cycle_check(attr_ref target,
        const resolvable* dependency_source,
        const dependency_evaluation_context& dep_ctx,
        const library_info& li);

    void establish_statement_dependency(dependency_value& val, index_t<postponed_statements_t> id);

public:
    symbol_dependency_tables(ordinary_assembly_context& sym_ctx);

    // add space dependency
    void add_dependency(space_ptr target,
        addr_res_ptr dependency_source,
        const dependency_evaluation_context& dep_ctx,
        post_stmt_ptr dependency_source_stmt = nullptr);

    void add_postponed_statement(post_stmt_ptr target, dependency_evaluation_context dep_ctx);

    // method for creating more than one dependency assigned to one statement
    dependency_adder add_dependencies(
        post_stmt_ptr dependency_source_stmt, const dependency_evaluation_context& dep_ctx, const library_info& li);
    dependency_adder add_dependencies(
        post_stmt_ptr dependency_source_stmt, const dependency_evaluation_context& dep_ctx, library_info&& li) = delete;

    // registers that some symbol has been defined
    // if resolver is present, location counter dependencies are checked as well (not just symbol deps)
    void add_defined(id_index what_changed, diagnostic_consumer* diag_consumer, const library_info& li);
    void add_defined(space_ptr what_changed, diagnostic_consumer* diag_consumer, const library_info& li);

    // checks for cycle in location counter value
    bool check_loctr_cycle(const library_info& li);

    // collect all postponed statements either if they still contain dependent objects
    postponed_statements_t collect_postponed();

    // assign default values to all unresoved dependants
    void resolve_all_as_default();

    friend dependency_adder;
};

// helper class to add dependencies
class dependency_adder
{
    friend class symbol_dependency_tables;

    symbol_dependency_tables& m_owner;
    index_t<postponed_statements_t> m_id;
    const library_info& m_li;

    dependency_adder(symbol_dependency_tables& owner, index_t<postponed_statements_t> id, const library_info& li)
        : m_owner(owner)
        , m_id(id)
        , m_li(li)
    {}

    [[nodiscard]] const dependency_evaluation_context& get_context() const noexcept;

    template<typename T>
    [[nodiscard]] bool add_dependency_with_cycle_check(T target, const resolvable* dependency_source) const;

public:
    // add symbol dependency on statement
    [[nodiscard]] bool add_dependency(id_index target, const resolvable* dependency_source) const;

    // add symbol attribute dependency on statement
    [[nodiscard]] bool add_dependency(id_index target, data_attr_kind attr, const resolvable* dependency_source) const;

    // add space dependency
    void add_dependency(space_ptr target, const resolvable* dependency_source) const;
};

} // namespace hlasm_plugin::parser_library::context

#endif
