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
#include <limits>
#include <set>
#include <stdexcept>

#include "checking/data_definition/data_def_fields.h"
#include "checking/data_definition/data_def_type_base.h"
#include "mach_expr_term.h"
#include "mach_expr_visitor.h"
#include "semantics/collector.h"
#include "utils/similar.h"

using namespace hlasm_plugin::parser_library::expressions;
using namespace hlasm_plugin::parser_library;


data_definition data_definition::create(
    semantics::collector& coll, std::string format, mach_expr_list exprs, nominal_value_ptr nominal, position begin)
{
    parser p(coll, std::move(format), std::move(exprs), std::move(nominal), begin);
    return p.parse();
}

void insert_deps(
    context::dependency_collector& into, context::dependency_solver& solver, const context::dependable* from)
{
    if (from)
        into = into + from->get_dependencies(solver);
}

constexpr char V_type = 'V';

context::dependency_collector data_definition::get_dependencies(context::dependency_solver& solver) const
{
    context::dependency_collector conjuction;
    // In V type, the symbols are external, it is not defined in current program and does not
    // have any dependencies.
    if (type != V_type)
        insert_deps(conjuction, solver, nominal_value.get());
    insert_deps(conjuction, solver, dupl_factor.get());
    insert_deps(conjuction, solver, length.get());
    insert_deps(conjuction, solver, scale.get());
    insert_deps(conjuction, solver, exponent.get());
    return conjuction;
}

context::dependency_collector data_definition::get_length_dependencies(context::dependency_solver& solver) const
{
    context::dependency_collector conjuction;
    insert_deps(conjuction, solver, dupl_factor.get());
    insert_deps(conjuction, solver, length.get());
    return conjuction;
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

char hlasm_plugin::parser_library::expressions::data_definition::get_type_attribute() const
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

int32_t data_definition::get_scale_attribute(context::dependency_solver& info) const
{
    auto def_type = access_data_def_type();
    if (def_type)
        return def_type->get_scale_attribute(evaluate_scale(info), evaluate_nominal_value(info));
    else
        return 0;
}

uint32_t data_definition::get_length_attribute(context::dependency_solver& info) const
{
    auto def_type = access_data_def_type();
    if (def_type)
        return def_type->get_length_attribute(evaluate_length(info), evaluate_nominal_value(info));
    else
        return 0;
}

int32_t data_definition::get_integer_attribute(context::dependency_solver& info) const
{
    auto def_type = access_data_def_type();
    if (def_type)
        return def_type->get_integer_attribute(
            evaluate_length(info), evaluate_scale(info), evaluate_nominal_value(info));
    else
        return 0;
}

bool data_definition::expects_single_symbol() const
{
    auto def_type = access_data_def_type();
    if (def_type)
        return def_type->expects_single_symbol();
    else
        return false;
}

bool data_definition::check_single_symbol_ok(const diagnostic_collector& add_diagnostic) const
{
    if (!expects_single_symbol() || !nominal_value)
        return true;
    if (!nominal_value->access_exprs())
        return true;

    bool ret = true;
    for (const auto& expr_or_addr : nominal_value->access_exprs()->exprs)
    {
        if (!std::holds_alternative<mach_expr_ptr>(expr_or_addr))
        {
            add_diagnostic(
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
            add_diagnostic(diagnostic_op::error_D030(expr->get_range(), std::string(1, type)));
            ret = false;
        }
    }
    return ret;
}

std::vector<context::id_index> data_definition::get_single_symbol_names() const
{
    // expects that check_single_symbol_ok returned true
    assert(check_single_symbol_ok(diagnostic_collector()));

    std::vector<context::id_index> symbols;
    symbols.reserve(nominal_value->access_exprs()->exprs.size());
    for (const auto& expr_or_addr : nominal_value->access_exprs()->exprs)
    {
        const mach_expression* expr = std::get<mach_expr_ptr>(expr_or_addr).get();
        const auto& symbol = dynamic_cast<const mach_expr_symbol&>(*expr);
        symbols.push_back(symbol.value);
    }
    return symbols;
}

void data_definition::collect_diags() const
{
    if (dupl_factor)
        collect_diags_from_child(*dupl_factor);
    if (program_type)
        collect_diags_from_child(*program_type);
    if (length)
        collect_diags_from_child(*length);
    if (scale)
        collect_diags_from_child(*scale);
    if (exponent)
        collect_diags_from_child(*exponent);

    if (nominal_value && nominal_value->access_exprs())
    {
        for (const auto& val : nominal_value->access_exprs()->exprs)
        {
            if (std::holds_alternative<expressions::mach_expr_ptr>(val))
                collect_diags_from_child(*std::get<expressions::mach_expr_ptr>(val));
            else
            {
                const auto& addr = std::get<expressions::address_nominal>(val);
                if (addr.base)
                    collect_diags_from_child(*addr.base);
                if (addr.displacement)
                    collect_diags_from_child(*addr.displacement);
            }
        }
    }
}

checking::data_def_field<int32_t> set_data_def_field(
    const expressions::mach_expression* e, context::dependency_solver& info)
{
    using namespace checking;
    data_def_field<int32_t> field;
    // if the expression cannot be evaluated, we return field as if it was not there
    field.present = e != nullptr && !e->get_dependencies(info).contains_dependencies();
    if (field.present)
    {
        field.rng = e->get_range();
        // TODO get_reloc get_abs
        auto ret(e->evaluate(info));
        if (ret.value_kind() == context::symbol_value_kind::ABS)
            field.value = e->evaluate(info).get_abs();
    }
    return field;
}

checking::dupl_factor_modifier_t data_definition::evaluate_dupl_factor(context::dependency_solver& info) const
{
    return dupl_factor ? set_data_def_field(dupl_factor.get(), info) : checking::data_def_field<int32_t>(1);
}

checking::data_def_length_t data_definition::evaluate_length(context::dependency_solver& info) const
{
    checking::data_def_length_t len(set_data_def_field(length.get(), info));
    len.len_type = length_type == expressions::data_definition::length_type::BIT ? checking::data_def_length_t::BIT
                                                                                 : checking::data_def_length_t::BYTE;
    return len;
}

checking::scale_modifier_t data_definition::evaluate_scale(context::dependency_solver& info) const
{
    auto common = set_data_def_field(scale.get(), info);
    return checking::scale_modifier_t(common.present, (int16_t)common.value, common.rng);
}

checking::exponent_modifier_t data_definition::evaluate_exponent(context::dependency_solver& info) const
{
    return set_data_def_field(exponent.get(), info);
}

inline checking::nominal_value_expressions extract_nominal_value_expressions(
    const expr_or_address_list& exprs, context::dependency_solver& info)
{
    checking::nominal_value_expressions values;
    for (const auto& e_or_a : exprs)
    {
        if (std::holds_alternative<expressions::mach_expr_ptr>(e_or_a))
        {
            const expressions::mach_expr_ptr& e = std::get<expressions::mach_expr_ptr>(e_or_a);
            auto deps = e->get_dependencies(info);
            bool ignored = deps.has_error || deps.contains_dependencies(); // ignore values with dependencies
            auto ev = e->evaluate(info);
            auto kind = ev.value_kind();
            if (kind == context::symbol_value_kind::ABS)
            {
                values.push_back(checking::data_def_expr {
                    ev.get_abs(),
                    checking::expr_type::ABS,
                    e->get_range(),
                    ignored,
                });
            }
            else if (kind == context::symbol_value_kind::RELOC)
            {
                // TO DO value of the relocatable expression
                // maybe push back data_def_addr?
                values.push_back(checking::data_def_expr {
                    0,
                    ev.get_reloc().is_complex() ? checking::expr_type::COMPLEX : checking::expr_type::RELOC,
                    e->get_range(),
                    ignored,
                });
            }
            else if (kind == context::symbol_value_kind::UNDEF)
            {
                values.push_back(checking::data_def_expr { 0, checking::expr_type::ABS, e->get_range(), true });
            }
            else
            {
                assert(false);
                continue;
            }
        }
        else // there is an address D(B)
        {
            const auto& a = std::get<expressions::address_nominal>(e_or_a);
            checking::data_def_address ch_adr;

            ch_adr.base = set_data_def_field(a.base.get(), info);
            ch_adr.displacement = set_data_def_field(a.displacement.get(), info);
            ch_adr.ignored = !ch_adr.base.present || !ch_adr.displacement.present; // ignore value with dependencies
            values.push_back(ch_adr);
        }
    }
    return values;
}

checking::nominal_value_t data_definition::evaluate_nominal_value(context::dependency_solver& info) const
{
    if (!nominal_value)
        return {};

    checking::nominal_value_t nom;
    nom.present = true;
    if (nominal_value->access_string())
    {
        nom.value = nominal_value->access_string()->value;
        nom.rng = nominal_value->access_string()->value_range;
    }
    else if (nominal_value->access_exprs())
    {
        nom.value = extract_nominal_value_expressions(nominal_value->access_exprs()->exprs, info);
    }
    else
        assert(false);

    return nom;
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

    if (nominal_value && nominal_value->access_exprs())
    {
        for (const auto& val : nominal_value->access_exprs()->exprs)
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

size_t hlasm_plugin::parser_library::expressions::data_definition::hash() const
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

data_definition::parser::parser(
    semantics::collector& coll, std::string format, mach_expr_list exprs, nominal_value_ptr nominal, position begin)
    : collector_(coll)
    , format_(std::move(format))
    , exprs_(std::move(exprs))
    , nominal_(std::move(nominal))
    , pos_(begin)
    , p_(0)
    , exprs_i_(0)
{
    context::to_upper(format_);
}

constexpr unsigned char digit_to_value(unsigned char c)
{
    static_assert((unsigned char)'0' + 0 == '0');
    static_assert((unsigned char)'0' + 1 == '1');
    static_assert((unsigned char)'0' + 2 == '2');
    static_assert((unsigned char)'0' + 3 == '3');
    static_assert((unsigned char)'0' + 4 == '4');
    static_assert((unsigned char)'0' + 5 == '5');
    static_assert((unsigned char)'0' + 6 == '6');
    static_assert((unsigned char)'0' + 7 == '7');
    static_assert((unsigned char)'0' + 8 == '8');
    static_assert((unsigned char)'0' + 9 == '9');
    return c - '0';
}

// parses a number that begins on index begin in format_ string. Leaves index of the first character after the number in
// begin.
std::optional<int> data_definition::parser::parse_number()
{
    constexpr long long min_l = -(1LL << 31);
    constexpr long long max_l = (1LL << 31) - 1;
    constexpr long long parse_limit_l = (1LL << 31);
    static_assert(std::numeric_limits<int>::min() <= min_l);
    static_assert(std::numeric_limits<int>::max() >= max_l);

    const auto initial_p = p_;
    const position initial_pos = pos_;

    long long v = 0;
    const bool negative = format_[p_] == '-';
    bool parsed_one = false;
    if (format_[p_] == '+' || negative)
        ++p_;
    while (p_ < format_.size())
    {
        unsigned char c = format_[p_];
        if (!isdigit(c))
            break;
        parsed_one = true;
        ++p_;

        if (v > parse_limit_l)
            continue;

        v = v * 10 + digit_to_value(c);
    }
    pos_.column += p_ - initial_p;
    if (!parsed_one)
    {
        result_.add_diagnostic(diagnostic_op::error_D002({ initial_pos, pos_ }));
        return std::nullopt;
    }
    if (negative)
        v = -v;
    if (v < min_l || v > max_l)
    {
        result_.add_diagnostic(diagnostic_op::error_D001({ initial_pos, pos_ }));
        return std::nullopt;
    }
    collector_.add_hl_symbol(token_info(range(initial_pos, pos_), semantics::hl_scopes::number));

    return (int)v;
}

void data_definition::parser::update_position(const mach_expression& e)
{
    pos_ = e.get_range().end;
    pos_.column += 1;
}
void data_definition::parser::update_position_by_one() { ++pos_.column; }

void data_definition::parser::parse_duplication_factor()
{
    if (isdigit(static_cast<unsigned char>(format_[0])) || format_[0] == '-') // duplication factor is present
    {
        position old_pos = pos_;
        auto dupl_factor_num = parse_number();

        if (dupl_factor_num)
            result_.dupl_factor = std::make_unique<mach_expr_constant>(*dupl_factor_num, range(old_pos, pos_));
    }
    else if (format_[0] == *data_definition::expr_placeholder)
    { // duplication factor as expression
        result_.dupl_factor = move_next_expression();
    }
}

bool is_type_extension(char type, char ch)
{
    return checking::data_def_type::types_and_extensions.count(std::make_pair(type, ch)) > 0;
}

bool is_modifier_or_prog(char ch) { return ch == 'P' || ch == 'L' || ch == 'S' || ch == 'E'; }

mach_expr_ptr data_definition::parser::move_next_expression()
{
    assert(exprs_i_ < exprs_.size());
    auto res = std::move(exprs_[exprs_i_]);
    ++exprs_i_;
    update_position(*res);
    ++p_;

    return res;
}

mach_expr_ptr data_definition::parser::parse_modifier_num_or_expr()
{
    if (auto c = format_[p_]; isdigit(c) || c == '-' || c == '+')
    {
        position old_pos = pos_;
        auto modifier = parse_number();

        if (modifier)
            return std::make_unique<mach_expr_constant>(*modifier, range(old_pos, pos_));
        // else leave expr null
    }
    else if (format_[p_] == expr_placeholder[0])
    {
        return move_next_expression();
    }
    else if (format_[p_] == nominal_placeholder[0])
    {
        auto exprs = nominal_->access_exprs();
        if (exprs && exprs->exprs.size() == 1 && std::holds_alternative<mach_expr_ptr>(exprs->exprs[0]))
        {
            // it is possible that a modifier has been parsed as nominal value,
            // if nominal value is not present at all and duplication factor is 0 or with DS instruction
            mach_expr_ptr& modifier = std::get<mach_expr_ptr>(exprs->exprs[0]);
            ++p_;
            update_position(*modifier);
            nominal_parsed_ = true;
            return std::move(modifier);
        }
        else
        {
            result_.add_diagnostic(diagnostic_op::error_D003({ pos_, { pos_.line, pos_.column + 1 } }));
            ++p_;
            // no need to update pos_, nominal value (if present) is the last character of the format string
        }
    }
    return nullptr;
}

void data_definition::parser::assign_expr_to_modifier(char modifier, mach_expr_ptr expr)
{
    switch (modifier)
    {
        case 'P':
            result_.program_type = std::move(expr);
            break;
        case 'L':
            result_.length = std::move(expr);
            break;
        case 'S':
            result_.scale = std::move(expr);
            break;
        case 'E':
            result_.exponent = std::move(expr);
            break;
        default:
            assert(false);
    }
}



void data_definition::parser::parse_modifier()
{
    // we assume, that format_[p_] determines one of modifiers PLSE
    position begin_pos = pos_;
    char modifier = format_[p_++];
    collector_.add_hl_symbol(token_info(
        range { begin_pos, { begin_pos.line, begin_pos.column + 1 } }, semantics::hl_scopes::data_def_modifier));
    if (p_ >= format_.size())
    {
        // expected something after modifier character
        result_.add_diagnostic(diagnostic_op::error_D003({ begin_pos, { begin_pos.line, begin_pos.column + 1 } }));
        return;
    }

    update_position_by_one();

    // parse bit length
    if (format_[p_] == '.')
    {
        ++p_;

        if (modifier == 'L')
            result_.length_type = length_type::BIT;
        else
            result_.add_diagnostic(diagnostic_op::error_D005({ begin_pos, { begin_pos.line, begin_pos.column + 1 } }));

        update_position_by_one();

        if (p_ >= format_.size())
        {
            // expected something after modifier character
            result_.add_diagnostic(diagnostic_op::error_D003({ begin_pos, { begin_pos.line, begin_pos.column + 1 } }));
            return;
        }
    }
    mach_expr_ptr expr = parse_modifier_num_or_expr();

    size_t rem_pos = remaining_modifiers_.find(modifier);
    if (rem_pos == std::string::npos) // wrong order
        result_.add_diagnostic(diagnostic_op::error_D004({ begin_pos, pos_ }));
    else
        remaining_modifiers_ = remaining_modifiers_.substr(rem_pos + 1);

    assign_expr_to_modifier(modifier, std::move(expr));
}

namespace {
struct loctr_reference_visitor final : public mach_expr_visitor
{
    bool found_loctr_reference = false;

    void visit(const mach_expr_constant&) override {}
    void visit(const mach_expr_data_attr&) override {}
    void visit(const mach_expr_symbol&) override {}
    void visit(const mach_expr_location_counter&) override { found_loctr_reference = true; }
    void visit(const mach_expr_self_def&) override {}
    void visit(const mach_expr_default&) override {}
    void visit(const mach_expr_literal& expr) override { expr.get_data_definition().apply(*this); }
};
} // namespace

data_definition data_definition::parser::parse()
{
    parse_duplication_factor();

    result_.type = format_[p_++];
    result_.type_range = { pos_, { pos_.line, pos_.column + 1 } };
    range unified_type_range = result_.type_range;
    update_position_by_one();


    if (p_ >= format_.size())
        return std::move(result_);

    if (is_type_extension(result_.type, format_[p_]))
    {
        result_.extension = format_[p_];
        result_.extension_range = { pos_, { pos_.line, pos_.column + 1 } };
        unified_type_range.end = result_.extension_range.end;
        ++p_;
        update_position_by_one();
    }

    collector_.add_hl_symbol(token_info(unified_type_range, semantics::hl_scopes::data_def_type));

    for (size_t i = 0; i < 5 && p_ < format_.size(); ++i)
    {
        if (is_modifier_or_prog(format_[p_]))
        {
            parse_modifier();
        }
        else if (format_[p_] == nominal_placeholder[0])
        {
            result_.nominal_value = std::move(nominal_);
            ++p_;
            break;
        }
        else
        {
            // consume all invalid characters and produce one diagnostic regarding all of them
            auto begin_pos = pos_;
            while (p_ < format_.size() && !is_modifier_or_prog(format_[p_]) && format_[p_] != nominal_placeholder[0])
            {
                ++p_;
                update_position_by_one();
            }

            result_.add_diagnostic(diagnostic_op::error_D006({ begin_pos, pos_ }));
        }
    }

    loctr_reference_visitor v;
    result_.apply(v);
    result_.references_loctr = v.found_loctr_reference;

    return std::move(result_);
}

bool hlasm_plugin::parser_library::expressions::is_similar(const data_definition& l, const data_definition& r) noexcept
{
    return hlasm_plugin::utils::is_similar(l,
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
