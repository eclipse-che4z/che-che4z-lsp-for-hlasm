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
#include <stdexcept>

#include "alignment.h"
#include "context/hlasm_context.h"
#include "context/literal_pool.h"

namespace hlasm_plugin::parser_library::context {

void ordinary_assembly_context::create_private_section()
{
    curr_section_ = create_section(id_storage::empty_id, section_kind::EXECUTABLE);
}

const std::vector<std::unique_ptr<section>>& ordinary_assembly_context::sections() const { return sections_; }

const std::unordered_map<id_index, symbol>& ordinary_assembly_context::symbols() const { return symbols_; }

ordinary_assembly_context::ordinary_assembly_context(id_storage& storage, const hlasm_context& hlasm_ctx)
    : curr_section_(nullptr)
    , m_literals(std::make_unique<literal_pool>())
    , ids(storage)
    , hlasm_ctx_(hlasm_ctx)
    , symbol_dependencies(*this)
{}
ordinary_assembly_context::ordinary_assembly_context(ordinary_assembly_context&&) noexcept = default;
ordinary_assembly_context::~ordinary_assembly_context() = default;

bool ordinary_assembly_context::create_symbol(
    id_index name, symbol_value value, symbol_attributes attributes, location symbol_location)
{
    auto res =
        symbols_.try_emplace(name, name, value, attributes, std::move(symbol_location), hlasm_ctx_.processing_stack());

    if (!res.second)
        throw std::runtime_error("symbol name in use");

    bool ok = true;

    if (value.value_kind() == symbol_value_kind::RELOC)
        ok = symbol_dependencies.check_loctr_cycle();

    if (value.value_kind() != symbol_value_kind::UNDEF)
        symbol_dependencies.add_defined();

    return ok;
}

void ordinary_assembly_context::add_symbol_reference(symbol sym) { symbol_refs_.try_emplace(sym.name, std::move(sym)); }

const symbol* ordinary_assembly_context::get_symbol_reference(context::id_index name) const
{
    auto tmp = symbol_refs_.find(name);

    return tmp == symbol_refs_.end() ? nullptr : &tmp->second;
}

symbol* ordinary_assembly_context::get_symbol(id_index name)
{
    auto tmp = symbols_.find(name);

    return tmp == symbols_.end() ? nullptr : &tmp->second;
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

void ordinary_assembly_context::set_section(id_index name, section_kind kind, location symbol_location)
{
    auto tmp = std::find_if(sections_.begin(), sections_.end(), [name, kind](const auto& sect) {
        return sect->name == name && sect->kind == kind;
    });

    if (tmp != sections_.end())
        curr_section_ = &**tmp;
    else
    {
        if (symbols_.find(name) != symbols_.end())
            throw std::invalid_argument("symbol already defined");

        curr_section_ = create_section(name, kind);

        auto tmp_addr = curr_section_->current_location_counter().current_address();
        symbols_.try_emplace(name,
            name,
            tmp_addr,
            symbol_attributes::make_section_attrs(),
            std::move(symbol_location),
            hlasm_ctx_.processing_stack());
    }
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
            default:
                throw std::invalid_argument("section type mismatch");
        }
    }();

    if (!symbols_
             .try_emplace(name,
                 name,
                 create_section(name, kind)->current_location_counter().current_address(),
                 attrs,
                 std::move(symbol_location),
                 processing_stack)
             .second)
        throw std::invalid_argument("symbol already defined");
}

void ordinary_assembly_context::set_location_counter(id_index name, location symbol_location)
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

        auto sym_tmp = symbols_.try_emplace(name,
            name,
            tmp_addr,
            symbol_attributes::make_section_attrs(),
            std::move(symbol_location),
            hlasm_ctx_.processing_stack());
        if (!sym_tmp.second)
            throw std::invalid_argument("symbol already defined");
    }
}

void ordinary_assembly_context::set_location_counter_value(const address& addr,
    size_t boundary,
    int offset,
    const resolvable* undefined_address,
    post_stmt_ptr dependency_source,
    const dependency_evaluation_context& dep_ctx)
{
    (void)set_location_counter_value_space(
        addr, boundary, offset, undefined_address, std::move(dependency_source), dep_ctx);
}

void ordinary_assembly_context::set_location_counter_value(const address& addr, size_t boundary, int offset)
{
    set_location_counter_value(addr, boundary, offset, nullptr, nullptr, dependency_evaluation_context {});
}

space_ptr ordinary_assembly_context::set_location_counter_value_space(const address& addr,
    size_t boundary,
    int offset,
    const resolvable* undefined_address,
    post_stmt_ptr dependency_source,
    const dependency_evaluation_context& dep_ctx)
{
    if (!curr_section_)
        create_private_section();

    address curr_addr = curr_section_->current_location_counter().current_address();
    auto sp = curr_section_->current_location_counter().set_value(addr, boundary, offset, undefined_address);

    if (sp)
    {
        if (!undefined_address)
            symbol_dependencies.add_dependency(sp,
                std::make_unique<alignable_address_resolver>(
                    std::move(curr_addr), std::vector<address> { addr }, boundary, offset),
                dep_ctx);
        else
            symbol_dependencies.add_dependency(sp,
                std::make_unique<alignable_address_abs_part_resolver>(undefined_address),
                dep_ctx,
                std::move(dependency_source));
        return sp;
    }
    else
    {
        return reserve_storage_area_space(offset, alignment { 0, boundary ? boundary : 1 }, dep_ctx).second;
    }
}

void ordinary_assembly_context::set_available_location_counter_value(
    size_t boundary, int offset, const dependency_evaluation_context& dep_ctx)
{
    if (!curr_section_)
        create_private_section();

    auto [sp, addr] = curr_section_->current_location_counter().set_available_value();

    if (sp)
        symbol_dependencies.add_dependency(
            sp, std::make_unique<aggregate_address_resolver>(std::move(addr), boundary, offset), dep_ctx);
    else
    {
        if (boundary)
            (void)align(alignment { 0, boundary }, dep_ctx);
        (void)reserve_storage_area(offset, context::no_align, dep_ctx);
    }
}
void ordinary_assembly_context::set_available_location_counter_value(size_t boundary, int offset)
{
    set_available_location_counter_value(boundary, offset, dependency_evaluation_context {});
}

bool ordinary_assembly_context::symbol_defined(id_index name) { return symbols_.find(name) != symbols_.end(); }

bool ordinary_assembly_context::section_defined(id_index name, section_kind kind)
{
    return std::find_if(sections_.begin(), sections_.end(), [name, kind](const auto& sect) {
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
    size_t length, alignment align, const dependency_evaluation_context& dep_ctx)
{
    return reserve_storage_area_space(length, align, dep_ctx).first;
}

address ordinary_assembly_context::reserve_storage_area(size_t length, alignment align)
{
    return reserve_storage_area(length, align, dependency_evaluation_context {});
}

address ordinary_assembly_context::align(alignment align, const dependency_evaluation_context& dep_ctx)
{
    return reserve_storage_area(0, align, dep_ctx);
}

address ordinary_assembly_context::align(alignment a) { return align(a, dependency_evaluation_context {}); }

space_ptr ordinary_assembly_context::register_ordinary_space(alignment align)
{
    if (!curr_section_)
        create_private_section();

    return curr_section_->current_location_counter().register_ordinary_space(align);
}

void ordinary_assembly_context::finish_module_layout(loctr_dependency_resolver* resolver)
{
    for (auto& sect : sections_)
    {
        for (size_t i = 0; i < sect->location_counters().size(); ++i)
        {
            if (i == 0)
                sect->location_counters()[0]->finish_layout(0);
            else
            {
                if (sect->location_counters()[i - 1]->has_unresolved_spaces())
                    return;

                sect->location_counters()[i]->finish_layout(sect->location_counters()[i - 1]->storage());
                symbol_dependencies.add_defined(resolver);
            }
        }
    }
}

const std::unordered_map<id_index, symbol>& ordinary_assembly_context::get_all_symbols() { return symbols_; }

std::pair<address, space_ptr> ordinary_assembly_context::reserve_storage_area_space(
    size_t length, alignment align, const dependency_evaluation_context& dep_ctx)
{
    if (!curr_section_)
        create_private_section();

    if (curr_section_->current_location_counter().need_space_alignment(align))
    {
        address addr = curr_section_->current_location_counter().current_address();
        auto [ret_addr, sp] = curr_section_->current_location_counter().reserve_storage_area(length, align);
        assert(sp);

        symbol_dependencies.add_dependency(sp, std::make_unique<address_resolver>(addr), dep_ctx);
        return std::make_pair(ret_addr, sp);
    }
    return std::make_pair(curr_section_->current_location_counter().reserve_storage_area(length, align).first, nullptr);
}

section* ordinary_assembly_context::create_section(id_index name, section_kind kind)
{
    section* ret = sections_.emplace_back(std::make_unique<section>(name, kind, ids)).get();
    if (first_section_ == nullptr)
        first_section_ = ret;
    return ret;
}


size_t ordinary_assembly_context::current_literal_pool_generation() const { return m_literals->current_generation(); }

void ordinary_assembly_context::generate_pool(dependency_solver& solver) { m_literals->generate_pool(*this, solver); }

const symbol* ordinary_assembly_dependency_solver::get_symbol(id_index name) const
{
    auto tmp = ord_context.symbols_.find(name);

    return tmp == ord_context.symbols_.end() ? nullptr : &tmp->second;
}

std::optional<address> ordinary_assembly_dependency_solver::get_loctr() const { return loctr_addr; }

id_index ordinary_assembly_dependency_solver::get_literal_id(
    const std::string& text, const std::shared_ptr<const expressions::data_definition>& lit)
{
    if (allow_adding_literals)
        return ord_context.m_literals->add_literal(text, lit, {}, unique_id);
    else
        return ord_context.m_literals->get_literal(literal_pool_generation, lit, unique_id);
}


dependency_evaluation_context ordinary_assembly_dependency_solver::derive_current_dependency_evaluation_context() const
{
    return dependency_evaluation_context {
        loctr_addr,
        literal_pool_generation,
        unique_id,
    };
}

} // namespace hlasm_plugin::parser_library::context
