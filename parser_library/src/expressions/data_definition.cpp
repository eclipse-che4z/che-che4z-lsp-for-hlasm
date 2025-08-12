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

#include "data_definition.h"

#include <assert.h>

#include "checking/data_definition/data_def_fields.h"
#include "checking/data_definition/data_def_type_base.h"
#include "checking/diagnostic_collector.h"
#include "compiler_options.h"
#include "context/ordinary_assembly/section.h"
#include "context/ordinary_assembly/symbol.h"
#include "context/using.h"
#include "ebcdic_encoding.h"
#include "mach_expr_term.h"
#include "mach_expr_visitor.h"
#include "semantics/collector.h"
#include "utils/general_hashers.h"
#include "utils/similar.h"

namespace hlasm_plugin::parser_library::expressions {
using utils::hashers::hash_combine;

constexpr char V_type = 'V';
constexpr char R_type = 'R';
constexpr char R_type_suppressed = 'r';

context::dependency_collector data_definition::get_dependencies(context::dependency_solver& solver) const
{
    context::dependency_collector deps = get_length_dependencies(solver);

    if (scale)
        deps.merge(scale->get_dependencies(solver));
    if (exponent)
        deps.merge(exponent->get_dependencies(solver));

    // In V type, the symbols are external, it is not defined in current program and does not
    // have any dependencies.
    if (type != V_type && type != R_type && type != R_type_suppressed && nominal_value)
        deps.merge(nominal_value->get_dependencies(solver));

    return deps;
}

context::dependency_collector data_definition::get_length_dependencies(context::dependency_solver& solver) const
{
    auto result = dupl_factor ? dupl_factor->get_dependencies(solver) : context::dependency_collector();
    result *= length ? length->get_dependencies(solver) : context::dependency_collector();
    return result;
}

const checking::data_def_type* data_definition::access_data_def_type() const
{
    return checking::data_def_type::access_data_def_type(type, extension);
}

context::alignment data_definition::get_alignment() const
{
    auto def_type = access_data_def_type();
    if (def_type)
        return def_type->get_alignment(length != nullptr);
    else
        return context::no_align;
}

char data_definition::get_type_attribute() const
{
    switch (type)
    {
        case 'B':
        case 'C':
        case 'P':
        case 'X':
        case 'Z':
            return type;
        case 'G':
            return '@';
        default:
            break;
    }
    if (length == nullptr)
    {
        switch (type)
        {
            case 'A':
            case 'J':
                return 'A';
            case 'D':
            case 'E':
            case 'F':
            case 'H':
            case 'L':
            case 'Q':
            case 'S':
            case 'V':
            case 'Y':
                return type;
            case 'R':
                return 'V';
            default:
                break;
        }
    }
    else
    {
        switch (type)
        {
            case 'F':
            case 'H':
                return 'G';
            case 'E':
            case 'D':
            case 'L':
                return 'K';
            case 'A':
            case 'S':
            case 'Q':
            case 'J':
            case 'R':
            case 'V':
            case 'Y':
                return 'R';
            default:
                break;
        }
    }
    return 'U';
}

int32_t data_definition::get_scale_attribute(context::dependency_solver& info, diagnostic_op_consumer& diags) const
{
    auto def_type = access_data_def_type();
    if (def_type)
        return def_type->get_scale_attribute(evaluate_scale(info, diags), evaluate_reduced_nominal_value());
    else
        return 0;
}

uint32_t data_definition::get_length_attribute(context::dependency_solver& info, diagnostic_op_consumer& diags) const
{
    auto def_type = access_data_def_type();
    if (def_type)
        return def_type->get_length_attribute(evaluate_length(info, diags), evaluate_reduced_nominal_value());
    else
        return 0;
}

context::integer_type data_definition::get_integer_attribute() const noexcept
{
    auto def_type = access_data_def_type();
    if (def_type)
        return def_type->get_int_type();
    else
        return context::integer_type::undefined;
}

context::symbol_attributes::program_type data_definition::get_program_attribute(
    context::dependency_solver& info, diagnostic_op_consumer& diags) const
{
    if (!program_type)
        return {};
    const auto p = program_type->evaluate(info, diags);
    if (p.value_kind() != context::symbol_value_kind::ABS)
        return {};
    return context::symbol_attributes::program_type((std::uint32_t)p.get_abs());
}

context::symbol_attributes data_definition::get_symbol_attributes(
    context::dependency_solver& solver, diagnostic_op_consumer& diags) const
{
    return context::symbol_attributes(context::symbol_origin::DAT,
        ebcdic_encoding::to_ebcdic((unsigned char)get_type_attribute()),
        get_length_attribute(solver, diags),
        get_scale_attribute(solver, diags),
        get_integer_attribute(),
        get_program_attribute(solver, diags));
}

bool data_definition::expects_single_symbol() const
{
    auto def_type = access_data_def_type();
    if (def_type)
        return def_type->expects_single_symbol();
    else
        return false;
}

bool data_definition::check_single_symbol_ok(diagnostic_op_consumer& diags) const
{
    const auto* exprs = nominal_value->access_exprs();
    if (!exprs)
        return true;

    bool ret = true;
    for (const auto& expr_or_addr : exprs->exprs)
    {
        if (!std::holds_alternative<mach_expr_ptr>(expr_or_addr))
        {
            diags.add_diagnostic(
                diagnostic_op::error_D030({ std::get<address_nominal>(expr_or_addr).base->get_range().start,
                                              std::get<address_nominal>(expr_or_addr).base->get_range().end },
                    std::string(1, type)));
            ret = false;
            continue;
        }
        const mach_expression* expr = std::get<mach_expr_ptr>(expr_or_addr).get();
        auto symbol = dynamic_cast<const mach_expr_symbol*>(expr);
        if (!symbol)
        {
            diags.add_diagnostic(diagnostic_op::error_D030(expr->get_range(), std::string(1, type)));
            ret = false;
        }
    }
    return ret;
}

checking::data_def_field<int32_t> set_data_def_field(
    const expressions::mach_expression* e, context::dependency_solver& info, diagnostic_op_consumer& diags)
{
    using namespace checking;
    data_def_field<int32_t> field;
    // if the expression cannot be evaluated, we return field as if it was not there
    if (e)
    {
        field.rng = e->get_range();

        if (field.present = !e->get_dependencies(info).contains_dependencies())
        {
            auto ret = e->evaluate(info, diags);

            if (ret.value_kind() == context::symbol_value_kind::ABS)
                field.value = ret.get_abs();
            else
            {
                field.present = false;
                diags.add_diagnostic(diagnostic_op::error_D034(e->get_range()));
            }
        }
    }
    return field;
}

checking::dupl_factor_modifier_t data_definition::evaluate_dupl_factor(
    context::dependency_solver& info, diagnostic_op_consumer& diags) const
{
    return dupl_factor ? set_data_def_field(dupl_factor.get(), info, diags) : checking::data_def_field<int32_t>(1);
}

checking::data_def_length_t data_definition::evaluate_length(
    context::dependency_solver& info, diagnostic_op_consumer& diags) const
{
    checking::data_def_length_t len(set_data_def_field(length.get(), info, diags));
    len.len_type = length_type == expressions::data_definition::length_type::BIT ? checking::data_def_length_t::BIT
                                                                                 : checking::data_def_length_t::BYTE;
    return len;
}

checking::scale_modifier_t data_definition::evaluate_scale(
    context::dependency_solver& info, diagnostic_op_consumer& diags) const
{
    auto common = set_data_def_field(scale.get(), info, diags);
    return checking::scale_modifier_t(common.present, (int16_t)common.value, common.rng);
}

checking::exponent_modifier_t data_definition::evaluate_exponent(
    context::dependency_solver& info, diagnostic_op_consumer& diags) const
{
    return set_data_def_field(exponent.get(), info, diags);
}

enum class nominal_eval_subtype
{
    none,
    S_type,
    SY_type,
};

struct extract_nominal_value_visitor
{
    context::dependency_solver& info;
    diagnostic_op_consumer& diags;
    nominal_eval_subtype type;

    checking::expr_or_address handle_simple_reloc_with_using_map(const context::address& addr, range r) const
    {
        checking::data_def_address ch_adr;
        ch_adr.total = r;

        const auto& base = addr.bases().front().first;
        auto translated_addr =
            info.using_evaluate(base.qualifier, base.owner, addr.offset(), type == nominal_eval_subtype::SY_type);

        if (translated_addr.reg == context::using_collection::invalid_register)
        {
            if (translated_addr.reg_offset)
                diags.add_diagnostic(diagnostic_op::error_ME008(translated_addr.reg_offset, r));
            else
                diags.add_diagnostic(diagnostic_op::error_ME007(r));
            ch_adr.ignored = true; // already diagnosed
        }
        else
        {
            ch_adr.displacement = translated_addr.reg_offset;
            ch_adr.base = translated_addr.reg;
        }

        return ch_adr;
    }

    checking::expr_or_address operator()(const expressions::mach_expr_ptr& e) const
    {
        auto deps = e->get_dependencies(info);
        bool ignored = deps.has_error || deps.contains_dependencies(); // ignore values with dependencies
        auto ev = e->evaluate(info, diags);
        switch (ev.value_kind())
        {
            case context::symbol_value_kind::UNDEF:
                return checking::data_def_expr { 0, checking::expr_type::ABS, e->get_range(), true };

            case context::symbol_value_kind::ABS:
                return checking::data_def_expr {
                    ev.get_abs(),
                    checking::expr_type::ABS,
                    e->get_range(),
                    ignored,
                };

            case context::symbol_value_kind::RELOC:
                if (const auto& addr = ev.get_reloc(); type != nominal_eval_subtype::none && addr.is_simple())
                    return handle_simple_reloc_with_using_map(addr, e->get_range());
                else
                    return checking::data_def_expr {
                        0,
                        addr.is_complex() ? checking::expr_type::COMPLEX : checking::expr_type::RELOC,
                        e->get_range(),
                        ignored,
                    };

            default:
                assert(false);
                return checking::data_def_expr {};
        }
    }

    checking::expr_or_address operator()(const expressions::address_nominal& a) const
    {
        checking::data_def_address ch_adr;

        ch_adr.total = a.total;
        ch_adr.base = set_data_def_field(a.base.get(), info, diags);
        ch_adr.displacement = set_data_def_field(a.displacement.get(), info, diags);
        return ch_adr;
    }
};

checking::nominal_value_expressions extract_nominal_value_expressions(const expr_or_address_list& exprs,
    context::dependency_solver& info,
    diagnostic_op_consumer& diags,
    nominal_eval_subtype type)
{
    extract_nominal_value_visitor visitor { info, diags, type };
    checking::nominal_value_expressions values;
    for (const auto& e_or_a : exprs)
        values.push_back(std::visit(visitor, e_or_a));

    return values;
}

constexpr bool is_valid_external_symbol(const context::section& s) noexcept
{
    using enum context::section_kind;
    return s.kind == DUMMY || s.kind == EXTERNAL_DSECT;
}

checking::nominal_value_expressions process_q_nominal(
    const expr_or_address_list& exprs, context::dependency_solver& info, diagnostic_op_consumer& diags)
{
    static constexpr std::string_view type = "Q";
    checking::nominal_value_expressions result;
    result.reserve(exprs.size());

    const auto goff = info.get_options().sysopt_xobject;

    for (const auto& expr_or_addr : exprs)
    {
        result.emplace_back(checking::data_def_expr { .ignored = true }); // everything is solved here

        if (!std::holds_alternative<mach_expr_ptr>(expr_or_addr))
        {
            diags.add_diagnostic(
                diagnostic_op::error_D030({ std::get<address_nominal>(expr_or_addr).base->get_range().start,
                                              std::get<address_nominal>(expr_or_addr).base->get_range().end },
                    type));
            continue;
        }
        const mach_expression* expr = std::get<mach_expr_ptr>(expr_or_addr).get();
        const auto* symbol_expr = dynamic_cast<const mach_expr_symbol*>(expr);
        if (!symbol_expr)
        {
            diags.add_diagnostic(diagnostic_op::error_D030(expr->get_range(), type));
            continue;
        }
        if (goff)
        {
            if (const auto* s = info.get_symbol(symbol_expr->value);
                s && s->attributes().origin() == context::symbol_origin::SECT)
                continue;
        }
        else if (const auto* s = info.get_section(symbol_expr->value); s && is_valid_external_symbol(*s))
            continue;

        diags.add_diagnostic(diagnostic_op::error_D035(symbol_expr->get_range(), goff));
    }

    return result;
}

checking::nominal_value_t data_definition::evaluate_nominal_value(
    context::dependency_solver& info, diagnostic_op_consumer& diags) const
{
    if (!nominal_value)
        return {};

    checking::nominal_value_t nom;
    nom.rng = nominal_value->value_range;
    nom.present = true;
    if (const auto* str = nominal_value->access_string())
    {
        nom.value = str->value;
    }
    else if (const auto* exprs = nominal_value->access_exprs())
    {
        // TODO: yes, this needs to be cleaned up
        if (type == 'Q')
            nom.value = process_q_nominal(exprs->exprs, info, diags);
        else
        {
            if (expects_single_symbol())
                (void)check_single_symbol_ok(diags);
            nom.value = extract_nominal_value_expressions(exprs->exprs,
                info,
                diags,
                type != 'S' ? nominal_eval_subtype::none
                            : (extension == 'Y' ? nominal_eval_subtype::SY_type : nominal_eval_subtype::S_type));
        }
    }
    else
        assert(false);

    return nom;
}

checking::reduced_nominal_value_t data_definition::evaluate_reduced_nominal_value() const
{
    if (!nominal_value)
        return {};

    checking::reduced_nominal_value_t nom;
    nom.present = true;
    if (const auto* str = nominal_value->access_string())
    {
        nom.value = str->value;
        nom.rng = str->value_range;
    }
    else if (const auto* exprs = nominal_value->access_exprs())
    {
        nom.value = exprs->exprs.size();
    }
    else
        assert(false);

    return nom;
}

long long data_definition::evaluate_total_length(
    context::dependency_solver& info, checking::data_instr_type checking_rules, diagnostic_op_consumer& diags) const
{
    auto dd_type = checking::data_def_type::access_data_def_type(type, extension);
    if (!dd_type)
        return -1;
    auto dupl = evaluate_dupl_factor(info, diags);
    auto len = evaluate_length(info, diags);

    diagnostic_collector drop_diags;

    if (!dd_type->check_dupl_factor(dupl, drop_diags))
        return -1;

    if (!dd_type->check_length(len, checking_rules, drop_diags))
        return -1;

    auto result = dd_type->get_length(dupl, len, evaluate_reduced_nominal_value());
    return result >= ((1ll << 31) - 1) * 8 ? -1 : (long long)result;
}

void data_definition::apply(mach_expr_visitor& visitor) const
{
    if (dupl_factor)
        dupl_factor->apply(visitor);
    if (program_type)
        program_type->apply(visitor);
    if (length)
        length->apply(visitor);
    if (scale)
        scale->apply(visitor);
    if (exponent)
        exponent->apply(visitor);

    if (const auto* exprs = nominal_value ? nominal_value->access_exprs() : nullptr)
    {
        for (const auto& val : exprs->exprs)
        {
            if (std::holds_alternative<expressions::mach_expr_ptr>(val))
                std::get<expressions::mach_expr_ptr>(val)->apply(visitor);
            else
            {
                const auto& addr = std::get<expressions::address_nominal>(val);
                if (addr.base)
                    addr.base->apply(visitor);
                if (addr.displacement)
                    addr.displacement->apply(visitor);
            }
        }
    }
}

size_t data_definition::hash() const
{
    auto ret = (size_t)0x65b40f329f97f6c9;
    ret = hash_combine(ret, type);
    ret = hash_combine(ret, extension);
    if (length)
        ret = hash_combine(ret, length->hash());

    ret = hash_combine(ret, (size_t)length_type);
    if (dupl_factor)
        ret = hash_combine(ret, dupl_factor->hash());
    if (program_type)
        ret = hash_combine(ret, program_type->hash());
    if (scale)
        ret = hash_combine(ret, scale->hash());
    if (exponent)
        ret = hash_combine(ret, exponent->hash());
    if (nominal_value)
        ret = hash_combine(ret, nominal_value->hash());

    return ret;
}

bool is_similar(const data_definition& l, const data_definition& r) noexcept
{
    return utils::is_similar(l,
        r,
        &data_definition::type,
        &data_definition::extension,
        &data_definition::length,
        &data_definition::length_type,
        &data_definition::dupl_factor,
        &data_definition::program_type,
        &data_definition::scale,
        &data_definition::exponent,
        &data_definition::nominal_value);
}

} // namespace hlasm_plugin::parser_library::expressions
