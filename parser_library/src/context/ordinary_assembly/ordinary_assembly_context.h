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

#ifndef CONTEXT_ORDINARY_ASSEMBLY_CONTEXT_H
#define CONTEXT_ORDINARY_ASSEMBLY_CONTEXT_H

#include <unordered_map>

#include "alignment.h"
#include "dependable.h"
#include "diagnostic_consumer.h"
#include "location_counter.h"
#include "loctr_dependency_resolver.h"
#include "section.h"
#include "symbol.h"
#include "symbol_dependency_tables.h"

namespace hlasm_plugin::parser_library::expressions {
struct data_definition;
} // namespace hlasm_plugin::parser_library::expressions

namespace hlasm_plugin::parser_library::context {
class hlasm_context;
class literal_pool;

// class holding complete information about the 'ordinary assembly' (assembler and machine instructions)
// it contains 'sections' ordinary 'symbols' and all dependencies between them
class ordinary_assembly_context
{
    // list of visited sections
    std::vector<std::unique_ptr<section>> sections_;
    // list of visited symbols
    std::unordered_map<id_index, symbol> symbols_;
    // list of lookaheaded symbols
    std::unordered_map<id_index, symbol> symbol_refs_;

    section* curr_section_;
    section* first_section_ = nullptr;

    // literals
    std::unique_ptr<literal_pool> m_literals;
    size_t m_statement_unique_id = 1;

    // access id storage
    id_storage& ids;
    const hlasm_context& hlasm_ctx_;

public:
    // access sections
    const std::vector<std::unique_ptr<section>>& sections() const;

    // access symbols
    const std::unordered_map<id_index, symbol>& symbols() const;

    // access symbol dependency table
    symbol_dependency_tables symbol_dependencies;

    ordinary_assembly_context(id_storage& storage, const hlasm_context& hlasm_ctx);
    ordinary_assembly_context(ordinary_assembly_context&&) noexcept;
    ~ordinary_assembly_context();

    // creates symbol
    // returns false if loctr cycle has occured
    [[nodiscard]] bool create_symbol(
        id_index name, symbol_value value, symbol_attributes attributes, location symbol_location);

    void add_symbol_reference(symbol sym);
    const symbol* get_symbol_reference(context::id_index name) const;

    symbol* get_symbol(id_index name);

    // gets section by name
    section* get_section(id_index name);

    // access current section
    const section* current_section() const;

    // sets current section
    void set_section(id_index name, section_kind kind, location symbol_location);

    // creates an external section
    void create_external_section(
        id_index name, section_kind kind, location symbol_location, processing_stack_t processing_stack);

    // sets current location counter of current section
    void set_location_counter(id_index name, location symbol_location);

    // sets value of the current location counter
    void set_location_counter_value(const address& addr,
        size_t boundary,
        int offset,
        const resolvable* undefined_address,
        post_stmt_ptr dependency_source,
        const dependency_evaluation_context& dep_ctx);
    void set_location_counter_value(const address& addr, size_t boundary, int offset);
    space_ptr set_location_counter_value_space(const address& addr,
        size_t boundary,
        int offset,
        const resolvable* undefined_address,
        post_stmt_ptr dependency_source,
        const dependency_evaluation_context& dep_ctx);

    // sets next available value for the current location counter
    void set_available_location_counter_value(
        size_t boundary, int offset, const dependency_evaluation_context& dep_ctx);
    void set_available_location_counter_value(size_t boundary, int offset);

    // check whether symbol is already defined
    bool symbol_defined(id_index name);
    // check whether section is already defined
    bool section_defined(id_index name, section_kind kind);
    // check whether location counter is already defined
    bool counter_defined(id_index name);

    // reserves storage area of specified length and alignment
    address reserve_storage_area(size_t length, alignment align, const dependency_evaluation_context& dep_ctx);
    address reserve_storage_area(size_t length, alignment align);

    // aligns storage
    address align(alignment align, const dependency_evaluation_context& dep_ctx);
    address align(alignment a);

    // adds space to the current location counter
    space_ptr register_ordinary_space(alignment align);

    // creates layout of every section
    void finish_module_layout(loctr_dependency_resolver* resolver);

    const std::unordered_map<id_index, symbol>& get_all_symbols();

    size_t current_literal_pool_generation() const;
    size_t next_unique_id() { return m_statement_unique_id++; }

    const literal_pool& literals() const { return *m_literals; }
    void generate_pool(dependency_solver& solver, diagnostic_op_consumer& diags);
    location_counter* implicit_ltorg_target() const
    {
        if (!first_section_)
            return nullptr;
        return &first_section_->current_location_counter();
    }

private:
    void create_private_section();
    std::pair<address, space_ptr> reserve_storage_area_space(
        size_t length, alignment align, const dependency_evaluation_context& dep_ctx);
    section* create_section(id_index name, section_kind kind);

    friend class ordinary_assembly_dependency_solver;
};

class ordinary_assembly_dependency_solver final : public dependency_solver
{
    ordinary_assembly_context& ord_context;
    std::optional<context::address> loctr_addr;
    size_t literal_pool_generation = (size_t)-1;
    size_t unique_id = 0;
    bool allow_adding_literals = false;

public:
    explicit ordinary_assembly_dependency_solver(ordinary_assembly_context& ord_context)
        : ord_context(ord_context)
        , literal_pool_generation(ord_context.current_literal_pool_generation())
        , unique_id(ord_context.next_unique_id())
        , allow_adding_literals(true)
    {}
    struct no_new_literals
    {};
    ordinary_assembly_dependency_solver(ordinary_assembly_context& ord_context, no_new_literals)
        : ord_context(ord_context)
    {}

    ordinary_assembly_dependency_solver(ordinary_assembly_context& ord_context, context::address loctr_addr)
        : ord_context(ord_context)
        , loctr_addr(std::move(loctr_addr))
        , literal_pool_generation(ord_context.current_literal_pool_generation())
        , unique_id(ord_context.next_unique_id())
        , allow_adding_literals(true)
    {}

    ordinary_assembly_dependency_solver(
        ordinary_assembly_context& ord_context, const dependency_evaluation_context& dep_ctx)
        : ord_context(ord_context)
        , loctr_addr(dep_ctx.loctr_address)
        , literal_pool_generation(dep_ctx.literal_pool_generation)
        , unique_id(dep_ctx.unique_id)
    {}

    const symbol* get_symbol(id_index name) const override;
    std::optional<address> get_loctr() const override;
    id_index get_literal_id(
        const std::string& text, const std::shared_ptr<const expressions::data_definition>& lit) override;

    dependency_evaluation_context derive_current_dependency_evaluation_context() const;
};

} // namespace hlasm_plugin::parser_library::context

#endif