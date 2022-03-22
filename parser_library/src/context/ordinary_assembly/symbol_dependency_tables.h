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
#include "dependable.h"
#include "dependant.h"
#include "loctr_dependency_resolver.h"
#include "postponed_statement.h"
#include "tagged_index.h"

namespace hlasm_plugin::parser_library::context {

class ordinary_assembly_context;
class using_collection;

struct dependency_evaluation_context
{
    std::optional<address> loctr_address;
    size_t literal_pool_generation = 0;
    size_t unique_id = 0;
    index_t<using_collection> active_using;

    dependency_evaluation_context() = default;
    dependency_evaluation_context(std::optional<address> loctr_address,
        size_t literal_pool_generation,
        size_t unique_id,
        index_t<using_collection> active_using)
        : loctr_address(std::move(loctr_address))
        , literal_pool_generation(literal_pool_generation)
        , unique_id(unique_id)
        , active_using(active_using)
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
    // actual dependecies of symbol or space
    std::unordered_map<dependant, std::pair<const resolvable*, dependency_evaluation_context>> dependencies_;

    // statements where dependencies are from
    std::unordered_map<dependant, statement_ref> dependency_source_stmts_;
    // addresses where dependencies are from
    std::unordered_map<dependant, addr_res_ptr> dependency_source_addrs_;
    // list of statements containing dependencies that can not be checked yet
    std::unordered_map<post_stmt_ptr, dependency_evaluation_context> postponed_stmts_;

    ordinary_assembly_context& sym_ctx_;

    bool check_cycle(dependant target, std::vector<dependant> dependencies);

    void resolve_dependant(dependant target,
        const resolvable* dep_src,
        loctr_dependency_resolver* resolver,
        const dependency_evaluation_context& dep_ctx);
    void resolve_dependant_default(dependant target);
    void resolve(loctr_dependency_resolver* resolver);

    std::vector<dependant> extract_dependencies(
        const resolvable* dependency_source, const dependency_evaluation_context& dep_ctx);
    std::vector<dependant> extract_dependencies(
        const std::vector<const resolvable*>& dependency_sources, const dependency_evaluation_context& dep_ctx);

    void try_erase_source_statement(dependant index);

    bool add_dependency(dependant target,
        const resolvable* dependency_source,
        bool check_cycle,
        const dependency_evaluation_context& dep_ctx);

public:
    symbol_dependency_tables(ordinary_assembly_context& sym_ctx);

    // add symbol dependency on statement
    // returns false if cyclic dependency occured
    [[nodiscard]] bool add_dependency(id_index target,
        const resolvable* dependency_source,
        post_stmt_ptr dependency_source_stmt,
        const dependency_evaluation_context& dep_ctx);

    // add symbol attribute dependency on statement
    // returns false if cyclic dependency occured
    [[nodiscard]] bool add_dependency(id_index target,
        data_attr_kind attr,
        const resolvable* dependency_source,
        post_stmt_ptr dependency_source_stmt,
        const dependency_evaluation_context& dep_ctx);

    // add space dependency
    void add_dependency(space_ptr target,
        const resolvable* dependency_source,
        post_stmt_ptr dependency_source_stmt,
        const dependency_evaluation_context& dep_ctx);
    void add_dependency(space_ptr target,
        addr_res_ptr dependency_source,
        const dependency_evaluation_context& dep_ctx,
        post_stmt_ptr dependency_source_stmt = nullptr);
    bool check_cycle(space_ptr target);

    // add statement dependency on its operands
    void add_dependency(post_stmt_ptr target, const dependency_evaluation_context& dep_ctx);

    // method for creating more than one dependency assigned to one statement
    dependency_adder add_dependencies(
        post_stmt_ptr dependency_source_stmt, const dependency_evaluation_context& dep_ctx);

    // registers that some symbol has been defined
    // if resolver is present, location counter dependencies are checked as well (not just symbol deps)
    void add_defined(loctr_dependency_resolver* resolver = nullptr);

    // checks for cycle in location counter value
    bool check_loctr_cycle();

    // collect all postponed statements either if they still contain dependent objects
    std::vector<std::pair<post_stmt_ptr, dependency_evaluation_context>> collect_postponed();

    // assign default values to all unresoved dependants
    void resolve_all_as_default();

    friend dependency_adder;
};

// helper class to add dependencies
class dependency_adder
{
    symbol_dependency_tables& owner_;
    size_t ref_count_;

    std::vector<dependant> dependants;
    dependency_evaluation_context dep_ctx;

public:
    post_stmt_ptr source_stmt;

    dependency_adder(symbol_dependency_tables& owner,
        post_stmt_ptr dependency_source_stmt,
        const dependency_evaluation_context& dep_ctx);

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
