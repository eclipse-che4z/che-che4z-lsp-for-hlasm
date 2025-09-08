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

#include <memory>
#include <optional>
#include <unordered_map>
#include <variant>
#include <vector>

#include "alignment.h"
#include "dependable.h"
#include "diagnostic_consumer.h"
#include "section.h"
#include "symbol.h"
#include "tagged_index.h"

namespace hlasm_plugin::parser_library {
class diagnosable_ctx;
class library_info;
} // namespace hlasm_plugin::parser_library

namespace hlasm_plugin::parser_library::expressions {
struct data_definition;
} // namespace hlasm_plugin::parser_library::expressions

namespace hlasm_plugin::parser_library::context {
struct dependency_evaluation_context;
class hlasm_context;
class literal_pool;
class location_counter;
class opcode_generation;
struct postponed_statement;
class symbol_dependency_tables;
class using_collection;

struct using_label_tag
{};
struct macro_label_tag
{};

// class holding complete information about the 'ordinary assembly' (assembler and machine instructions)
// it contains 'sections' ordinary 'symbols' and all dependencies between them
class ordinary_assembly_context
{
    // list of visited sections
    std::vector<std::unique_ptr<section>> sections_;
    // list of visited symbols
    std::unordered_map<id_index, std::variant<symbol, using_label_tag, macro_label_tag>> symbols_;
    // list of lookaheaded symbols
    std::unordered_map<id_index, symbol> symbol_refs_;

    // ids that were mentioned as macro labels and could have been symbols
    bool reporting_candidates = false;

    section* curr_section_;
    section* first_control_section_ = nullptr;
    section* last_active_control_section = nullptr;

    // literals
    std::unique_ptr<literal_pool> m_literals;
    size_t m_statement_unique_id = 1;

    hlasm_context& hlasm_ctx_;

    std::unique_ptr<symbol_dependency_tables> m_symbol_dependencies;

    section* ensure_current_section();
    location_counter& loctr();

public:
    // access sections
    const std::vector<std::unique_ptr<section>>& sections() const;

    // access symbols
    const auto& symbols() const { return symbols_; }

    // access symbol dependency table
    symbol_dependency_tables& symbol_dependencies() { return *m_symbol_dependencies; }

    explicit ordinary_assembly_context(hlasm_context& hlasm_ctx);
    ordinary_assembly_context(ordinary_assembly_context&&) noexcept;
    ~ordinary_assembly_context();

    [[nodiscard]] symbol& create_symbol(id_index name, symbol_value value, symbol_attributes attributes);

    void add_symbol_reference(id_index name, symbol_attributes attributes, const library_info& li);
    const symbol* get_symbol_reference(context::id_index name) const;

    symbol* get_symbol(id_index name);
    const symbol* get_symbol(id_index name) const;

    // gets section by name
    section* get_section(id_index name) const noexcept;

    // access current section
    const section* current_section() const;

    // sets current section
    section* set_section(id_index name, section_kind kind, const library_info& li);
    section* create_and_set_class(id_index name, const library_info& li, section* base, bool partitioned);
    section* set_section(section& s);

    // creates an external section
    void create_external_section(id_index name, section_kind kind, std::optional<position> pos = std::nullopt);

    // sets current location counter of current section
    void set_location_counter(id_index name, const library_info& li);
    void set_location_counter(location_counter& l);

    // sets value of the current location counter
    void set_location_counter_value(size_t boundary,
        int offset,
        const resolvable& undefined_address,
        std::unique_ptr<postponed_statement> dependency_source,
        const dependency_evaluation_context& dep_ctx);
    void set_location_counter_value(const address& addr, size_t boundary, int offset);
    space_ptr set_location_counter_value_space(
        const address& addr, size_t boundary, int offset, const dependency_evaluation_context& dep_ctx);

    // sets next available value for the current location counter
    void set_available_location_counter_value();

    // check whether symbol is already defined
    bool symbol_defined(id_index name) const;
    // check whether section is already defined
    bool section_defined(id_index name, section_kind kind) const;
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
    void finish_module_layout(diagnostic_consumer* diag_consumer, const library_info& li);

    size_t current_literal_pool_generation() const;
    size_t next_unique_id() { return ++m_statement_unique_id; }
    size_t current_unique_id() const { return m_statement_unique_id; }

    literal_pool& literals() { return *m_literals; }
    const literal_pool& literals() const { return *m_literals; }
    void generate_pool(diagnosable_ctx& diags, index_t<using_collection> active_using, const library_info& li) const;
    location_counter& implicit_ltorg_target()
    {
        if (!first_control_section_)
            create_private_section();

        return first_control_section_->current_location_counter();
    }

    bool is_using_label(id_index name) const;
    void register_using_label(id_index name);

    index_t<using_collection> current_using() const;
    bool using_label_active(index_t<using_collection> context_id, id_index label, const section* sect) const;

    void symbol_mentioned_on_macro(id_index name);
    void start_reporting_label_candidates();

    opcode_generation current_opcode_generation() const;

    section* get_last_active_control_section() const { return last_active_control_section; }

private:
    void create_private_section();
    std::pair<address, space_ptr> reserve_storage_area_space(
        size_t length, alignment align, const dependency_evaluation_context& dep_ctx);
    section* create_section(id_index name, section_kind kind);
    section* create_section(id_index name, section_kind kind, goff_details details);

    friend class ordinary_assembly_dependency_solver;
};

} // namespace hlasm_plugin::parser_library::context

#endif
