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
    context = coll.get_using(p).context;
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

    // drop conflicting usings
    if (u.label)
        compute_context_drop(u.label);

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

size_t using_collection::using_entry::compute_context_drop(register_t d)
{
    size_t invalidated = 0;
    for (auto& e : context.m_state)
    {
        if (e.label == nullptr)
        {
            invalidated += std::count(e.regs.begin(), e.regs.end(), d);
            std::replace(e.regs.begin(), e.regs.end(), d, invalid_register);
        }
    }
    std::erase_if(context.m_state, [](const auto& e) { return e.regs == invalid_register_set; });

    return invalidated;
}

auto using_collection::using_drop_definition::abs_or_reloc(
    using_collection& coll, const mach_expression* e, bool abs_is_register) -> std::optional<qualified_address>
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
        return qualified_address(nullptr, nullptr, v);
    }
    if (res.value_kind() == symbol_value_kind::RELOC && res.get_reloc().is_simple())
    {
        const auto& base = res.get_reloc().bases().front().first;
        return qualified_address(base.qualifier, base.owner, res.get_reloc().offset());
    }

    // TODO: diagnose
    return std::nullopt;
}
std::variant<std::monostate, using_collection::qualified_id, using_collection::register_t>
using_collection::using_drop_definition::abs_or_label(
    using_collection& coll, const mach_expression* e, bool allow_qualification)
{
    if (!e)
        return std::monostate();

    if (auto sym = dynamic_cast<const expressions::mach_expr_symbol*>(e); sym)
    {
        if (sym->qualifier && !allow_qualification)
        {
            // TODO: diagnose
            return std::monostate();
        }
        return qualified_id { sym->qualifier, sym->value };
    }

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
    const qualified_address& base,
    diagnostic_consumer<diagnostic_op>& diag) const
{
    if (!m_parent)
    {
        // TODO: diagnose dependent using without any active using
        return {};
    }
    const auto& ctx = coll.get_using(m_parent).context;

    auto v = ctx.evaluate(base.qualifier, base.sect, base.offset, false);

    if (v.mapping_regs == invalid_register_set)
    {
        // TODO: diagnose no matching using
        return {};
    }

    return using_entry_resolved(
        m_parent, m_label, b.first, b.second, std::min(len.value_or(v.length), v.length), v.mapping_regs, v.reg_offset);
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
    if (b->qualifier)
    {
        // TODO: qualifier not allowed
    }
    auto e = abs_or_reloc(coll, m_end);

    std::array<std::optional<qualified_address>, reg_set_size> bases_;
    std::transform(
        m_base.begin(), m_base.end(), bases_.begin(), [&coll](auto e) { return abs_or_reloc(coll, e, true); });
    const auto bases = std::span(bases_).first(std::find(m_base.begin(), m_base.end(), nullptr) - m_base.begin());

    std::optional<offset_t> len;
    if (e.has_value())
    {
        if (e->qualifier)
        {
            // TODO: qualifier not allowed
        }
        if (b->sect != e->sect)
        {
            // TODO: diagnose
        }
        else if (b->offset >= e->offset)
        {
            // TODO: diagnose
        }
        else
            len = e->offset - b->offset;
    }

    if (bases.size() == 1 && bases.front().has_value() && bases.front()->sect != nullptr)
        return resolve_using_dep(coll, { b->sect, b->offset }, len, bases.front().value(), diag);

    // labeled/ordinary USING continues
    for (auto& base : bases)
    {
        if (base.has_value() && base->sect != nullptr)
        {
            // TODO: diagnose absolute value expected
            base.reset(); // drop the value
        }
    }

    register_set_t reg_set_ = invalid_register_set;
    auto reg_set = std::span(reg_set_).first(bases.size());
    std::transform(bases.begin(), bases.end(), reg_set.begin(), [](const auto& r) {
        return r ? (register_t)r->offset : invalid_register;
    });

    return using_entry_resolved(m_parent, m_label, b->sect, b->offset, len.value_or(0x1000 * bases.size()), reg_set, 0);
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
        void operator()(qualified_id id) { args.emplace_back(id.name); }

    } transform { diag };

    for (const auto* expr : std::span(m_base.begin(), std::find(m_base.begin(), m_base.end(), nullptr)))
        std::visit(transform, abs_or_label(coll, expr, false));

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
        expr.second = expr.first->evaluate(solver);
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



using_collection::evaluate_result using_collection::evaluate(
    index_t context_id, id_index label, const section* section, offset_t offset, bool long_offset) const
{
    if (!context_id)
        return evaluate_result { invalid_register, 0 };

    auto tmp = get_using(context_id).context.evaluate(label, section, offset, long_offset);

    return evaluate_result { tmp.mapping_regs[0], tmp.reg_offset };
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

auto using_collection::using_context::evaluate(
    id_index label, const section* section, long long offset, int32_t min_disp, int32_t max_disp) const
    -> context_evaluate_result
{
    struct result_candidate
    {
        const entry* result_entry = nullptr;
        ptrdiff_t result_reg = 0;
        long long min_dist_abs = std::numeric_limits<long long>::max();
        long long min_dist = 0;
        long long min_dist_reg = -1;
    };
    result_candidate positive;
    result_candidate negative;
    for (const auto& s : m_state)
    {
        if (label != s.label || section != s.section)
            continue;
        auto next_dist = (offset - s.offset) + s.reg_offset;
        for (const auto& reg : s.regs)
        {
            const auto dist = next_dist;
            next_dist -= 0x1000;

            result_candidate& target = dist >= 0 ? positive : negative;
            if (reg != invalid_register)
            {
                auto abs_dist = std::abs(dist);
                if (abs_dist < target.min_dist_abs || abs_dist == target.min_dist_abs && reg > target.min_dist_reg)
                {
                    target.min_dist_abs = abs_dist;
                    target.min_dist_reg = reg;
                    target.result_reg = &reg - s.regs.data();
                    target.result_entry = &s;
                    target.min_dist = dist;
                }
            }
        }
    }

    const auto& r = [min_disp, max_disp](const auto& p, const auto& n) {
        if (p.result_entry && p.min_dist >= min_disp && p.min_dist <= max_disp)
            return p;
        if (n.result_entry && n.min_dist >= min_disp && n.min_dist <= max_disp)
            return n;

        if (p.min_dist_abs <= n.min_dist_abs)
            return p;
        else
            return n;
    }(positive, negative);

    if (!r.result_entry)
        return context_evaluate_result { invalid_register_set, 0, 0 };

    context_evaluate_result result {
        invalid_register_set,
        clamp<offset_t>(r.min_dist),
        clamp<offset_t>(min_disp < 0 ? 0 : r.result_entry->length - (offset - r.result_entry->offset)),
    };

    if (result.reg_offset >= min_disp && result.reg_offset <= max_disp)
        std::copy(r.result_entry->regs.begin() + r.result_reg, r.result_entry->regs.end(), result.mapping_regs.begin());
    else if (result.reg_offset < min_disp)
        result.reg_offset -= min_disp;
    else /* if (result.reg_offset > max_disp) */
        result.reg_offset -= max_disp;

    return result;
}

constexpr int32_t min_long = -(1 << 19);
constexpr int32_t max_long = (1 << 19) - 1;

constexpr int32_t min_short = 0;
constexpr int32_t max_short = (1 << 12) - 1;

auto using_collection::using_context::evaluate(
    id_index label, const section* section, offset_t offset, bool long_offset) const -> context_evaluate_result
{
    if (long_offset)
        return evaluate(label, section, offset, min_long, max_long);
    else
        return evaluate(label, section, offset, min_short, max_short);
}

} // namespace hlasm_plugin::parser_library::context
