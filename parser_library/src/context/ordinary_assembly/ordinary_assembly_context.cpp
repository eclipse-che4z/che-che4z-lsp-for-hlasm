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

#include "ordinary_assembly_context.h"

#include <algorithm>
#include <cassert>
#include <memory>

#include "alignment.h"
#include "context/hlasm_context.h"
#include "context/literal_pool.h"
#include "context/using.h"
#include "location_counter.h"
#include "symbol_dependency_tables.h"

namespace hlasm_plugin::parser_library::context {

bool symbol_can_be_assigned(const auto& symbols, auto name)
{
    auto it = symbols.find(name);
    return it == symbols.end() || std::holds_alternative<macro_label_tag>(it->second);
}

void ordinary_assembly_context::create_private_section()
{
    curr_section_ = create_section(id_index(), section_kind::EXECUTABLE);
}

const std::vector<std::unique_ptr<section>>& ordinary_assembly_context::sections() const { return sections_; }

ordinary_assembly_context::ordinary_assembly_context(hlasm_context& hlasm_ctx)
    : curr_section_(nullptr)
    , m_literals(std::make_unique<literal_pool>(hlasm_ctx))
    , hlasm_ctx_(hlasm_ctx)
    , m_symbol_dependencies(std::make_unique<symbol_dependency_tables>(*this))
{}
ordinary_assembly_context::ordinary_assembly_context(ordinary_assembly_context&&) noexcept = default;
ordinary_assembly_context::~ordinary_assembly_context() = default;

bool ordinary_assembly_context::create_symbol(
    id_index name, symbol_value value, symbol_attributes attributes, location symbol_location, const library_info& li)
{
    assert(symbol_can_be_assigned(symbols_, name));

    const auto value_kind = value.value_kind();

    symbols_.insert_or_assign(
        name, symbol(name, std::move(value), attributes, std::move(symbol_location), hlasm_ctx_.processing_stack()));

    bool ok = true;

    if (value_kind != symbol_value_kind::UNDEF)
        m_symbol_dependencies->add_defined(name, nullptr, li);

    return ok;
}

void ordinary_assembly_context::add_symbol_reference(symbol sym, const library_info& li)
{
    auto [it, _] = symbol_refs_.try_emplace(sym.name(), std::move(sym));
    m_symbol_dependencies->add_defined(it->first, nullptr, li);
}

const symbol* ordinary_assembly_context::get_symbol_reference(context::id_index name) const
{
    auto tmp = symbol_refs_.find(name);

    return tmp == symbol_refs_.end() ? nullptr : &tmp->second;
}

symbol* ordinary_assembly_context::get_symbol(id_index name)
{
    auto tmp = symbols_.find(name);

    return tmp == symbols_.end() ? nullptr : std::get_if<symbol>(&tmp->second);
}

const symbol* ordinary_assembly_context::get_symbol(id_index name) const
{
    auto tmp = symbols_.find(name);

    return tmp == symbols_.end() ? nullptr : std::get_if<symbol>(&tmp->second);
}

section* ordinary_assembly_context::get_section(id_index name)
{
    for (auto& tmp : sections_)
    {
        if (tmp->name == name)
        {
            return &(*tmp);
        }
    }
    return nullptr;
}

const section* ordinary_assembly_context::current_section() const { return curr_section_; }

section* ordinary_assembly_context::set_section(
    id_index name, section_kind kind, location symbol_location, const library_info& li)
{
    auto tmp = std::ranges::find_if(
        sections_, [name, kind](const auto& sect) { return sect->name == name && sect->kind == kind; });

    if (tmp != sections_.end())
        curr_section_ = std::to_address(*tmp);
    else
    {
        curr_section_ = create_section(name, kind);

        auto tmp_addr = curr_section_->current_location_counter().current_address();
        if (!name.empty())
        {
            assert(symbol_can_be_assigned(symbols_, name));
            symbols_.insert_or_assign(name,
                symbol(name,
                    tmp_addr,
                    symbol_attributes::make_section_attrs(),
                    std::move(symbol_location),
                    hlasm_ctx_.processing_stack()));
            m_symbol_dependencies->add_defined(name, nullptr, li);
        }
    }

    return curr_section_;
}

void ordinary_assembly_context::create_external_section(
    id_index name, section_kind kind, location symbol_location, processing_stack_t processing_stack)
{
    const auto attrs = [kind]() {
        switch (kind)
        {
            case section_kind::EXTERNAL:
                return symbol_attributes::make_extrn_attrs();
            case section_kind::WEAK_EXTERNAL:
                return symbol_attributes::make_wxtrn_attrs();
        }
        assert(false);
    }();


    assert(symbol_can_be_assigned(symbols_, name));

    symbols_.insert_or_assign(name,
        symbol(name,
            create_section(name, kind)->current_location_counter().current_address(),
            attrs,
            std::move(symbol_location),
            std::move(processing_stack)));
}

void ordinary_assembly_context::set_location_counter(id_index name, location symbol_location, const library_info& li)
{
    if (!curr_section_)
        create_private_section();

    bool defined = false;

    for (const auto& sect : sections_)
    {
        if (sect->counter_defined(name))
        {
            curr_section_ = sect.get();
            defined = true;
        }
    }

    curr_section_->set_location_counter(name);

    if (!defined)
    {
        auto tmp_addr = curr_section_->current_location_counter().current_address();

        assert(symbol_can_be_assigned(symbols_, name));
        symbols_.insert_or_assign(name,
            symbol(name,
                tmp_addr,
                symbol_attributes::make_section_attrs(),
                std::move(symbol_location),
                hlasm_ctx_.processing_stack()));

        m_symbol_dependencies->add_defined(name, nullptr, li);
    }
}

void ordinary_assembly_context::set_location_counter_value(const address& addr,
    size_t boundary,
    int offset,
    const resolvable* undefined_address,
    post_stmt_ptr dependency_source,
    const dependency_evaluation_context& dep_ctx,
    const library_info& li)
{
    (void)set_location_counter_value_space(
        addr, boundary, offset, undefined_address, std::move(dependency_source), dep_ctx, li);
}

void ordinary_assembly_context::set_location_counter_value(
    const address& addr, size_t boundary, int offset, const library_info& li)
{
    set_location_counter_value(
        addr, boundary, offset, nullptr, nullptr, dependency_evaluation_context(current_opcode_generation()), li);
}

space_ptr ordinary_assembly_context::set_location_counter_value_space(const address& addr,
    size_t boundary,
    int offset,
    const resolvable* undefined_address,
    post_stmt_ptr dependency_source,
    const dependency_evaluation_context& dep_ctx,
    const library_info& li)
{
    if (!curr_section_)
        create_private_section();

    if (undefined_address)
    {
        auto sp = curr_section_->current_location_counter().set_value_undefined(boundary, offset);
        m_symbol_dependencies->add_dependency(sp,
            std::make_unique<alignable_address_abs_part_resolver>(undefined_address),
            dep_ctx,
            li,
            std::move(dependency_source));
        return sp;
    }


    if (auto [curr_addr, sp] = curr_section_->current_location_counter().set_value(addr, boundary, offset); sp)
    {
        m_symbol_dependencies->add_dependency(sp,
            std::make_unique<alignable_address_resolver>(
                std::move(curr_addr), std::vector<address> { addr }, boundary, offset),
            dep_ctx,
            li);
        return sp;
    }

    return reserve_storage_area_space(offset, alignment { 0, boundary ? boundary : 1 }, dep_ctx, li).second;
}

void ordinary_assembly_context::set_available_location_counter_value(const library_info& li)
{
    if (!curr_section_)
        create_private_section();

    auto [sp, addr] = curr_section_->current_location_counter().set_available_value();

    if (sp)
        m_symbol_dependencies->add_dependency(sp,
            std::make_unique<aggregate_address_resolver>(std::move(addr), 0, 0),
            dependency_evaluation_context(current_opcode_generation()),
            li);
}

bool ordinary_assembly_context::symbol_defined(id_index name) const
{
    auto it = symbols_.find(name);
    return it != symbols_.end() && !std::holds_alternative<macro_label_tag>(it->second);
}

bool ordinary_assembly_context::section_defined(id_index name, section_kind kind) const
{
    return std::ranges::find_if(sections_, [name, kind](const auto& sect) {
        return sect->name == name && sect->kind == kind;
    }) != sections_.end();
}

bool ordinary_assembly_context::counter_defined(id_index name)
{
    if (!curr_section_)
        create_private_section();

    for (const auto& sect : sections_)
    {
        if (sect->counter_defined(name))
            return true;
    }
    return false;
}

address ordinary_assembly_context::reserve_storage_area(
    size_t length, alignment align, const dependency_evaluation_context& dep_ctx, const library_info& li)
{
    return reserve_storage_area_space(length, align, dep_ctx, li).first;
}

address ordinary_assembly_context::reserve_storage_area(size_t length, alignment align, const library_info& li)
{
    return reserve_storage_area(length, align, dependency_evaluation_context(current_opcode_generation()), li);
}

address ordinary_assembly_context::align(
    alignment align, const dependency_evaluation_context& dep_ctx, const library_info& li)
{
    return reserve_storage_area(0, align, dep_ctx, li);
}

address ordinary_assembly_context::align(alignment a, const library_info& li)
{
    return align(a, dependency_evaluation_context(current_opcode_generation()), li);
}

space_ptr ordinary_assembly_context::register_ordinary_space(alignment align)
{
    if (!curr_section_)
        create_private_section();

    return curr_section_->current_location_counter().register_ordinary_space(align);
}

void ordinary_assembly_context::finish_module_layout(diagnostic_consumer* diag_consumer, const library_info& li)
{
    for (auto& sect : sections_)
    {
        size_t last_offset = 0;
        for (const auto& loctr : sect->location_counters())
        {
            symbol_dependencies().add_defined(loctr->finish_layout(last_offset), diag_consumer, li);
            if (loctr->has_unresolved_spaces())
                return;
            last_offset = loctr->storage();
        }
    }
}

std::pair<address, space_ptr> ordinary_assembly_context::reserve_storage_area_space(
    size_t length, alignment align, const dependency_evaluation_context& dep_ctx, const library_info& li)
{
    if (!curr_section_)
        create_private_section();

    if (curr_section_->current_location_counter().need_space_alignment(align))
    {
        address addr = curr_section_->current_location_counter().current_address_for_alignment_evaluation(align);
        auto [ret_addr, sp] = curr_section_->current_location_counter().reserve_storage_area(length, align);
        assert(sp);

        m_symbol_dependencies->add_dependency(
            sp, std::make_unique<address_resolver>(addr, align.boundary), dep_ctx, li);
        return std::make_pair(ret_addr, sp);
    }
    return std::make_pair(curr_section_->current_location_counter().reserve_storage_area(length, align).first, nullptr);
}

section* ordinary_assembly_context::create_section(id_index name, section_kind kind)
{
    section* ret = sections_.emplace_back(std::make_unique<section>(name, kind)).get();
    if (first_control_section_ == nullptr
        && (kind == section_kind::COMMON || kind == section_kind::EXECUTABLE || kind == section_kind::READONLY))
        first_control_section_ = ret;
    return ret;
}


size_t ordinary_assembly_context::current_literal_pool_generation() const { return m_literals->current_generation(); }

void ordinary_assembly_context::generate_pool(
    diagnosable_ctx& diags, index_t<using_collection> active_using, const library_info& li) const
{
    m_literals->generate_pool(diags, active_using, li);
}
bool ordinary_assembly_context::is_using_label(id_index name) const
{
    auto it = symbols_.find(name);
    return it != symbols_.end() && std::holds_alternative<using_label_tag>(it->second);
}

void ordinary_assembly_context::register_using_label(id_index name)
{
    assert(symbol_can_be_assigned(symbols_, name));

    symbols_.insert_or_assign(name, using_label_tag {});

    symbol_refs_.erase(name);
}

index_t<using_collection> ordinary_assembly_context::current_using() const { return hlasm_ctx_.using_current(); }

bool ordinary_assembly_context::using_label_active(
    index_t<using_collection> context_id, id_index label, const section* sect) const
{
    const auto& usings = hlasm_ctx_.usings();

    assert(usings.resolved());

    if (!is_using_label(label))
        return false;

    return usings.is_label_mapping_section(context_id, label, sect);
}

void ordinary_assembly_context::symbol_mentioned_on_macro(id_index name)
{
    symbols_.try_emplace(name, macro_label_tag {});
}

void ordinary_assembly_context::start_reporting_label_candidates() { reporting_candidates = true; }

opcode_generation ordinary_assembly_context::current_opcode_generation() const
{
    return hlasm_ctx_.current_opcode_generation();
}

} // namespace hlasm_plugin::parser_library::context
