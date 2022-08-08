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
#include <cassert>
#include <queue>
#include <stack>
#include <stdexcept>

#include "ordinary_assembly_context.h"
#include "ordinary_assembly_dependency_solver.h"
#include "processing/instruction_sets/low_language_processor.h"
#include "processing/instruction_sets/postponed_statement_impl.h"

namespace hlasm_plugin::parser_library::context {

bool symbol_dependency_tables::check_cycle(dependant target, std::vector<dependant> dependencies)
{
    if (std::find(dependencies.begin(), dependencies.end(), target)
        != dependencies.end()) // dependencies contain target itself
    {
        resolve_dependant_default(target);
        return false;
    }

    while (!dependencies.empty())
    {
        auto top_dep = std::move(dependencies.back());
        dependencies.pop_back();

        auto dependant = find_dependency_value(top_dep);
        if (!dependant)
            continue;

        for (auto&& dep : extract_dependencies(dependant->m_resolvable, dependant->m_dec))
        {
            if (dep == target)
            {
                resolve_dependant_default(target);
                return false;
            }
            dependencies.push_back(std::move(dep));
        }
    }
    return true;
}

struct resolve_dependant_visitor
{
    symbol_value& val;
    loctr_dependency_resolver* resolver;
    ordinary_assembly_context& sym_ctx;
    std::unordered_map<dependant, statement_ref>& dependency_source_stmts;
    const dependency_evaluation_context& dep_ctx;

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
            resolver->resolve_unknown_loctr_dependency(sp,
                val.get_reloc(),
                dependency_source_stmts.find(sp)->second.stmt_ref->first->resolved_stmt()->stmt_range_ref(),
                dep_ctx);
        else
            space::resolve(sp, length);
    }
};

void symbol_dependency_tables::resolve_dependant(dependant target,
    const resolvable* dep_src,
    loctr_dependency_resolver* resolver,
    const dependency_evaluation_context& dep_ctx)
{
    context::ordinary_assembly_dependency_solver dep_solver(m_sym_ctx, dep_ctx);
    symbol_value val = dep_src->resolve(dep_solver);

    std::visit(resolve_dependant_visitor { val, resolver, m_sym_ctx, m_dependency_source_stmts, dep_ctx }, target);
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

void symbol_dependency_tables::resolve(loctr_dependency_resolver* resolver)
{
    const auto resolvable = [this, resolver](const std::pair<dependant, dependency_value>& v) {
        if (!resolver && std::holds_alternative<space_ptr>(v.first))
            return false;
        return !has_dependencies(v.second.m_resolvable, v.second.m_dec);
    };
    auto it = m_dependencies.end();
    while ((it = std::find_if(m_dependencies.begin(), m_dependencies.end(), resolvable)) != m_dependencies.end())
    {
        const auto& [target, dep_value] = *it;

        resolve_dependant(target, dep_value.m_resolvable, resolver, dep_value.m_dec); // resolve target
        try_erase_source_statement(target);
        m_dependencies.erase(it);
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
    const resolvable* dependency_source, const dependency_evaluation_context& dep_ctx)
{
    std::vector<dependant> ret;
    context::ordinary_assembly_dependency_solver dep_solver(m_sym_ctx, dep_ctx);
    auto deps = dependency_source->get_dependencies(dep_solver);

    ret.insert(ret.end(),
        std::make_move_iterator(deps.undefined_symbols.begin()),
        std::make_move_iterator(deps.undefined_symbols.end()));

    if (!ret.empty())
        return ret;

    ret.insert(ret.end(),
        std::make_move_iterator(deps.unresolved_spaces.begin()),
        std::make_move_iterator(deps.unresolved_spaces.end()));

    if (!ret.empty())
        return ret;

    ret.insert(ret.end(),
        std::make_move_iterator(deps.undefined_attr_refs.begin()),
        std::make_move_iterator(deps.undefined_attr_refs.end()));

    if (deps.unresolved_address)
        for (auto& [space_id, count] : deps.unresolved_address->normalized_spaces())
        {
            assert(count != 0);
            ret.push_back(space_id);
        }


    return ret;
}

bool symbol_dependency_tables::has_dependencies(
    const resolvable* dependency_source, const dependency_evaluation_context& dep_ctx)
{
    context::ordinary_assembly_dependency_solver dep_solver(m_sym_ctx, dep_ctx);
    auto deps = dependency_source->get_dependencies(dep_solver);

    return !deps.undefined_symbols.empty() || !deps.unresolved_spaces.empty() || !deps.undefined_attr_refs.empty()
        || deps.unresolved_address && !deps.unresolved_address->normalized_spaces().empty();
}

std::vector<dependant> symbol_dependency_tables::extract_dependencies(
    const std::vector<const resolvable*>& dependency_sources, const dependency_evaluation_context& dep_ctx)
{
    std::vector<dependant> ret;

    for (auto dep : dependency_sources)
    {
        auto tmp = extract_dependencies(dep, dep_ctx);
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
    const dependency_evaluation_context& dep_ctx)
{
    if (check_for_cycle)
    {
        auto dependencies = extract_dependencies(dependency_source, dep_ctx);

        bool no_cycle = check_cycle(target, dependencies);
        if (!no_cycle)
        {
            resolve(nullptr);
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
    const dependency_evaluation_context& dep_ctx)
{
    dependency_adder adder(*this, std::move(dependency_source_stmt), dep_ctx);
    bool added = adder.add_dependency(target, dependency_source);
    adder.finish();

    return added;
}

bool symbol_dependency_tables::add_dependency(id_index target,
    data_attr_kind attr,
    const resolvable* dependency_source,
    post_stmt_ptr dependency_source_stmt,
    const dependency_evaluation_context& dep_ctx)
{
    dependency_adder adder(*this, std::move(dependency_source_stmt), dep_ctx);
    bool added = adder.add_dependency(target, attr, dependency_source);
    adder.finish();

    return added;
}

void symbol_dependency_tables::add_dependency(space_ptr space,
    const resolvable* dependency_source,
    post_stmt_ptr dependency_source_stmt,
    const dependency_evaluation_context& dep_ctx)
{
    dependency_adder adder(*this, std::move(dependency_source_stmt), dep_ctx);
    adder.add_dependency(space, dependency_source);
    adder.finish();
}

void symbol_dependency_tables::add_dependency(space_ptr target,
    addr_res_ptr dependency_source,
    const dependency_evaluation_context& dep_ctx,
    post_stmt_ptr dependency_source_stmt)
{
    auto [it, inserted] = m_dependency_source_addrs.emplace(target, std::move(dependency_source));

    assert(inserted);

    add_dependency(dependant(target), &*it->second, false, dep_ctx);

    if (dependency_source_stmt)
    {
        auto [sit, sinserted] = m_postponed_stmts.try_emplace(std::move(dependency_source_stmt), dep_ctx);

        assert(sinserted);

        m_dependency_source_stmts.emplace(dependant(std::move(target)), statement_ref(sit, 1));
    }
}

bool symbol_dependency_tables::check_cycle(space_ptr target)
{
    auto dep_src = find_dependency_value(target);
    if (!dep_src)
        return true;

    bool no_cycle = check_cycle(target, extract_dependencies(dep_src->m_resolvable, dep_src->m_dec));

    if (!no_cycle)
        resolve(nullptr);

    return no_cycle;
}

void symbol_dependency_tables::add_dependency(post_stmt_ptr target, const dependency_evaluation_context& dep_ctx)
{
    dependency_adder adder(*this, std::move(target), dep_ctx);
    adder.add_dependency();
    adder.finish();
}

dependency_adder symbol_dependency_tables::add_dependencies(
    post_stmt_ptr dependency_source_stmt, const dependency_evaluation_context& dep_ctx)
{
    return dependency_adder(*this, std::move(dependency_source_stmt), dep_ctx);
}

void symbol_dependency_tables::add_defined(loctr_dependency_resolver* resolver) { resolve(resolver); }

bool symbol_dependency_tables::check_loctr_cycle()
{
    struct deref_tools
    {
        bool operator()(const dependant* l, const dependant* r) const { return *l == *r; }
        size_t operator()(const dependant* p) const { return std::hash<dependant>()(*p); }
    };

    const auto dep_g = [this]() {
        // create graph
        std::unordered_map<const dependant*, std::vector<dependant>, deref_tools, deref_tools> result;
        for (const auto& [target, dep_src_loctr] : m_dependencies)
        {
            if (!std::holds_alternative<space_ptr>(target))
                continue;

            auto new_deps = extract_dependencies(dep_src_loctr.m_resolvable, dep_src_loctr.m_dec);

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
                    next_steps.emplace_front(&*d); // to_address broken in libc++-12
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

dependency_adder::dependency_adder(
    symbol_dependency_tables& owner, post_stmt_ptr dependency_source_stmt, const dependency_evaluation_context& dep_ctx)
    : m_owner(owner)
    , m_ref_count(0)
    , m_dep_ctx(dep_ctx)
    , m_source_stmt(std::move(dependency_source_stmt))
{}

bool dependency_adder::add_dependency(id_index target, const resolvable* dependency_source)
{
    bool added = m_owner.add_dependency(dependant(target), dependency_source, true, m_dep_ctx);

    if (added)
    {
        ++m_ref_count;
        m_dependants.push_back(target);
    }

    return added;
}

bool dependency_adder::add_dependency(id_index target, data_attr_kind attr, const resolvable* dependency_source)
{
    bool added = m_owner.add_dependency(dependant(attr_ref { attr, target }), dependency_source, true, m_dep_ctx);

    if (added)
    {
        ++m_ref_count;
        m_dependants.push_back(attr_ref { attr, target });
    }

    return added;
}

void dependency_adder::add_dependency(space_ptr target, const resolvable* dependency_source)
{
    m_owner.add_dependency(dependant(target), dependency_source, false, m_dep_ctx);
    ++m_ref_count;
    m_dependants.push_back(target);
}

void dependency_adder::add_dependency() { ++m_ref_count; }

void dependency_adder::finish()
{
    if (m_ref_count == 0)
        return;

    auto [it, inserted] = m_owner.m_postponed_stmts.try_emplace(std::move(m_source_stmt), m_dep_ctx);

    if (!inserted)
        throw std::runtime_error("statement already registered");

    statement_ref ref(it, m_ref_count);

    for (auto& dep : m_dependants)
        m_owner.m_dependency_source_stmts.emplace(std::move(dep), ref);

    m_dependants.clear();
}

} // namespace hlasm_plugin::parser_library::context
