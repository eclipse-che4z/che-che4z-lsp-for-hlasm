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

// helper structure to count dependencies of a statement
struct statement_ref
{
    using ref_t = std::unordered_map<post_stmt_ptr, dependency_evaluation_context>::const_iterator;
    statement_ref(ref_t stmt_ref, size_t ref_count = (size_t)1);

    ref_t stmt_ref;
    size_t ref_count;
};

class dependency_adder;
// class holding data about dependencies between symbols
class symbol_dependency_tables
{
    struct dependency_value
    {
        const resolvable* m_resolvable;
        dependency_evaluation_context m_dec;
        size_t m_last_dependencies;

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

    void insert_depenency(
        dependant target, const resolvable* dependency_source, const dependency_evaluation_context& dep_ctx);

    dependant delete_dependency(std::unordered_map<dependant, dependency_value>::iterator it);

    class dep_value;
    class dep_reference;
    class dep_iterator;
    friend void swap(dep_reference l, dep_reference r) noexcept;
    dep_iterator dependency_iterator(size_t idx);
    dep_iterator dep_begin();
    dep_iterator dep_end();
    size_t m_dependencies_skip_index = 0;

    // statements where dependencies are from
    std::unordered_map<dependant, statement_ref> m_dependency_source_stmts;
    // addresses where dependencies are from
    std::unordered_map<dependant, addr_res_ptr> m_dependency_source_addrs;
    // list of statements containing dependencies that can not be checked yet
    std::unordered_map<post_stmt_ptr, dependency_evaluation_context> m_postponed_stmts;

    ordinary_assembly_context& m_sym_ctx;

    bool check_cycle(dependant target, std::vector<dependant> dependencies, const library_info& li);

    void resolve_dependant(dependant target,
        const resolvable* dep_src,
        diagnostic_s_consumer* diag_consumer,
        const dependency_evaluation_context& dep_ctx,
        const library_info& li);
    void resolve_dependant_default(const dependant& target);
    void resolve(
        std::variant<id_index, space_ptr> what_changed, diagnostic_s_consumer* diag_consumer, const library_info& li);

    const dependency_value* find_dependency_value(const dependant& target) const;

    std::vector<dependant> extract_dependencies(
        const resolvable* dependency_source, const dependency_evaluation_context& dep_ctx, const library_info& li);
    bool update_dependencies(const dependency_value& v, const library_info& li);
    std::vector<dependant> extract_dependencies(const std::vector<const resolvable*>& dependency_sources,
        const dependency_evaluation_context& dep_ctx,
        const library_info& li);

    void try_erase_source_statement(const dependant& index);

    bool add_dependency(dependant target,
        const resolvable* dependency_source,
        bool check_cycle,
        const dependency_evaluation_context& dep_ctx,
        const library_info& li);

public:
    symbol_dependency_tables(ordinary_assembly_context& sym_ctx);

    // add symbol dependency on statement
    // returns false if cyclic dependency occured
    [[nodiscard]] bool add_dependency(id_index target,
        const resolvable* dependency_source,
        post_stmt_ptr dependency_source_stmt,
        const dependency_evaluation_context& dep_ctx,
        const library_info& li);

    // add symbol attribute dependency on statement
    // returns false if cyclic dependency occured
    [[nodiscard]] bool add_dependency(id_index target,
        data_attr_kind attr,
        const resolvable* dependency_source,
        post_stmt_ptr dependency_source_stmt,
        const dependency_evaluation_context& dep_ctx,
        const library_info& li);

    // add space dependency
    void add_dependency(space_ptr target,
        const resolvable* dependency_source,
        post_stmt_ptr dependency_source_stmt,
        const dependency_evaluation_context& dep_ctx,
        const library_info& li);
    void add_dependency(space_ptr target,
        addr_res_ptr dependency_source,
        const dependency_evaluation_context& dep_ctx,
        const library_info& li,
        post_stmt_ptr dependency_source_stmt = nullptr);
    bool check_cycle(space_ptr target, const library_info& li);

    // add statement dependency on its operands
    void add_dependency(post_stmt_ptr target, const dependency_evaluation_context& dep_ctx, const library_info& li);

    // method for creating more than one dependency assigned to one statement
    dependency_adder add_dependencies(
        post_stmt_ptr dependency_source_stmt, const dependency_evaluation_context& dep_ctx, const library_info& li);

    // registers that some symbol has been defined
    // if resolver is present, location counter dependencies are checked as well (not just symbol deps)
    void add_defined(const std::variant<id_index, space_ptr>& what_changed,
        diagnostic_s_consumer* diag_consumer,
        const library_info& li);

    // checks for cycle in location counter value
    bool check_loctr_cycle(const library_info& li);

    // collect all postponed statements either if they still contain dependent objects
    std::vector<std::pair<post_stmt_ptr, dependency_evaluation_context>> collect_postponed();

    // assign default values to all unresoved dependants
    void resolve_all_as_default();

    friend dependency_adder;
};

// helper class to add dependencies
class dependency_adder
{
    symbol_dependency_tables& m_owner;
    size_t m_ref_count;

    std::vector<dependant> m_dependants;
    dependency_evaluation_context m_dep_ctx;

    post_stmt_ptr m_source_stmt;

    const library_info& m_li;

public:
    dependency_adder(symbol_dependency_tables& owner,
        post_stmt_ptr dependency_source_stmt,
        const dependency_evaluation_context& dep_ctx,
        const library_info& li);

    // add symbol dependency on statement
    [[nodiscard]] bool add_dependency(id_index target, const resolvable* dependency_source);

    // add symbol attribute dependency on statement
    [[nodiscard]] bool add_dependency(id_index target, data_attr_kind attr, const resolvable* dependency_source);

    // add space dependency
    void add_dependency(space_ptr target, const resolvable* dependency_source);

    // add statement dependency on its operands
    void add_dependency();

    // finish adding dependencies
    void finish();
};

} // namespace hlasm_plugin::parser_library::context

#endif
