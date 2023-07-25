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

#include "symbol_dependency_tables.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <memory>
#if __cpp_lib_polymorphic_allocator >= 201902L && __cpp_lib_memory_resource >= 201603L
#    include <memory_resource>
#endif
#include <queue>
#include <stack>
#include <unordered_set>

#include "ordinary_assembly_context.h"
#include "ordinary_assembly_dependency_solver.h"
#include "processing/instruction_sets/low_language_processor.h"
#include "processing/instruction_sets/postponed_statement_impl.h"

namespace hlasm_plugin::parser_library::context {
bool symbol_dependency_tables::check_cycle(
    dependant target, std::vector<dependant> dependencies, const library_info& li)
{
    if (dependencies.empty())
        return true;

    if (std::find(dependencies.begin(), dependencies.end(), target)
        != dependencies.end()) // dependencies contain target itself
    {
        resolve_dependant_default(target);
        return false;
    }

    const auto dep_to_depref = [](const dependant& d) -> dependant_ref {
        if (std::holds_alternative<id_index>(d))
            return std::get<id_index>(d);
        if (std::holds_alternative<attr_ref>(d))
            return std::get<attr_ref>(d);
        if (std::holds_alternative<space_ptr>(d))
            return std::get<space_ptr>(d).get();
        assert(false);
    };

#if __cpp_lib_polymorphic_allocator >= 201902L && __cpp_lib_memory_resource >= 201603L
    alignas(std::max_align_t) std::array<unsigned char, 8 * 1024> buffer;
    std::pmr::monotonic_buffer_resource buffer_resource(buffer.data(), buffer.size());
    std::pmr::unordered_set<dependant_ref> seen_before(&buffer_resource);
#else
    std::unordered_set<dependant_ref> seen_before;
#endif
    for (const auto& d : dependencies)
        seen_before.emplace(dep_to_depref(d));

    while (!dependencies.empty())
    {
        auto top_dep = std::move(dependencies.back());
        dependencies.pop_back();

        auto dependant = find_dependency_value(top_dep);
        if (!dependant)
            continue;

        for (auto&& dep : extract_dependencies(dependant->m_resolvable, dependant->m_dec, li))
        {
            if (dep == target)
            {
                resolve_dependant_default(target);
                return false;
            }
            if (!seen_before.emplace(dep_to_depref(dep)).second)
                continue;
            dependencies.push_back(std::move(dep));
        }
    }
    return true;
}

struct resolve_dependant_visitor
{
    symbol_value& val;
    diagnostic_s_consumer* diag_consumer;
    ordinary_assembly_context& sym_ctx;
    std::unordered_map<dependant, statement_ref>& dependency_source_stmts;
    const dependency_evaluation_context& dep_ctx;
    const library_info& li;

    void operator()(const attr_ref& ref) const
    {
        assert(ref.attribute == data_attr_kind::L || ref.attribute == data_attr_kind::S);

        auto tmp_sym = sym_ctx.get_symbol(ref.symbol_id);
        assert(!tmp_sym->attributes().is_defined(ref.attribute));

        symbol_attributes::value_t value = (val.value_kind() == symbol_value_kind::ABS)
            ? val.get_abs()
            : symbol_attributes::default_value(ref.attribute);

        if (ref.attribute == data_attr_kind::L)
            tmp_sym->set_length(value);
        if (ref.attribute == data_attr_kind::S)
            tmp_sym->set_scale(value);
    }
    void operator()(id_index symbol) const { sym_ctx.get_symbol(symbol)->set_value(val); }
    void operator()(const space_ptr& sp) const
    {
        int length = 0;
        if (sp->kind == space_kind::ORDINARY || sp->kind == space_kind::LOCTR_MAX || sp->kind == space_kind::LOCTR_SET)
            length = (val.value_kind() == symbol_value_kind::ABS && val.get_abs() >= 0) ? val.get_abs() : 0;
        else if (sp->kind == space_kind::ALIGNMENT)
            length = val.value_kind() == symbol_value_kind::RELOC ? val.get_reloc().offset() : 0;

        if (sp->kind == space_kind::LOCTR_UNKNOWN)
        {
            // I don't think that the postponed statement has to always exist.
            auto dep_it = dependency_source_stmts.find(sp);
            const postponed_statement* stmt =
                dep_it == dependency_source_stmts.end() ? nullptr : dep_it->second.stmt_ref->first.get();
            resolve_unknown_loctr_dependency(sp, val, stmt);
        }
        else
            space::resolve(sp, length);
    }

    void resolve_unknown_loctr_dependency(
        context::space_ptr sp, const context::symbol_value& sym_val, const postponed_statement* stmt) const
    {
        using namespace processing;

        assert(diag_consumer);

        const auto add_diagnostic = [stmt, d = diag_consumer](auto f) {
            if (stmt)
                d->add_diagnostic(
                    add_stack_details(f(stmt->resolved_stmt()->stmt_range_ref()), stmt->location_stack()));
            else
                d->add_diagnostic(add_stack_details(f(range()), {}));
        };

        if (sym_val.value_kind() != context::symbol_value_kind::RELOC)
        {
            add_diagnostic(diagnostic_op::error_A245_ORG_expression);
            return;
        }

        const auto& addr = sym_val.get_reloc();

        if (auto spaces = addr.normalized_spaces();
            std::find_if(spaces.begin(), spaces.end(), [&sp](const auto& e) { return e.first == sp; }) != spaces.end())
            add_diagnostic(diagnostic_op::error_E033);

        auto tmp_loctr = sym_ctx.current_section()->current_location_counter();

        sym_ctx.set_location_counter(sp->owner.name, location(), li);
        sym_ctx.current_section()->current_location_counter().switch_to_unresolved_value(sp);

        if (auto org = check_address_for_ORG(
                addr, sym_ctx.align(context::no_align, dep_ctx, li), sp->previous_boundary, sp->previous_offset);
            org != check_org_result::valid)
        {
            if (org == check_org_result::underflow)
                add_diagnostic(diagnostic_op::error_E068);
            else if (org == check_org_result::invalid_address)
                add_diagnostic(diagnostic_op::error_A115_ORG_op_format);

            (void)sym_ctx.current_section()->current_location_counter().restore_from_unresolved_value(sp);
            sym_ctx.set_location_counter(tmp_loctr.name, location(), li);
            return;
        }

        auto new_sp = sym_ctx.set_location_counter_value_space(
            addr, sp->previous_boundary, sp->previous_offset, nullptr, nullptr, dep_ctx, li);

        auto ret = sym_ctx.current_section()->current_location_counter().restore_from_unresolved_value(sp);
        sym_ctx.set_location_counter(tmp_loctr.name, location(), li);

        context::space::resolve(sp, std::move(ret));

        if (!sym_ctx.symbol_dependencies.check_cycle(new_sp, li))
            add_diagnostic(diagnostic_op::error_E033);

        for (auto& sect : sym_ctx.sections())
        {
            for (auto& loctr : sect->location_counters())
            {
                if (!loctr->check_underflow())
                {
                    add_diagnostic(diagnostic_op::error_E068);
                    return;
                }
            }
        }
    }
};

struct dependant_visitor
{
    std::variant<id_index, space_ptr> operator()(id_index id) const { return id; }
    std::variant<id_index, space_ptr> operator()(attr_ref a) const { return a.symbol_id; }
    std::variant<id_index, space_ptr> operator()(space_ptr p) const { return std::move(p); }
};

void symbol_dependency_tables::resolve_dependant(dependant target,
    const resolvable* dep_src,
    diagnostic_s_consumer* diag_consumer,
    const dependency_evaluation_context& dep_ctx,
    const library_info& li)
{
    context::ordinary_assembly_dependency_solver dep_solver(m_sym_ctx, dep_ctx, li);
    symbol_value val = dep_src->resolve(dep_solver);

    std::visit(
        resolve_dependant_visitor { val, diag_consumer, m_sym_ctx, m_dependency_source_stmts, dep_ctx, li }, target);
}

struct resolve_dependant_default_visitor
{
    ordinary_assembly_context& m_sym_ctx;

    void operator()(const attr_ref& ref) const
    {
        assert(ref.attribute == data_attr_kind::L || ref.attribute == data_attr_kind::S);
        auto tmp_sym = m_sym_ctx.get_symbol(ref.symbol_id);
        assert(!tmp_sym->attributes().is_defined(ref.attribute));
        if (ref.attribute == data_attr_kind::L)
            tmp_sym->set_length(1);
        if (ref.attribute == data_attr_kind::S)
            tmp_sym->set_scale(0);
    }
    void operator()(id_index symbol) const
    {
        auto tmp_sym = m_sym_ctx.get_symbol(symbol);
        tmp_sym->set_value(0);
    }
    void operator()(const space_ptr& sp) const { space::resolve(sp, 1); }
};

void symbol_dependency_tables::resolve_dependant_default(const dependant& target)
{
    std::visit(resolve_dependant_default_visitor { m_sym_ctx }, target);
}

void symbol_dependency_tables::resolve(
    std::variant<id_index, space_ptr> what_changed, diagnostic_s_consumer* diag_consumer, const library_info& li)
{
    const auto cleanup_deps = [&what_changed](auto& entry) {
        std::erase_if(entry.m_last_dependencies, [&what_changed](const auto& v) {
            return what_changed == v || std::holds_alternative<space_ptr>(v) && std::get<space_ptr>(v)->resolved();
        });
    };
    const auto resolvable = [this, diag_consumer, &cleanup_deps, &li](std::pair<const dependant, dependency_value>& v) {
        cleanup_deps(v.second);
        if (!diag_consumer && std::holds_alternative<space_ptr>(v.first))
            return false;
        if (!v.second.m_last_dependencies.empty() || (v.second.m_has_t_attr_dependency && !diag_consumer))
            return false;
        return !update_dependencies(v.second, li);
    };
    auto it = m_dependencies.end();
    while ((it = std::find_if(m_dependencies.begin(), m_dependencies.end(), resolvable)) != m_dependencies.end())
    {
        const auto& [target, dep_value] = *it;

        resolve_dependant(target, dep_value.m_resolvable, diag_consumer, dep_value.m_dec, li); // resolve target
        try_erase_source_statement(target);

        for (auto nested = std::next(it); nested != m_dependencies.end(); ++nested)
            cleanup_deps(nested->second);

        what_changed = std::visit(dependant_visitor(), std::move(m_dependencies.extract(it).key()));
    }
}

const symbol_dependency_tables::dependency_value* symbol_dependency_tables::find_dependency_value(
    const dependant& target) const
{
    if (auto it = m_dependencies.find(target); it != m_dependencies.end())
        return &it->second;

    return nullptr;
}

std::vector<dependant> symbol_dependency_tables::extract_dependencies(
    const resolvable* dependency_source, const dependency_evaluation_context& dep_ctx, const library_info& li)
{
    std::vector<dependant> ret;
    context::ordinary_assembly_dependency_solver dep_solver(m_sym_ctx, dep_ctx, li);
    auto deps = dependency_source->get_dependencies(dep_solver);

    for (const auto& ref : deps.undefined_symbolics)
        if (ref.get())
            ret.emplace_back(ref.name);

    if (!ret.empty())
        return ret;

    ret.insert(ret.end(),
        std::make_move_iterator(deps.unresolved_spaces.begin()),
        std::make_move_iterator(deps.unresolved_spaces.end()));

    if (!ret.empty())
        return ret;

    for (const auto& ref : deps.undefined_symbolics)
    {
        for (int i = 1; i < static_cast<int>(data_attr_kind::max); ++i)
            if (ref.get(static_cast<data_attr_kind>(i)))
                ret.emplace_back(attr_ref { static_cast<data_attr_kind>(i), ref.name });
    }

    if (deps.unresolved_address)
        for (auto& [space_id, count] : deps.unresolved_address->normalized_spaces())
        {
            assert(count != 0);
            ret.push_back(space_id);
        }


    return ret;
}

bool symbol_dependency_tables::update_dependencies(dependency_value& d, const library_info& li)
{
    context::ordinary_assembly_dependency_solver dep_solver(m_sym_ctx, d.m_dec, li);
    auto deps = d.m_resolvable->get_dependencies(dep_solver);

    d.m_last_dependencies.clear();
    d.m_has_t_attr_dependency = false;

    for (const auto& ref : deps.undefined_symbolics)
    {
        if (ref.get(context::data_attr_kind::T))
            d.m_has_t_attr_dependency = true;

        if (ref.has_only(context::data_attr_kind::T))
            continue;

        d.m_last_dependencies.emplace_back(ref.name);
    }

    if (!d.m_last_dependencies.empty() || d.m_has_t_attr_dependency)
        return true;

    for (const auto& sp : deps.unresolved_spaces)
        d.m_last_dependencies.emplace_back(sp);

    if (deps.unresolved_address)
        for (auto&& [sp, _] : deps.unresolved_address->normalized_spaces())
            d.m_last_dependencies.emplace_back(std::move(sp));

    return !d.m_last_dependencies.empty();
}

std::vector<dependant> symbol_dependency_tables::extract_dependencies(
    const std::vector<const resolvable*>& dependency_sources,
    const dependency_evaluation_context& dep_ctx,
    const library_info& li)
{
    std::vector<dependant> ret;

    for (auto dep : dependency_sources)
    {
        auto tmp = extract_dependencies(dep, dep_ctx, li);
        ret.insert(ret.end(), std::make_move_iterator(tmp.begin()), std::make_move_iterator(tmp.end()));
    }
    return ret;
}

void symbol_dependency_tables::try_erase_source_statement(const dependant& index)
{
    auto ait = m_dependency_source_addrs.find(index);
    if (ait != m_dependency_source_addrs.end())
        m_dependency_source_addrs.erase(ait);

    auto it = m_dependency_source_stmts.find(index);
    if (it == m_dependency_source_stmts.end())
        return;

    auto& [dep, ref] = *it;

    assert(ref.ref_count >= 1);

    if (--ref.ref_count == 0)
        m_postponed_stmts.erase(ref.stmt_ref);

    m_dependency_source_stmts.erase(it);
}

symbol_dependency_tables::symbol_dependency_tables(ordinary_assembly_context& sym_ctx)
    : m_sym_ctx(sym_ctx)
{}

bool symbol_dependency_tables::add_dependency(dependant target,
    const resolvable* dependency_source,
    bool check_for_cycle,
    const dependency_evaluation_context& dep_ctx,
    const library_info& li)
{
    if (check_for_cycle)
    {
        bool no_cycle = check_cycle(target, extract_dependencies(dependency_source, dep_ctx, li), li);
        if (!no_cycle)
        {
            resolve(std::visit(dependant_visitor(), target), nullptr, li);
            return false;
        }
    }

    auto [it, inserted] = m_dependencies.try_emplace(std::move(target), dependency_source, dep_ctx);

    assert(inserted);

    return true;
}

bool symbol_dependency_tables::add_dependency(id_index target,
    const resolvable* dependency_source,
    post_stmt_ptr dependency_source_stmt,
    const dependency_evaluation_context& dep_ctx,
    const library_info& li)
{
    dependency_adder adder(*this, std::move(dependency_source_stmt), dep_ctx, li);
    bool added = adder.add_dependency(target, dependency_source);
    adder.finish();

    return added;
}

bool symbol_dependency_tables::add_dependency(id_index target,
    data_attr_kind attr,
    const resolvable* dependency_source,
    post_stmt_ptr dependency_source_stmt,
    const dependency_evaluation_context& dep_ctx,
    const library_info& li)
{
    dependency_adder adder(*this, std::move(dependency_source_stmt), dep_ctx, li);
    bool added = adder.add_dependency(target, attr, dependency_source);
    adder.finish();

    return added;
}

void symbol_dependency_tables::add_dependency(space_ptr space,
    const resolvable* dependency_source,
    post_stmt_ptr dependency_source_stmt,
    const dependency_evaluation_context& dep_ctx,
    const library_info& li)
{
    dependency_adder adder(*this, std::move(dependency_source_stmt), dep_ctx, li);
    adder.add_dependency(space, dependency_source);
    adder.finish();
}

void symbol_dependency_tables::add_dependency(space_ptr target,
    addr_res_ptr dependency_source,
    const dependency_evaluation_context& dep_ctx,
    const library_info& li,
    post_stmt_ptr dependency_source_stmt)
{
    auto [it, inserted] = m_dependency_source_addrs.emplace(target, std::move(dependency_source));

    assert(inserted);

    add_dependency(dependant(target), std::to_address(it->second), false, dep_ctx, li);

    if (dependency_source_stmt)
    {
        auto [sit, sinserted] = m_postponed_stmts.try_emplace(std::move(dependency_source_stmt), dep_ctx);

        assert(sinserted);

        m_dependency_source_stmts.emplace(dependant(std::move(target)), statement_ref(sit, 1));
    }
}

bool symbol_dependency_tables::check_cycle(space_ptr target, const library_info& li)
{
    auto dep_src = find_dependency_value(target);
    if (!dep_src)
        return true;

    bool no_cycle = check_cycle(target, extract_dependencies(dep_src->m_resolvable, dep_src->m_dec, li), li);

    if (!no_cycle)
        resolve(std::move(target), nullptr, li);

    return no_cycle;
}

void symbol_dependency_tables::add_dependency(
    post_stmt_ptr target, const dependency_evaluation_context& dep_ctx, const library_info& li)
{
    dependency_adder adder(*this, std::move(target), dep_ctx, li);
    adder.add_dependency();
    adder.finish();
}

dependency_adder symbol_dependency_tables::add_dependencies(
    post_stmt_ptr dependency_source_stmt, const dependency_evaluation_context& dep_ctx, const library_info& li)
{
    return dependency_adder(*this, std::move(dependency_source_stmt), dep_ctx, li);
}

void symbol_dependency_tables::add_defined(
    const std::variant<id_index, space_ptr>& what_changed, diagnostic_s_consumer* diag_consumer, const library_info& li)
{
    resolve(what_changed, diag_consumer, li);
}

bool symbol_dependency_tables::check_loctr_cycle(const library_info& li)
{
    struct deref_tools
    {
        bool operator()(const dependant* l, const dependant* r) const { return *l == *r; }
        size_t operator()(const dependant* p) const { return std::hash<dependant>()(*p); }
    };

    const auto dep_g = [this, &li]() {
        // create graph
        std::unordered_map<const dependant*, std::vector<dependant>, deref_tools, deref_tools> result;
        for (const auto& [target, dep_src_loctr] : m_dependencies)
        {
            if (!std::holds_alternative<space_ptr>(target))
                continue;

            auto new_deps = extract_dependencies(dep_src_loctr.m_resolvable, dep_src_loctr.m_dec, li);

            std::erase_if(new_deps, [](const auto& e) { return !std::holds_alternative<space_ptr>(e); });

            if (!new_deps.empty())
                result.emplace(&target, std::move(new_deps));
        }
        return result;
    }();

    std::unordered_set<const dependant*, deref_tools, deref_tools> cycles;

    std::unordered_set<const dependant*, deref_tools, deref_tools> visited;
    std::vector<const dependant*> path;
    std::deque<const dependant*> next_steps;

    for (const auto& [target, _] : dep_g)
    {
        if (visited.contains(target))
            continue;

        next_steps.push_back(target);

        while (!next_steps.empty())
        {
            auto next = std::move(next_steps.front());
            next_steps.pop_front();
            if (!next)
            {
                path.pop_back();
                continue;
            }
            if (auto it = std::find_if(path.begin(), path.end(), [next](const auto* v) { return *v == *next; });
                it != path.end())
            {
                cycles.insert(it, path.end());
                continue;
            }
            if (!visited.emplace(next).second)
                continue;
            if (auto it = dep_g.find(next); it != dep_g.end())
            {
                path.push_back(next);
                next_steps.emplace_front(nullptr);
                for (auto d = it->second.rbegin(); d != it->second.rend(); ++d)
                    next_steps.emplace_front(std::to_address(d));
            }
        }
        assert(path.empty());
    }

    for (const auto* target : cycles)
    {
        resolve_dependant_default(*target);
        try_erase_source_statement(*target);
        m_dependencies.erase(*target);
    }

    return cycles.empty();
}

std::vector<std::pair<post_stmt_ptr, dependency_evaluation_context>> symbol_dependency_tables::collect_postponed()
{
    std::vector<std::pair<post_stmt_ptr, dependency_evaluation_context>> res;

    res.reserve(m_postponed_stmts.size());
    for (auto it = m_postponed_stmts.begin(); it != m_postponed_stmts.end();)
    {
        auto node = m_postponed_stmts.extract(it++);
        res.emplace_back(std::move(node.key()), std::move(node.mapped()));
    }

    m_postponed_stmts.clear();
    m_dependency_source_stmts.clear();
    m_dependencies.clear();

    return res;
}

void symbol_dependency_tables::resolve_all_as_default()
{
    for (auto& [target, dep_src] : m_dependencies)
        resolve_dependant_default(target);
}

statement_ref::statement_ref(ref_t stmt_ref, size_t ref_count)
    : stmt_ref(std::move(stmt_ref))
    , ref_count(ref_count)
{}

dependency_adder::dependency_adder(symbol_dependency_tables& owner,
    post_stmt_ptr dependency_source_stmt,
    const dependency_evaluation_context& dep_ctx,
    const library_info& li)
    : m_owner(owner)
    , m_ref_count(0)
    , m_dep_ctx(dep_ctx)
    , m_source_stmt(std::move(dependency_source_stmt))
    , m_li(li)
{}

bool dependency_adder::add_dependency(id_index target, const resolvable* dependency_source)
{
    bool added = m_owner.add_dependency(dependant(target), dependency_source, true, m_dep_ctx, m_li);

    if (added)
    {
        ++m_ref_count;
        m_dependants.push_back(target);
    }

    return added;
}

bool dependency_adder::add_dependency(id_index target, data_attr_kind attr, const resolvable* dependency_source)
{
    bool added = m_owner.add_dependency(dependant(attr_ref { attr, target }), dependency_source, true, m_dep_ctx, m_li);

    if (added)
    {
        ++m_ref_count;
        m_dependants.push_back(attr_ref { attr, target });
    }

    return added;
}

void dependency_adder::add_dependency(space_ptr target, const resolvable* dependency_source)
{
    m_owner.add_dependency(dependant(target), dependency_source, false, m_dep_ctx, m_li);
    ++m_ref_count;
    m_dependants.push_back(target);
}

void dependency_adder::add_dependency() { ++m_ref_count; }

void dependency_adder::finish()
{
    if (m_ref_count == 0)
        return;

    auto [it, inserted] = m_owner.m_postponed_stmts.try_emplace(std::move(m_source_stmt), m_dep_ctx);

    assert(inserted);

    statement_ref ref(it, m_ref_count);

    for (auto& dep : m_dependants)
        m_owner.m_dependency_source_stmts.emplace(std::move(dep), ref);

    m_dependants.clear();
}

} // namespace hlasm_plugin::parser_library::context
