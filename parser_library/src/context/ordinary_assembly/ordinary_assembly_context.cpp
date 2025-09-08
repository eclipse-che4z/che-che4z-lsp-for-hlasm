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
    set_section(*create_section(id_index(), section_kind::EXECUTABLE));
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

section* ordinary_assembly_context::ensure_current_section()
{
    if (!curr_section_)
        create_private_section();
    return curr_section_;
}

location_counter& ordinary_assembly_context::loctr()
{
    auto* s = ensure_current_section();
    return s->current_location_counter();
}

symbol& ordinary_assembly_context::create_symbol(id_index name, symbol_value value, symbol_attributes attributes)
{
    assert(symbol_can_be_assigned(symbols_, name));

    const auto [it, _] =
        symbols_.insert_or_assign(name, symbol(name, std::move(value), attributes, hlasm_ctx_.processing_stack()));

    return std::get<symbol>(it->second);
}

void ordinary_assembly_context::add_symbol_reference(
    id_index name, symbol_attributes attributes, const library_info& li)
{
    auto [it, _] = symbol_refs_.try_emplace(name, name, symbol_value(), attributes, processing_stack_t());
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

section* ordinary_assembly_context::get_section(id_index name) const noexcept
{
    for (auto& tmp : sections_)
    {
        if (tmp->name == name)
        {
            return std::to_address(tmp);
        }
    }
    return nullptr;
}

const section* ordinary_assembly_context::current_section() const { return curr_section_; }

section* ordinary_assembly_context::set_section(id_index name, section_kind kind, const library_info& li)
{
    auto tmp = std::ranges::find_if(
        sections_, [name, kind](const auto& sect) { return sect->name == name && sect->kind == kind; });

    section* s;

    if (tmp != sections_.end())
        s = set_section(**tmp);
    else
    {
        s = set_section(*create_section(name, kind));

        if (!name.empty())
        {
            assert(symbol_can_be_assigned(symbols_, name));
            symbols_.insert_or_assign(name,
                symbol(name,
                    s->current_location_counter().current_address(),
                    symbol_attributes::make_section_attrs(),
                    hlasm_ctx_.processing_stack()));
            m_symbol_dependencies->add_defined(name, nullptr, li);
        }
    }

    return s;
}

section* ordinary_assembly_context::create_and_set_class(
    id_index name, const library_info& li, section* base, bool partitioned)
{
    assert(std::ranges::find(sections_, name, &section::name) == sections_.end());
    assert(symbol_can_be_assigned(symbols_, name));

    auto* s = set_section(*create_section(name,
        section_kind::EXECUTABLE,
        {
            .owner = last_active_control_section,
            .parent = base,
            .partitioned = partitioned,
        }));
    symbols_.insert_or_assign(name,
        symbol(name,
            s->current_location_counter().current_address(),
            symbol_attributes::make_section_attrs(),
            hlasm_ctx_.processing_stack()));
    m_symbol_dependencies->add_defined(name, nullptr, li);

    return s;
}

section* ordinary_assembly_context::set_section(section& s)
{
    curr_section_ = &s;
    if (s.kind != section_kind::DUMMY)
    {
        if (s.goff.has_value())
            last_active_control_section = s.goff->owner;
        else
            last_active_control_section = &s;
    }

    return &s;
}

void ordinary_assembly_context::create_external_section(id_index name, section_kind kind, std::optional<position> pos)
{
    const auto attrs = [kind]() {
        using enum section_kind;
        switch (kind)
        {
            case EXTERNAL:
                return symbol_attributes::make_extrn_attrs();
            case WEAK_EXTERNAL:
                return symbol_attributes::make_wxtrn_attrs();
            case EXTERNAL_DSECT:
                return symbol_attributes::make_section_attrs();
        }
        assert(false);
    }();


    assert(symbol_can_be_assigned(symbols_, name));

    symbols_.insert_or_assign(name,
        symbol(name,
            create_section(name, kind)->current_location_counter().current_address(),
            attrs,
            hlasm_ctx_.processing_stack(),
            pos));
}

void ordinary_assembly_context::set_location_counter(id_index name, const library_info& li)
{
    ensure_current_section();

    location_counter* defined = nullptr;
    for (const auto& sect : sections_)
    {
        if ((defined = sect->find_location_counter(name)))
        {
            set_section(*sect);
            sect->set_location_counter(*defined);
            break;
        }
    }

    if (!defined)
    {
        auto& l = curr_section_->set_location_counter(name);
        assert(symbol_can_be_assigned(symbols_, name));
        symbols_.insert_or_assign(name,
            symbol(name, l.current_address(), symbol_attributes::make_section_attrs(), hlasm_ctx_.processing_stack()));

        m_symbol_dependencies->add_defined(name, nullptr, li);
    }
}

void ordinary_assembly_context::set_location_counter(location_counter& l)
{
    set_section(l.owner)->set_location_counter(l);
}

void ordinary_assembly_context::set_location_counter_value(size_t boundary,
    int offset,
    const resolvable& undefined_address,
    post_stmt_ptr dependency_source,
    const dependency_evaluation_context& dep_ctx)
{
    m_symbol_dependencies->add_dependency(loctr().set_value_undefined(boundary, offset),
        std::make_unique<alignable_address_abs_part_resolver>(&undefined_address),
        dep_ctx,
        std::move(dependency_source));
}

void ordinary_assembly_context::set_location_counter_value(const address& addr, size_t boundary, int offset)
{
    (void)set_location_counter_value_space(
        addr, boundary, offset, dependency_evaluation_context(current_opcode_generation()));
}

space_ptr ordinary_assembly_context::set_location_counter_value_space(
    const address& addr, size_t boundary, int offset, const dependency_evaluation_context& dep_ctx)
{
    if (auto [curr_addr, sp] = loctr().set_value(addr, boundary, offset); sp)
    {
        m_symbol_dependencies->add_dependency(
            sp, std::make_unique<alignable_address_resolver>(std::move(curr_addr), addr, boundary, offset), dep_ctx);
        return std::move(sp);
    }

    return reserve_storage_area_space(offset, alignment { 0, boundary ? boundary : 1 }, dep_ctx).second;
}

void ordinary_assembly_context::set_available_location_counter_value()
{
    auto [sp, addr] = loctr().set_available_value();

    if (sp)
        m_symbol_dependencies->add_dependency(std::move(sp),
            std::make_unique<aggregate_address_resolver>(std::move(addr), 0, 0),
            dependency_evaluation_context(current_opcode_generation()));
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
    ensure_current_section();

    for (const auto& sect : sections_)
    {
        if (sect->find_location_counter(name))
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
    return reserve_storage_area(length, align, dependency_evaluation_context(current_opcode_generation()));
}

address ordinary_assembly_context::align(alignment align, const dependency_evaluation_context& dep_ctx)
{
    return reserve_storage_area(0, align, dep_ctx);
}

address ordinary_assembly_context::align(alignment a)
{
    return align(a, dependency_evaluation_context(current_opcode_generation()));
}

space_ptr ordinary_assembly_context::register_ordinary_space(alignment align)
{
    return loctr().register_ordinary_space(align);
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
    size_t length, alignment align, const dependency_evaluation_context& dep_ctx)
{
    auto& l = loctr();

    if (l.need_space_alignment(align))
    {
        address addr = l.current_address_for_alignment_evaluation(align);
        auto [ret_addr, sp] = l.reserve_storage_area(length, align);
        assert(sp);

        m_symbol_dependencies->add_dependency(sp, std::make_unique<address_resolver>(addr, align.boundary), dep_ctx);
        return std::make_pair(ret_addr, sp);
    }
    return std::make_pair(l.reserve_storage_area(length, align).first, nullptr);
}

section* ordinary_assembly_context::create_section(id_index name, section_kind kind)
{
    using enum section_kind;
    section* ret = sections_.emplace_back(std::make_unique<section>(name, kind)).get();
    if (first_control_section_ == nullptr && (kind == COMMON || kind == EXECUTABLE || kind == READONLY))
        first_control_section_ = ret;
    return ret;
}

section* ordinary_assembly_context::create_section(id_index name, section_kind kind, goff_details details)
{
    using enum section_kind;
    section* ret = sections_.emplace_back(std::make_unique<section>(name, kind, std::move(details))).get();
    if (first_control_section_ == nullptr && (kind == COMMON || kind == EXECUTABLE || kind == READONLY))
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
