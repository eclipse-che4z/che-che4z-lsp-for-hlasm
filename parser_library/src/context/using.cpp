/*
 * Copyright (c) 2021 Broadcom.
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

#include "using.h"

#include <limits>

#include "diagnostic_consumer.h"
#include "expressions/mach_expr_term.h"
#include "expressions/mach_expression.h"
#include "ordinary_assembly/dependable.h"
#include "utils/similar.h"

namespace hlasm_plugin::parser_library::context {

void using_collection::using_entry::resolve(using_collection& coll, diagnostic_consumer<diagnostic_op>& diag)
{
    resolved = definition.resolve(coll, diag);
}

void using_collection::using_entry::compute_context(using_collection& coll, diagnostic_consumer<diagnostic_op>& diag)
{
    std::visit([&coll, &diag, this](const auto& e) { compute_context(coll, e, diag); }, resolved);
}


void using_collection::using_entry::duplicate_parent_context(using_collection& coll, index_t p)
{
    if (!p)
        return;
    context = coll.m_usings[p.index].context;
}

void using_collection::using_entry::compute_context(
    using_collection& coll, index_t parent, diagnostic_consumer<diagnostic_op>&)
{
    duplicate_parent_context(coll, parent); // just duplicate previous state on error
}

void using_collection::using_entry::compute_context(
    using_collection& coll, const using_entry_resolved& u, diagnostic_consumer<diagnostic_op>& diag)
{
    duplicate_parent_context(coll, u.parent);

    for (auto r : u.reg_set)
        if (r != invalid_register)
            compute_context_drop(r, u.label); // drop conflicting usings

    context.m_state.emplace_back(using_context::entry { u.label, u.owner, u.begin, u.length, u.reg_set, u.reg_offset });
}

void using_collection::using_entry::compute_context(
    using_collection& coll, const drop_entry_resolved& d, diagnostic_consumer<diagnostic_op>& diag)
{
    size_t invalidated = 0;
    duplicate_parent_context(coll, d.parent);
    for (const auto& drop : d.drop)
        invalidated += std::visit([this](auto value) { return compute_context_drop(value); }, drop);

    if (invalidated == 0)
    {
        // TODO: warning no impact???
    }
}

size_t using_collection::using_entry::compute_context_drop(id_index d)
{
    return std::erase_if(context.m_state, [d](const auto& e) { return e.label == d; });
}

size_t using_collection::using_entry::compute_context_drop(register_t d, id_index label)
{
    size_t invalidated = 0;
    for (auto& e : context.m_state)
        if (e.label == label)
            invalidated += std::exchange(e.regs[d], invalid_register) != invalid_register;
    std::erase_if(context.m_state, [](const auto& e) { return e.regs == invalid_register_set; });

    return invalidated;
}

auto using_collection::using_drop_definition::abs_or_reloc(
    using_collection& coll, const mach_expression* e, bool abs_is_register)
    -> std::optional<std::pair<const section*, offset_t>>
{
    if (!e)
        return std::nullopt;

    const auto& res = coll.eval_expr(e);

    if (res.value_kind() == symbol_value_kind::ABS)
    {
        auto v = res.get_abs();
        if (abs_is_register && (v < 0 || v >= reg_set_size))
        {
            // TODO: diagnose
            return std::nullopt;
        }
        return std::make_pair(nullptr, v);
    }
    if (res.value_kind() == symbol_value_kind::RELOC && res.get_reloc().is_simple())
        return std::make_pair(res.get_reloc().bases().front().first.owner, res.get_reloc().offset());

    // TODO: diagnose
    return std::nullopt;
}
std::variant<std::monostate, id_index, using_collection::register_t>
using_collection::using_drop_definition::abs_or_label(using_collection& coll, const mach_expression* e)
{
    if (!e)
        return std::monostate();

    if (auto sym = dynamic_cast<const expressions::mach_expr_symbol*>(e); sym)
        return sym->value;

    const auto& res = coll.eval_expr(e);

    if (res.value_kind() == symbol_value_kind::ABS)
    {
        if (auto v = res.get_abs(); v >= 0 && v < reg_set_size)
            return (unsigned char)v;
        // TODO: diagnose
    }
    // TODO: diagnose

    return std::monostate();
}

using_collection::resolved_entry using_collection::using_drop_definition::resolve_using_dep(using_collection& coll,
    const std::pair<const section*, offset_t>& b,
    std::optional<offset_t> len,
    const std::pair<const section*, offset_t>& base,
    diagnostic_consumer<diagnostic_op>& diag) const
{
    if (m_parent == index_t())
    {
        // TODO: diagnose dependent using without any active using
        return {};
    }
    const auto& ctx = coll.m_usings[m_parent.index].context;

    auto v = ctx.evaluate(coll, m_label, b.first, b.second, false);

    if (v.mapping_regs == invalid_register_set)
    {
        // TODO: diagnose no matching using
        return {};
    }

    return using_entry_resolved(m_parent, m_label, b.first, b.second, v.length, v.mapping_regs, v.reg_offset);
}

using_collection::resolved_entry using_collection::using_drop_definition::resolve_using(
    using_collection& coll, diagnostic_consumer<diagnostic_op>& diag) const
{
    assert(!m_base.empty() && m_base.size() <= reg_set_size);

    auto b = abs_or_reloc(coll, m_begin);
    if (!b.has_value())
    {
        // TODO: diagnose
        return {};
    }
    auto e = abs_or_reloc(coll, m_end);

    std::array<std::optional<std::pair<const section*, offset_t>>, reg_set_size> bases_;
    const auto bases = std::span(bases_).first(
        std::count_if(m_base.begin(), m_base.end(), [](const auto* e) { return e != nullptr; }));
    std::transform(
        m_base.begin(), m_base.end(), bases.begin(), [&coll](auto e) { return abs_or_reloc(coll, e, true); });

    std::optional<offset_t> len;
    if (e.has_value())
    {
        if (b->first != e->first)
        {
            // TODO: diagnose
        }
        else if (b->second <= e->second)
        {
            // TODO: diagnose
        }
        else
            len = e->second - b->second;
    }

    if (bases.size() == 1 && bases.front().has_value() && bases.front()->first != nullptr)
        return resolve_using_dep(coll, b.value(), len, bases.front().value(), diag);

    // labeled/ordinary USING continues
    for (auto& base : bases)
    {
        if (base.has_value() && base->first != nullptr)
        {
            // TODO: diagnose absolute value expected
            base.reset(); // drop the value
        }
    }

    register_set_t reg_set_;
    auto reg_set = std::span(reg_set_).first(bases.size());
    std::transform(bases.begin(), bases.end(), reg_set.begin(), [](const auto& r) {
        return r ? (register_t)r->second : invalid_register;
    });

    return using_entry_resolved(
        m_parent, m_label, b->first, b->second, len.value_or(0x1000 * bases.size()), reg_set, 0);
}

using_collection::resolved_entry using_collection::using_drop_definition::resolve_drop(
    using_collection& coll, diagnostic_consumer<diagnostic_op>& diag) const
{
    struct
    {
        diagnostic_consumer<diagnostic_op>& diag;
        std::vector<std::variant<id_index, register_t>> args;

        void operator()(std::monostate)
        {
            // TODO: diagnose
        }
        void operator()(register_t r) { args.emplace_back(r); }
        void operator()(id_index id) { args.emplace_back(id); }

    } transform { diag };

    for (const auto* expr : m_base)
        std::visit(transform, abs_or_label(coll, expr));

    return drop_entry_resolved(m_parent, std::move(transform.args));
}

using_collection::resolved_entry using_collection::using_drop_definition::resolve(
    using_collection& coll, diagnostic_consumer<diagnostic_op>& diag) const
{
    if (is_using())
        return resolve_using(coll, diag);
    else if (is_drop())
        return resolve_drop(coll, diag);
    else
        assert(false);

    return index_t {};
}

const symbol_value& using_collection::eval_expr(const mach_expression* e) const
{
    auto it = m_expressions.find(e);
    assert(it != m_expressions.end());
    return it->second;
}

using_collection::using_collection(using_collection&&) noexcept = default;
using_collection& using_collection::operator=(using_collection&&) noexcept = default;
using_collection::~using_collection() = default;

void using_collection::resolve_all(dependency_solver& solver, diagnostic_consumer<diagnostic_op>& diag)
{
    for (auto& expr : m_expressions)
    {
        expr.second = expr.first->resolve(solver);
        expr.first->collect_diags();
        for (auto& d : expr.first->diags())
            diag.add_diagnostic(std::move(d));
    }

    for (auto& u : m_usings)
    {
        u.resolve(*this, diag);
        u.compute_context(*this, diag);
    }
}

using_collection::index_t using_collection::add(index_t current,
    id_index label,
    std::unique_ptr<mach_expression> begin,
    std::unique_ptr<mach_expression> end,
    std::span<std::unique_ptr<mach_expression>> args,
    diagnostic_consumer<diagnostic_op>& diag)
{
    assert(args.size() <= reg_set_size);

    auto b = m_expressions.try_emplace(std::move(begin)).first->first.get();
    auto e = end ? m_expressions.try_emplace(std::move(end)).first->first.get() : nullptr;

    std::array<const mach_expression*, reg_set_size> base_;
    const auto base = std::span(base_).first(args.size());

    std::transform(args.begin(), args.end(), base.begin(), [this](auto& a) {
        return m_expressions.try_emplace(std::move(a)).first->first.get();
    });
    m_usings.emplace_back(current, b, base, label, e);
    return index_t(m_usings.size());
}

using_collection::index_t using_collection::remove(
    index_t current, std::span<std::unique_ptr<mach_expression>> args, diagnostic_consumer<diagnostic_op>& diag)
{
    std::array<const mach_expression*, reg_set_size> base_;
    const auto base = std::span(base_).first(args.size());

    std::transform(args.begin(), args.end(), base.begin(), [this](auto& a) {
        return m_expressions.try_emplace(std::move(a)).first->first.get();
    });
    m_usings.emplace_back(current, base);

    return index_t(m_usings.size());
}

size_t using_collection::expression_hash::operator()(const std::unique_ptr<mach_expression>& v) const
{
    return v->hash();
}
size_t using_collection::expression_hash::operator()(const mach_expression* v) const { return v->hash(); }

bool using_collection::expression_equal::operator()(
    const std::unique_ptr<mach_expression>& l, const std::unique_ptr<mach_expression>& r) const
{
    return utils::is_similar(*l, *r);
}
bool using_collection::expression_equal::operator()(
    const mach_expression* l, const std::unique_ptr<mach_expression>& r) const
{
    return utils::is_similar(*l, *r);
}
bool using_collection::expression_equal::operator()(
    const std::unique_ptr<mach_expression>& l, const mach_expression* r) const
{
    return utils::is_similar(*l, *r);
}

template</* std::integral */ typename R, /* std::integral */ typename T>
R clamp(T value)
{
    if (value < std::numeric_limits<R>::min())
        return std::numeric_limits<R>::min();
    else if (value > std::numeric_limits<R>::max())
        return std::numeric_limits<R>::min();

    return (R)value;
}

auto using_collection::using_context::evaluate(const using_collection& coll,
    id_index label,
    const section* section,
    long long offset,
    int32_t min_disp,
    int32_t max_disp) const -> evaluate_result
{
    const entry* result_entry = nullptr;
    ptrdiff_t result_reg = 0;
    auto min_dist_abs = std::numeric_limits<long long>::max();
    long long min_dist = 0;
    long long min_dist_reg = -1;
    for (const auto& s : m_state)
    {
        if (label != s.label || section != s.section)
            continue;
        auto new_dist = (offset - s.offset) + s.reg_offset;
        for (const auto& reg : s.regs)
        {
            if (new_dist < min_disp)
                break;
            if (reg != invalid_register)
            {
                auto abs_dist = std::abs(new_dist);
                if (abs_dist < min_dist_abs || abs_dist == min_dist_abs && reg > min_dist_reg)
                {
                    min_dist_abs = abs_dist;
                    min_dist_reg = reg;
                    result_reg = &reg - s.regs.data();
                    result_entry = &s;
                    min_dist = new_dist;
                }
            }
            new_dist -= 0x1000;
        }
    }
    if (!result_entry)
        return evaluate_result { invalid_register_set, 0, 0 };

    evaluate_result result {
        invalid_register_set,
        clamp<offset_t>(min_dist),
        clamp<offset_t>(min_disp < 0 ? 0 : result_entry->length - (offset - result_entry->offset)),
    };
    if (result.reg_offset >= min_disp && result.reg_offset <= max_disp)
        std::copy(result_entry->regs.begin() + result_reg, result_entry->regs.end(), result.mapping_regs.begin());

    return result;
}

constexpr int32_t min_long = -(1 << 19);
constexpr int32_t max_long = (1 << 19) - 1;

constexpr int32_t min_short = 0;
constexpr int32_t max_short = (1 << 12) - 1;

auto using_collection::using_context::evaluate(
    const using_collection& coll, id_index label, const section* section, offset_t offset, bool long_offset) const
    -> evaluate_result
{
    auto result = evaluate(coll, label, section, offset, min_short, max_short);

    if (long_offset && result.mapping_regs == invalid_register_set)
        result = evaluate(coll, label, section, offset, min_long, max_long);

    return result;
}

} // namespace hlasm_plugin::parser_library::context
