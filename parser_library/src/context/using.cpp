/*
 * Copyright (c) 2022 Broadcom.
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

#include <bitset>
#include <iterator>
#include <limits>
#include <span>

#include "context/ordinary_assembly/ordinary_assembly_context.h"
#include "context/ordinary_assembly/section.h"
#include "diagnosable_ctx.h"
#include "diagnostic_consumer.h"
#include "diagnostic_tools.h"
#include "expressions/mach_expr_term.h"
#include "expressions/mach_expression.h"
#include "ordinary_assembly/dependable.h"
#include "ordinary_assembly/ordinary_assembly_dependency_solver.h"
#include "utils/similar.h"

constexpr std::string_view USING = "USING";

namespace hlasm_plugin::parser_library::context {

void using_collection::using_entry::resolve(using_collection& coll, diagnostic_consumer_t<diagnostic_op>& diag)
{
    resolved = definition.resolve(coll, diag);
}

void using_collection::using_entry::compute_context(using_collection& coll, diagnostic_consumer_t<diagnostic_op>& diag)
{
    std::visit(
        [&coll, &diag, this](const auto& e) {
            if (e.parent)
                context = coll.get(e.parent).context;

            compute_context_correction(e, diag);
        },
        resolved);
}

void using_collection::using_entry::compute_context_correction(
    const failed_entry_resolved&, diagnostic_consumer_t<diagnostic_op>&)
{
    // just keep the duplicated previous state on error
}

void using_collection::using_entry::compute_context_correction(
    const using_entry_resolved& u, diagnostic_consumer_t<diagnostic_op>&)
{
    // drop conflicting usings
    if (!u.label.empty())
        compute_context_drop(u.label); // not diagnosed, but maybe we should warn

    context.m_state.emplace_back(using_context::entry { u.label, u.owner, u.begin, u.length, u.reg_set, u.reg_offset });
}

std::string_view convert_diag(const id_index& id) // lifetime! id needs to outlive the return value!
{
    return id.to_string_view();
}
int convert_diag(using_collection::register_t c) { return c; }

void using_collection::using_entry::compute_context_correction(
    const drop_entry_resolved& d, diagnostic_consumer_t<diagnostic_op>& diag)
{
    for (const auto& [drop, rng] : d.drop)
        std::visit(
            [this, &diag, &rng = rng](auto value) {
                if (compute_context_drop(value) == 0)
                {
                    diag.add_diagnostic(diagnostic_op::warn_U001_drop_had_no_effect(rng, convert_diag(value)));
                }
            },
            drop);
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
        if (e.label.empty())
        {
            invalidated += std::ranges::count(e.regs, d);
            std::ranges::replace(e.regs, d, invalid_register);
        }
    }
    std::erase_if(context.m_state, [](const auto& e) { return e.regs == invalid_register_set; });

    return invalidated;
}

auto using_collection::using_drop_definition::abs_or_reloc(const using_collection& coll,
    index_t<mach_expression> e,
    bool abs_is_register) -> std::pair<std::optional<qualified_address>, range>
{
    if (!e)
        return { std::nullopt, range() };

    const auto& expr = coll.get(e);
    const auto& value = expr.value;
    auto rng = expr.expression->get_range();

    if (value.value_kind() == symbol_value_kind::ABS)
    {
        auto v = value.get_abs();
        if (abs_is_register && (v < 0 || v >= reg_set_size))
        {
            return { std::nullopt, rng };
        }
        return { qualified_address(id_index(), nullptr, v), rng };
    }
    if (value.value_kind() == symbol_value_kind::RELOC && value.get_reloc().is_simple())
    {
        const auto& base = value.get_reloc().bases().front();
        return { qualified_address(base.qualifier, base.owner, value.get_reloc().offset()), rng };
    }

    return { std::nullopt, rng };
}
auto using_collection::using_drop_definition::reg_or_label(const using_collection& coll,
    index_t<mach_expression> e) -> std::pair<std::variant<std::monostate, qualified_id, register_t>, range>
{
    if (!e)
        return { std::monostate(), range() };

    const auto& expr = coll.get(e);
    auto rng = expr.expression->get_range();

    if (!expr.label.empty())
    {
        return { qualified_id { id_index(), expr.label }, rng };
    }

    if (expr.value.value_kind() == symbol_value_kind::ABS)
    {
        if (auto v = expr.value.get_abs(); v >= 0 && v < reg_set_size)
            return { (register_t)v, rng };
    }

    return { std::monostate(), rng };
}

using_collection::resolved_entry using_collection::using_drop_definition::resolve_using_dep(
    const using_collection& coll,
    const std::pair<const section*, offset_t>& b,
    std::optional<offset_t> len,
    const qualified_address& base,
    const range& rng,
    diagnostic_consumer_t<diagnostic_op>& diag) const
{
    if (!m_parent)
    {
        diag.add_diagnostic(diagnostic_op::error_U004_no_active_using(rng));
        return {};
    }
    const auto& ctx = coll.get(m_parent).context;

    auto v = ctx.evaluate(base.qualifier, base.sect, base.offset, false);

    if (v.mapping_regs == invalid_register_set)
    {
        diag.add_diagnostic(diagnostic_op::error_U004_no_active_using(rng));
        return failed_entry_resolved { m_parent };
    }

    return using_entry_resolved(
        m_parent, m_label, b.first, b.second, std::min(len.value_or(v.length), v.length), v.mapping_regs, v.reg_offset);
}

using_collection::resolved_entry using_collection::using_drop_definition::resolve_using(
    const using_collection& coll, diagnostic_consumer_t<diagnostic_op>& diag) const
{
    assert(!m_base.empty() && m_base.size() <= reg_set_size);

    auto [b, b_rng] = abs_or_reloc(coll, m_begin, false);
    if (!b.has_value())
    {
        diag.add_diagnostic(diagnostic_op::error_M113(USING, b_rng));
        return failed_entry_resolved { m_parent };
    }
    if (!b->qualifier.empty())
    {
        // diagnose and ignore
        diag.add_diagnostic(diagnostic_op::error_U002_label_not_allowed(b_rng));
    }
    auto [e, e_rng] = abs_or_reloc(coll, m_end, false);

    std::array<std::pair<std::optional<qualified_address>, range>, reg_set_size> bases_;
    std::ranges::transform(m_base, bases_.begin(), [&coll](auto base) { return abs_or_reloc(coll, base, true); });
    const auto bases = std::span(bases_).first(std::ranges::find(m_base, index_t<mach_expression>()) - m_base.begin());

    std::optional<offset_t> len;
    if (e.has_value())
    {
        if (!e->qualifier.empty())
        {
            // diagnose and ignore
            diag.add_diagnostic(diagnostic_op::error_U002_label_not_allowed(e_rng));
        }
        if (b->sect != e->sect || b->offset >= e->offset)
        {
            constexpr auto section_name = [](const section* s) {
                if (s)
                    return s->name.to_string_view();
                else
                    return std::string_view();
            };
            // diagnose and ignore
            diag.add_diagnostic(diagnostic_op::error_U005_invalid_range(
                b_rng, e_rng, section_name(b->sect), b->offset, section_name(e->sect), e->offset));
        }
        else
            len = e->offset - b->offset;
    }
    else if (m_end)
        diag.add_diagnostic(diagnostic_op::error_M113(USING, e_rng)); // diagnose and ignore

    if (bases.size() == 1 && bases.front().first.has_value() && bases.front().first->sect != nullptr)
        return resolve_using_dep(
            coll, { b->sect, b->offset }, len, bases.front().first.value(), bases.front().second, diag);

    // labeled/ordinary USING continues
    for (auto& [base, base_rng] : bases)
    {
        if (!base.has_value() || base->sect != nullptr)
        {
            diag.add_diagnostic(diagnostic_op::error_M120(USING, base_rng));
            base.reset(); // ignore the value
        }
    }

    register_set_t reg_set_ = invalid_register_set;
    auto reg_set = std::span(reg_set_).first(bases.size());
    std::ranges::transform(
        bases, reg_set.begin(), [](const auto& r) { return r.first ? (register_t)r.first->offset : invalid_register; });
    std::bitset<reg_set_size> test_regs;
    for (size_t i = 0; i < reg_set.size(); ++i)
    {
        auto r = reg_set[i];
        if (r == invalid_register)
            continue;
        if (test_regs.test(r))
        {
            diag.add_diagnostic(diagnostic_op::error_U006_duplicate_base_specified(bases[i].second));
            break;
        }
        test_regs.set(r);
    }

    return using_entry_resolved(m_parent, m_label, b->sect, b->offset, len.value_or(0x1000 * bases.size()), reg_set, 0);
}

using_collection::resolved_entry using_collection::using_drop_definition::resolve_drop(
    const using_collection& coll, diagnostic_consumer_t<diagnostic_op>& diag) const
{
    struct
    {
        diagnostic_consumer_t<diagnostic_op>& diag;
        std::vector<std::pair<std::variant<id_index, register_t>, range>> args;

        void operator()(std::monostate, range rng)
        {
            diag.add_diagnostic(diagnostic_op::error_U003_drop_label_or_reg(rng));
        }
        void operator()(register_t r, range rng) { args.emplace_back(r, rng); }
        void operator()(qualified_id id, range rng)
        {
            if (!id.qualifier.empty())
                diag.add_diagnostic(diagnostic_op::error_U002_label_not_allowed(rng));
            args.emplace_back(id.name, rng);
        }

    } transform { diag };

    for (const auto& expr : std::span(m_base.begin(), std::ranges::find(m_base, index_t<mach_expression>())))
    {
        auto [v, rng] = reg_or_label(coll, expr);
        std::visit([&transform, &rng = rng](auto value) { transform(value, rng); }, v);
    }

    return drop_entry_resolved(m_parent, std::move(transform.args));
}

using_collection::resolved_entry using_collection::using_drop_definition::resolve(
    using_collection& coll, diagnostic_consumer_t<diagnostic_op>& diag) const
{
    if (is_using())
        return resolve_using(coll, diag);
    else if (is_drop())
        return resolve_drop(coll, diag);
    else
        assert(false);

    return {};
}

using_collection::using_collection(using_collection&&) noexcept = default;
using_collection& using_collection::operator=(using_collection&&) noexcept = default;
using_collection::~using_collection() = default;

namespace {
id_index identify_label(const ordinary_assembly_context& ord_context, const expressions::mach_expression* expression)
{
    if (auto sym = dynamic_cast<const expressions::mach_expr_symbol*>(expression);
        sym && sym->qualifier.empty() && ord_context.is_using_label(sym->value))
        return sym->value;

    return id_index();
}
} // namespace

void using_collection::resolve_all(
    ordinary_assembly_context& ord_context, diagnostic_consumer_t<diagnostic>& diag, const library_info& li)
{
    assert(!m_resolved);

    const auto evaluate_expression = [&ord_context, &diag, this, &li](expression_value& expr) {
        expr.label = identify_label(ord_context, expr.expression.get());
        if (expr.label.empty())
        {
            const auto& expr_context = get(expr.context);
            ordinary_assembly_dependency_solver solver(ord_context, expr_context.evaluation_ctx, li);
            diagnostic_consumer_transform diag_collector([&diag, &expr_context](diagnostic_op d) {
                diag.add_diagnostic(add_stack_details(std::move(d), expr_context.stack));
            });

            expr.value = expr.expression->evaluate(solver, diag_collector);
        }
    };

    size_t expr_id = 0;
    for (auto& u : m_usings)
    {
        assert(u.expression_used_limit);

        for (; expr_id != u.expression_used_limit.value(); ++expr_id)
            evaluate_expression(m_expr_values[expr_id]);

        diagnostic_consumer_transform t([this, &diag, &u](diagnostic_op d) {
            diag.add_diagnostic(add_stack_details(std::move(d), get(u.instruction_ctx).stack));
        });
        u.resolve(*this, t);
        u.compute_context(*this, t);
    }

    m_resolved = true;
}

index_t<using_collection::instruction_context> using_collection::add(
    dependency_evaluation_context ctx, processing_stack_t stack)
{
    m_instruction_contexts.emplace_back(std::move(ctx), std::move(stack));
    return index_t<instruction_context>(m_instruction_contexts.size() - 1);
}

index_t<using_collection::mach_expression> using_collection::add(
    std::unique_ptr<const mach_expression> expr, index_t<instruction_context> ctx)
{
    m_expr_values.emplace_back(std::move(expr), ctx);
    return index_t<mach_expression>(m_expr_values.size() - 1);
}


index_t<using_collection> using_collection::add(index_t<using_collection> current,
    id_index label,
    std::unique_ptr<mach_expression> begin,
    std::unique_ptr<mach_expression> end,
    std::vector<std::unique_ptr<mach_expression>> args,
    dependency_evaluation_context eval_ctx,
    processing_stack_t stack)
{
    assert(!args.empty() && args.size() <= reg_set_size);

    index_t<instruction_context> ctx = add(std::move(eval_ctx), std::move(stack));

    auto b = add(std::move(begin), ctx);
    auto e = end ? add(std::move(end), ctx) : index_t<mach_expression>();

    std::vector<index_t<mach_expression>> base;
    base.reserve(args.size());

    std::ranges::transform(args, std::back_inserter(base), [this, ctx](auto& a) { return add(std::move(a), ctx); });

    m_usings.emplace_back(current, index_t<mach_expression>(m_expr_values.size()), ctx, b, std::move(base), label, e);
    return index_t<using_collection>(m_usings.size() - 1);
}

index_t<using_collection> using_collection::remove(index_t<using_collection> current,
    std::vector<std::unique_ptr<mach_expression>> args,
    dependency_evaluation_context eval_ctx,
    processing_stack_t stack)
{
    assert(!args.empty());
    index_t<instruction_context> ctx = add(std::move(eval_ctx), std::move(stack));

    std::vector<index_t<mach_expression>> base;
    base.reserve(args.size());

    std::ranges::transform(args, std::back_inserter(base), [this, ctx](auto& a) { return add(std::move(a), ctx); });
    m_usings.emplace_back(current, index_t<mach_expression>(m_expr_values.size()), ctx, std::move(base));

    return index_t<using_collection>(m_usings.size() - 1);
}

using_collection::evaluate_result using_collection::evaluate(
    index_t<using_collection> context_id, id_index label, const section* owner, offset_t offset, bool long_offset) const
{
    if (!context_id)
        return evaluate_result { invalid_register, 0 };

    auto tmp = get(context_id).context.evaluate(label, owner, offset, long_offset);

    if (tmp.length < 0)
        return evaluate_result { invalid_register, 1 - tmp.length };
    else
        return evaluate_result { tmp.mapping_regs[0], tmp.reg_offset };
}

bool hlasm_plugin::parser_library::context::using_collection::is_label_mapping_section(
    index_t<using_collection> context_id, id_index label, const section* owner) const
{
    assert(m_resolved);

    if (!context_id)
        return false;

    const auto& state = get(context_id).context.m_state;

    return std::ranges::any_of(state, [label, owner](const auto& s) { return s.label == label && s.owner == owner; });
}

template</* std::integral */ typename R, /* std::integral */ typename T>
R clamp(T value)
{
    if (value < std::numeric_limits<R>::min())
        return std::numeric_limits<R>::min();
    else if (value > std::numeric_limits<R>::max())
        return std::numeric_limits<R>::max();

    return (R)value;
}

auto using_collection::using_context::evaluate(id_index label,
    const section* owner,
    long long offset,
    int32_t min_disp,
    int32_t max_disp,
    bool ignore_length) const -> context_evaluate_result
{
    struct result_candidate_entry
    {
        const entry* result_entry = nullptr;
        ptrdiff_t result_reg = 0;
        long long min_dist_abs = std::numeric_limits<long long>::max();
        long long min_dist = 0;
        int min_dist_reg = -1;

        bool is_better_candidate(long long new_abs_dist, register_t new_reg) const
        {
            return new_abs_dist < min_dist_abs || new_abs_dist == min_dist_abs && new_reg > min_dist_reg;
        }
    };
    struct result_candidate
    {
        result_candidate_entry valid;
        result_candidate_entry invalid;
    };
    result_candidate positive;
    result_candidate negative;

    if (owner == nullptr && min_disp <= offset && offset <= max_disp)
    {
        // implicit 0 mapping
        static constexpr entry zero_entry {
            id_index(),
            nullptr,
            0,
            0x1000,
            []() {
                register_set_t zero_reg = invalid_register_set;
                zero_reg[0] = 0;
                return zero_reg;
            }(),
            0,
        };
        (offset >= 0 ? positive : negative) = result_candidate { &zero_entry, 0, std::abs(offset), offset, 0 };
    }

    for (const auto& s : m_state)
    {
        if (label != s.label || owner != s.owner)
            continue;
        auto next_dist = (offset - s.offset) + s.reg_offset;
        const bool fits_limit = next_dist < s.length;
        for (const auto& reg : s.regs)
        {
            const auto dist = next_dist;
            next_dist -= 0x1000;

            if (reg == invalid_register)
                continue;

            result_candidate& target = dist >= 0 ? positive : negative;
            auto abs_dist = std::abs(dist);
            if (auto& t = target.invalid; t.is_better_candidate(abs_dist, reg))
            {
                t.min_dist_abs = abs_dist;
                t.min_dist_reg = reg;
                t.result_reg = &reg - s.regs.data();
                t.result_entry = &s;
                t.min_dist = dist;
            }
            if (auto& t = target.valid; t.is_better_candidate(abs_dist, reg) && dist >= min_disp && dist <= max_disp
                && (ignore_length || fits_limit))
            {
                t.min_dist_abs = abs_dist;
                t.min_dist_reg = reg;
                t.result_reg = &reg - s.regs.data();
                t.result_entry = &s;
                t.min_dist = dist;
            }
        }
    }

    const auto& r = [](const auto& p, const auto& n) -> const auto& {
        if (p.valid.result_entry)
            return p.valid;
        if (n.valid.result_entry)
            return n.valid;

        if (p.invalid.result_entry && n.invalid.result_entry)
        {
            if (p.invalid.min_dist_abs <= n.invalid.min_dist_abs)
                return p.invalid;
            else
                return n.invalid;
        }
        else if (p.invalid.result_entry)
            return p.invalid;
        else
            return n.invalid;
    }(positive, negative);

    if (!r.result_entry)
        return context_evaluate_result { invalid_register_set, 0, 0 };

    context_evaluate_result result {
        invalid_register_set,
        clamp<offset_t>(r.min_dist),
        clamp<offset_t>(ignore_length ? 0 : r.result_entry->length - (offset - r.result_entry->offset)),
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
    id_index label, const section* owner, offset_t offset, bool long_offset) const -> context_evaluate_result
{
    if (long_offset)
        return evaluate(label, owner, offset, min_long, max_long, true);
    else
        return evaluate(label, owner, offset, min_short, max_short, false);
}

std::vector<using_context_description> using_collection::describe(index_t<using_collection> context_id) const
{
    if (!context_id)
        return {};

    std::vector<using_context_description> result;

    for (const auto& u : get(context_id).context.m_state)
        result.emplace_back(u.label,
            u.owner ? std::optional(u.owner->name) : std::nullopt,
            u.offset,
            (unsigned long)u.length,
            u.reg_offset,
            std::vector<using_collection::register_t>(u.regs.begin(), std::ranges::find(u.regs, invalid_register)));

    return result;
}

} // namespace hlasm_plugin::parser_library::context
