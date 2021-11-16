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

namespace hlasm_plugin::parser_library::context {

class ordinary_assembly_context;

// helper structure to count dependencies of a statement
struct statement_ref
{
    using ref_t = std::unordered_map<post_stmt_ptr, std::optional<address>>::const_iterator;
    statement_ref(ref_t stmt_ref, size_t ref_count = (size_t)1);

    std::unordered_map<post_stmt_ptr, std::optional<address>>::const_iterator stmt_ref;
    size_t ref_count;
};

class dependency_adder;
// class holding data about dependencies between symbols
class symbol_dependency_tables
{
    // actual dependecies of symbol or space
    std::unordered_map<dependant, std::pair<const resolvable*, std::optional<address>>> dependencies_;

    // statements where dependencies are from
    std::unordered_map<dependant, statement_ref> dependency_source_stmts_;
    // addresses where dependencies are from
    std::unordered_map<dependant, addr_res_ptr> dependency_source_addrs_;
    // list of statements containing dependencies that can not be checked yet
    std::unordered_map<post_stmt_ptr, std::optional<address>> postponed_stmts_;

    ordinary_assembly_context& sym_ctx_;

    bool check_cycle(dependant target, std::vector<dependant> dependencies);

    void resolve_dependant(dependant target,
        const resolvable* dep_src,
        loctr_dependency_resolver* resolver,
        std::optional<address> loctr_addr);
    void resolve_dependant_default(dependant target);
    void resolve(loctr_dependency_resolver* resolver);

    std::vector<dependant> extract_dependencies(const resolvable* dependency_source, std::optional<address> loctr_addr);
    std::vector<dependant> extract_dependencies(
        const std::vector<const resolvable*>& dependency_sources, std::optional<address> loctr_addr);

    void try_erase_source_statement(dependant index);

    bool add_dependency(
        dependant target, const resolvable* dependency_source, bool check_cycle, std::optional<address> loctr_addr);

public:
    symbol_dependency_tables(ordinary_assembly_context& sym_ctx);

    // add symbol dependency on statement
    // returns false if cyclic dependency occured
    [[nodiscard]] bool add_dependency(id_index target,
        const resolvable* dependency_source,
        post_stmt_ptr dependency_source_stmt,
        std::optional<context::address> loctr_addr);

    // add symbol attribute dependency on statement
    // returns false if cyclic dependency occured
    [[nodiscard]] bool add_dependency(id_index target,
        data_attr_kind attr,
        const resolvable* dependency_source,
        post_stmt_ptr dependency_source_stmt,
        std::optional<context::address> loctr_addr);

    // add space dependency
    void add_dependency(space_ptr target,
        const resolvable* dependency_source,
        post_stmt_ptr dependency_source_stmt,
        std::optional<context::address> loctr_addr);
    void add_dependency(space_ptr target,
        addr_res_ptr dependency_source,
        std::optional<context::address> loctr_addr,
        post_stmt_ptr dependency_source_stmt = nullptr);
    bool check_cycle(space_ptr target);

    // add statement dependency on its operands
    void add_dependency(post_stmt_ptr target, std::optional<context::address> loctr_addr);

    // method for creating more than one dependency assigned to one statement
    dependency_adder add_dependencies(post_stmt_ptr dependency_source_stmt, std::optional<context::address> loctr_addr);

    // registers that some symbol has been defined
    // if resolver is present, location counter dependencies are checked as well (not just symbol deps)
    void add_defined(loctr_dependency_resolver* resolver = nullptr);

    // checks for cycle in location counter value
    bool check_loctr_cycle();

    // collect all postponed statements either if they still contain dependent objects
    std::vector<std::pair<post_stmt_ptr, std::optional<address>>> collect_postponed();

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
    std::optional<context::address> loctr_addr;

public:
    post_stmt_ptr source_stmt;

    dependency_adder(symbol_dependency_tables& owner,
        post_stmt_ptr dependency_source_stmt,
        std::optional<context::address> loctr_addr);

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
