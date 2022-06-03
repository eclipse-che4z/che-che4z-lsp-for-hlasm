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
#include "checking/diagnostic_collector.h"
#include "context/using.h"
#include "ebcdic_encoding.h"
#include "lexing/logical_line.h"
#include "mach_expr_term.h"
#include "mach_expr_visitor.h"
#include "semantics/collector.h"
#include "utils/similar.h"

namespace hlasm_plugin::parser_library::expressions {

constexpr char V_type = 'V';

context::dependency_collector data_definition::get_dependencies(context::dependency_solver& solver) const
{
    context::dependency_collector deps = get_length_dependencies(solver);

    if (scale)
        deps.merge(scale->get_dependencies(solver));
    if (exponent)
        deps.merge(exponent->get_dependencies(solver));

    // In V type, the symbols are external, it is not defined in current program and does not
    // have any dependencies.
    if (type != V_type && nominal_value)
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

int32_t data_definition::get_integer_attribute(context::dependency_solver& info, diagnostic_op_consumer& diags) const
{
    auto def_type = access_data_def_type();
    if (def_type)
        return def_type->get_integer_attribute(
            evaluate_length(info, diags), evaluate_scale(info, diags), evaluate_reduced_nominal_value());
    else
        return 0;
}

context::symbol_attributes data_definition::get_symbol_attributes(
    context::dependency_solver& solver, diagnostic_op_consumer& diags) const
{
    return context::symbol_attributes(context::symbol_origin::DAT,
        ebcdic_encoding::a2e[(unsigned char)get_type_attribute()],
        get_length_attribute(solver, diags),
        get_scale_attribute(solver, diags),
        get_integer_attribute(solver, diags));
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

checking::data_def_field<int32_t> set_data_def_field(
    const expressions::mach_expression* e, context::dependency_solver& info, diagnostic_op_consumer& diags)
{
    using namespace checking;
    data_def_field<int32_t> field;
    // if the expression cannot be evaluated, we return field as if it was not there
    if (e)
    {
        field.rng = e->get_range();
        field.present = !e->get_dependencies(info).contains_dependencies();
    }
    if (field.present)
    {
        // TODO get_reloc get_abs
        auto ret = e->evaluate(info, diags);

        if (ret.value_kind() == context::symbol_value_kind::ABS)
            field.value = ret.get_abs();
        else
            field.present = false;
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

inline checking::nominal_value_expressions extract_nominal_value_expressions(const expr_or_address_list& exprs,
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

checking::nominal_value_t data_definition::evaluate_nominal_value(
    context::dependency_solver& info, diagnostic_op_consumer& diags) const
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
        nom.value = extract_nominal_value_expressions(nominal_value->access_exprs()->exprs,
            info,
            diags,
            type != 'S' ? nominal_eval_subtype::none
                        : (extension == 'Y' ? nominal_eval_subtype::SY_type : nominal_eval_subtype::S_type));
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
    if (nominal_value->access_string())
    {
        nom.value = nominal_value->access_string()->value;
        nom.rng = nominal_value->access_string()->value_range;
    }
    else if (nominal_value->access_exprs())
    {
        nom.value = nominal_value->access_exprs()->exprs.size();
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

    if (checking_rules == checking::data_instr_type::DC)
    {
        if (!dd_type->check_length<checking::data_instr_type::DC>(len, drop_diags))
            return -1;
    }
    else if (checking_rules == checking::data_instr_type::DS)
    {
        if (!dd_type->check_length<checking::data_instr_type::DS>(len, drop_diags))
            return -1;
    }
    else
        assert(false);

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

namespace {
struct loctr_reference_visitor final : public mach_expr_visitor
{
    bool found_loctr_reference = false;

    void visit(const mach_expr_constant&) override {}
    void visit(const mach_expr_data_attr&) override {}
    void visit(const mach_expr_data_attr_literal&) override {}
    void visit(const mach_expr_symbol&) override {}
    void visit(const mach_expr_location_counter&) override { found_loctr_reference = true; }
    void visit(const mach_expr_default&) override {}
    void visit(const mach_expr_literal& expr) override { expr.get_data_definition().apply(*this); }
};

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

bool is_type_extension(char type, char ch)
{
    return checking::data_def_type::types_and_extensions.count(std::make_pair(type, ch)) > 0;
}

void move_range(range& r)
{
    // TODO: this is not a good way to handle this - parser should ideally provide the correct tokens
    if (r.start.column >= lexing::default_ictl.end)
    {
        r.start.line += 1;
        r.start.column = lexing::default_ictl.continuation;
    }
    else
        r.start.column += 1;
}

void remove_char(std::string_view& v, range& r)
{
    v.remove_prefix(1);
    move_range(r);
}
} // namespace

std::optional<std::pair<int, range>> data_definition_parser::parse_number(std::string_view& v, range& r)
{
    if (v.empty())
        return std::nullopt;

    constexpr long long min_l = -(1LL << 31);
    constexpr long long max_l = (1LL << 31) - 1;
    constexpr long long parse_limit_l = (1LL << 31);
    static_assert(std::numeric_limits<int>::min() <= min_l);
    static_assert(std::numeric_limits<int>::max() >= max_l);

    const auto move_by_one = [&v, &r]() {
        assert(!v.empty());
        remove_char(v, r);
    };

    long long result = 0;
    const bool negative = [&]() {
        switch (v.front())
        {
            case '-':
                move_by_one();
                return true;
            case '+':
                move_by_one();
                return false;
            default:
                return false;
        }
    }();

    auto range_start = r;

    bool parsed_one = false;
    while (!v.empty())
    {
        unsigned char c = v.front();
        if (!isdigit(c))
            break;
        parsed_one = true;

        move_by_one();

        if (result > parse_limit_l)
            continue;

        result = result * 10 + digit_to_value(c);
    }
    if (!parsed_one)
    {
        m_errors.push_back(diagnostic_op::error_D002({ range_start.start, r.start }));
        return std::nullopt;
    }
    if (negative)
        result = -result;
    if (result < min_l || result > max_l)
    {
        m_errors.push_back(diagnostic_op::error_D001({ range_start.start, r.start }));
        return std::nullopt;
    }
    m_collector->add_hl_symbol(token_info(range(range_start.start, r.start), semantics::hl_scopes::number));

    return std::make_pair((int)result, range(range_start.start, r.start));
}

static bool can_have_exponent(char type)
{
    switch (type)
    {
        case 'D':
        case 'E':
        case 'F':
        case 'H':
        case 'L':
            return true;
        default:
            return false;
    }
}

mach_expr_ptr data_definition_parser::read_number(push_arg& v, range& r)
{
    if (std::holds_alternative<mach_expr_ptr>(v))
        return std::get<mach_expr_ptr>(std::move(v));
    else
        return read_number(std::get<std::string_view>(v), r);
}

mach_expr_ptr data_definition_parser::read_number(std::string_view& v, range& r)
{
    auto parse_result = parse_number(v, r);
    if (!parse_result.has_value())
        return {};
    const auto& [num, rng] = parse_result.value();
    return std::make_unique<mach_expr_constant>(num, rng);
}

namespace {
struct
{
    bool operator()(const std::string_view& s) const { return s.empty(); }
    bool operator()(const mach_expr_ptr& p) const { return !p; }
} constexpr argument_empty;
struct
{
    std::optional<char> operator()(const std::string_view& s) const
    {
        if (s.empty())
            return std::nullopt;
        else
            return s.front();
    }
    std::optional<char> operator()(const mach_expr_ptr&) const { return std::nullopt; }
} constexpr first_letter;
struct
{
    std::optional<char> operator()(const std::string_view& s) const
    {
        if (s.empty())
            return std::nullopt;
        else
            return (char)toupper((unsigned char)s.front());
    }
    std::optional<char> operator()(const mach_expr_ptr&) const { return std::nullopt; }
} constexpr first_letter_upper;
struct
{
    std::pair<bool, bool> operator()(const std::string_view& s) const
    {
        if (s.empty())
            return { false, false };
        else
            return { isdigit((unsigned char)s.front()), false };
    }
    std::pair<bool, bool> operator()(const mach_expr_ptr&) const { return { true, true }; }
} constexpr is_dupl_factor;
} // namespace

void data_definition_parser::push(push_arg v, range r)
{
    if (std::visit(argument_empty, v))
    {
        m_state = {};
        return;
    }

    const auto move_by_one = [&v, &r]() {
        assert(std::holds_alternative<std::string_view>(v));
        auto& text = std::get<std::string_view>(v);
        assert(!text.empty());
        auto ret = r;
        remove_char(text, r);
        ret.end = r.start;

        return ret;
    };

    while (!std::visit(argument_empty, v))
    {
        switch (m_state.parsing_state)
        {
            case state::done:
                return;

            case state::duplicating_factor: {
                auto [has_dup_factor, is_expr] = std::visit(is_dupl_factor, v);
                if (has_dup_factor)
                    m_result.dupl_factor = read_number(v, r);
                m_state = { state::read_type, { false, is_expr, false, false }, r.end };
                break;
            }

            case state::read_type: {
                m_result.type = std::visit(first_letter_upper, v).value();
                m_result.type_range = move_by_one();
                auto type_range = m_result.type_range;

                if (auto t_e = std::visit(first_letter_upper, v);
                    t_e.has_value() && is_type_extension(m_result.type, t_e.value()))
                {
                    m_result.extension = t_e.value();

                    m_result.extension_range = move_by_one();

                    type_range.end = r.start;
                }
                m_collector->add_hl_symbol(token_info(type_range, semantics::hl_scopes::data_def_type));
                m_state = { state::try_reading_program, { false, false, false, false }, std::nullopt };
                break;
            }

            case state::try_reading_program:
                if (auto c = std::visit(first_letter, v); c != 'P' && c != 'p')
                {
                    m_state.parsing_state = state::try_reading_length;
                    break;
                }
                m_collector->add_hl_symbol(token_info(move_by_one(), semantics::hl_scopes::data_def_modifier));
                m_state = { state::read_program, { true, false, true, false }, r.start };
                break;

            case state::read_program:
                if (!(m_result.program_type = read_number(v, r)))
                {
                    m_state = {};
                    return;
                }
                m_state = { state::try_reading_length, { false, true, false, false }, std::nullopt };
                break;

            case state::try_reading_length:
                if (auto c = std::visit(first_letter, v); c != 'L' && c != 'l')
                {
                    m_state.parsing_state = state::try_reading_scale;
                    break;
                }
                m_collector->add_hl_symbol(token_info(move_by_one(), semantics::hl_scopes::data_def_modifier));
                m_state = { state::try_reading_bitfield, { true, false, false, true }, r.start };
                break;

            case state::try_reading_bitfield: {
                auto bit_field = std::visit(first_letter, v) == '.';
                if (bit_field)
                {
                    m_result.length_type = data_definition::length_type::BIT;
                    move_by_one();
                }
                m_state = { state::read_length, { true, bit_field, false, false }, r.start };
                break;
            }

            case state::read_length:
                if (!(m_result.length = read_number(v, r)))
                {
                    m_state = {};
                    return;
                }
                m_state = { state::try_reading_scale, { false, true, false, false }, std::nullopt };
                break;

            case state::try_reading_scale:
                if (auto c = std::visit(first_letter, v); c != 'S' && c != 's')
                {
                    m_state.parsing_state = state::try_reading_exponent;
                    break;
                }
                m_collector->add_hl_symbol(token_info(move_by_one(), semantics::hl_scopes::data_def_modifier));
                m_state = { state::read_scale, { true, false, true, false }, r.start };
                break;

            case state::read_scale:
                if (!(m_result.scale = read_number(v, r)))
                {
                    m_state = {};
                    return;
                }
                m_state = { state::try_reading_exponent, { false, true, false, false }, std::nullopt };
                break;

            case state::try_reading_exponent:
                if (auto c = std::visit(first_letter, v); c != 'E' && c != 'e' || !can_have_exponent(m_result.type))
                {
                    m_state.parsing_state = state::too_much_text;
                    break;
                }
                m_collector->add_hl_symbol(token_info(move_by_one(), semantics::hl_scopes::data_def_modifier));
                m_state = { state::read_exponent, { true, false, true, false }, r.start };
                break;

            case state::read_exponent:
                if (!(m_result.exponent = read_number(v, r)))
                {
                    m_state = {};
                    return;
                }
                m_state = { state::too_much_text, {}, std::nullopt };
                break;

            case state::too_much_text:
                m_state = {};
                m_errors.push_back(diagnostic_op::error_D006(r));
                return;

            default:
                assert(false);
        }
    }
}
void data_definition_parser::push(nominal_value_ptr n) { m_result.nominal_value = std::move(n); }

std::pair<data_definition, std::vector<diagnostic_op>> data_definition_parser::take_result()
{
    if (m_state.expecting_next.has_value())
        m_errors.push_back(diagnostic_op::error_D003(range(m_state.expecting_next.value())));

    loctr_reference_visitor v;
    m_result.apply(v);
    m_result.references_loctr = v.found_loctr_reference;

    return std::make_pair(std::move(m_result), std::move(m_errors));
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
