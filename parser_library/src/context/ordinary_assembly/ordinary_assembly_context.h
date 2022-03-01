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
#include <variant>

#include "alignment.h"
#include "dependable.h"
#include "diagnostic_consumer.h"
#include "location_counter.h"
#include "loctr_dependency_resolver.h"
#include "section.h"
#include "symbol.h"
#include "symbol_dependency_tables.h"

namespace hlasm_plugin::parser_library {
class diagnosable_ctx;
} // namespace hlasm_plugin::parser_library

namespace hlasm_plugin::parser_library::expressions {
struct data_definition;
} // namespace hlasm_plugin::parser_library::expressions

namespace hlasm_plugin::parser_library::context {
class hlasm_context;
class literal_pool;

struct label_tag
{};

// class holding complete information about the 'ordinary assembly' (assembler and machine instructions)
// it contains 'sections' ordinary 'symbols' and all dependencies between them
class ordinary_assembly_context
{
    // list of visited sections
    std::vector<std::unique_ptr<section>> sections_;
    // list of visited symbols
    std::unordered_map<id_index, std::variant<symbol, label_tag>> symbols_;
    // list of lookaheaded symbols
    std::unordered_map<id_index, symbol> symbol_refs_;

    section* curr_section_;
    section* first_control_section_ = nullptr;

    // literals
    std::unique_ptr<literal_pool> m_literals;
    size_t m_statement_unique_id = 1;

    // access id storage
    id_storage& ids;
    hlasm_context& hlasm_ctx_;

public:
    // access sections
    const std::vector<std::unique_ptr<section>>& sections() const;

    // access symbols
    const std::unordered_map<id_index, std::variant<symbol, label_tag>>& symbols() const;

    // access symbol dependency table
    symbol_dependency_tables symbol_dependencies;

    ordinary_assembly_context(id_storage& storage, hlasm_context& hlasm_ctx);
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

    size_t current_literal_pool_generation() const;
    size_t next_unique_id() { return ++m_statement_unique_id; }
    size_t current_unique_id() const { return m_statement_unique_id; }

    literal_pool& literals() { return *m_literals; }
    const literal_pool& literals() const { return *m_literals; }
    void generate_pool(diagnosable_ctx& diags) const;
    location_counter* implicit_ltorg_target()
    {
        if (!first_control_section_)
            create_private_section();

        return &first_control_section_->current_location_counter();
    }

    bool is_using_label(id_index name) const;
    void register_using_label(id_index name);

private:
    void create_private_section();
    std::pair<address, space_ptr> reserve_storage_area_space(
        size_t length, alignment align, const dependency_evaluation_context& dep_ctx);
    section* create_section(id_index name, section_kind kind);

    friend class ordinary_assembly_dependency_solver;
};

} // namespace hlasm_plugin::parser_library::context

#endif