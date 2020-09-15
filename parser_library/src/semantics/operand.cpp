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

#include "operand.h"

#include "expressions/conditional_assembly/terms/ca_var_sym.h"

namespace hlasm_plugin::parser_library::semantics {

//***************** operand *********************

operand::operand(const operand_type type, range operand_range)
    : type(type)
    , operand_range(std::move(operand_range))
{}

model_operand* operand::access_model()
{
    return type == operand_type::MODEL ? static_cast<model_operand*>(this) : nullptr;
}

ca_operand* operand::access_ca() { return type == operand_type::CA ? static_cast<ca_operand*>(this) : nullptr; }

macro_operand* operand::access_mac() { return type == operand_type::MAC ? static_cast<macro_operand*>(this) : nullptr; }

data_def_operand* operand::access_data_def()
{
    return type == operand_type::DAT ? static_cast<data_def_operand*>(this) : nullptr;
}

machine_operand* operand::access_mach() { return dynamic_cast<machine_operand*>(this); }

assembler_operand* operand::access_asm() { return dynamic_cast<assembler_operand*>(this); }

empty_operand::empty_operand(range operand_range)
    : operand(operand_type::EMPTY, std::move(operand_range))
{}

model_operand::model_operand(concat_chain chain, range operand_range)
    : operand(operand_type::MODEL, std::move(operand_range))
    , chain(std::move(chain))
{
    concatenation_point::clear_concat_chain(this->chain);
}

evaluable_operand::evaluable_operand(const operand_type type, range operand_range)
    : operand(type, std::move(operand_range))
{}

machine_operand::machine_operand(const mach_kind kind)
    : kind(kind)
{}

expr_machine_operand* machine_operand::access_expr()
{
    return kind == mach_kind::EXPR ? static_cast<expr_machine_operand*>(this) : nullptr;
}

address_machine_operand* machine_operand::access_address()
{
    return kind == mach_kind::ADDR ? static_cast<address_machine_operand*>(this) : nullptr;
}

std::unique_ptr<checking::operand> make_check_operand(expressions::mach_evaluate_info info,
    expressions::mach_expression& expr,
    std::optional<checking::machine_operand_type> type_hint = std::nullopt)
{
    auto res = expr.evaluate(info);
    if (res.value_kind() == context::symbol_value_kind::ABS)
    {
        return std::make_unique<checking::one_operand>(res.get_abs());
    }
    else
    {
        if (type_hint && *type_hint == checking::machine_operand_type::REG_IMM)
        {
            return std::make_unique<checking::one_operand>(0);
        }
        else
        {
            return std::make_unique<checking::address_operand>(
                checking::address_state::UNRES, 0, 0, 0, checking::operand_state::ONE_OP);
        }
    }
}

//***************** expr_machine_operand *********************

expr_machine_operand::expr_machine_operand(expressions::mach_expr_ptr expression, range operand_range)
    : evaluable_operand(operand_type::MACH, std::move(operand_range))
    , machine_operand(mach_kind::EXPR)
    , simple_expr_operand(std::move(expression))
{}

std::unique_ptr<checking::operand> expr_machine_operand::get_operand_value(expressions::mach_evaluate_info info) const
{
    return make_check_operand(info, *expression);
}

std::unique_ptr<checking::operand> expr_machine_operand::get_operand_value(
    expressions::mach_evaluate_info info, checking::machine_operand_type type_hint) const
{
    return make_check_operand(info, *expression, type_hint);
}

// suppress MSVC warning 'inherits via dominance'
bool expr_machine_operand::has_dependencies(expressions::mach_evaluate_info info) const
{
    return simple_expr_operand::has_dependencies(info);
}

// suppress MSVC warning 'inherits via dominance'
bool expr_machine_operand::has_error(expressions::mach_evaluate_info info) const
{
    return simple_expr_operand::has_error(info);
}

// suppress MSVC warning 'inherits via dominance'
std::vector<const context::resolvable*> expr_machine_operand::get_resolvables() const
{
    return simple_expr_operand::get_resolvables();
}

void expr_machine_operand::collect_diags() const { collect_diags_from_child(*expression); }

//***************** address_machine_operand *********************

address_machine_operand::address_machine_operand(expressions::mach_expr_ptr displacement,
    expressions::mach_expr_ptr first_par,
    expressions::mach_expr_ptr second_par,
    range operand_range,
    checking::operand_state state)
    : evaluable_operand(operand_type::MACH, std::move(operand_range))
    , machine_operand(mach_kind::ADDR)
    , displacement(std::move(displacement))
    , first_par(std::move(first_par))
    , second_par(std::move(second_par))
    , state(std::move(state))
{}

bool address_machine_operand::has_dependencies(expressions::mach_evaluate_info info) const
{
    if (first_par)
    {
        if (second_par)
            return displacement->get_dependencies(info).contains_dependencies()
                || first_par->get_dependencies(info).contains_dependencies()
                || second_par->get_dependencies(info).contains_dependencies(); // D(B1,B2)
        else
            return displacement->get_dependencies(info).contains_dependencies()
                || first_par->get_dependencies(info).contains_dependencies(); // D(B)
    }
    else
        return displacement->get_dependencies(info).contains_dependencies()
            || second_par->get_dependencies(info).contains_dependencies(); // D(,B)
}

bool address_machine_operand::has_error(expressions::mach_evaluate_info info) const
{
    if (first_par)
    {
        if (second_par)
            return displacement->get_dependencies(info).has_error || first_par->get_dependencies(info).has_error
                || second_par->get_dependencies(info).has_error; // D(B1,B2)
        else
            return displacement->get_dependencies(info).has_error
                || first_par->get_dependencies(info).has_error; // D(B)
    }
    else
        return displacement->get_dependencies(info).has_error || second_par->get_dependencies(info).has_error; // D(,B)
}

std::vector<const context::resolvable*> address_machine_operand::get_resolvables() const
{
    std::vector<const context::resolvable*> res;

    res.push_back(&*displacement);
    if (first_par)
        res.push_back(&*first_par);
    if (second_par)
        res.push_back(&*second_par);

    return res;
}

std::unique_ptr<checking::operand> address_machine_operand::get_operand_value(
    expressions::mach_evaluate_info info) const
{
    context::symbol_value displ, first, second;
    context::symbol_value::abs_value_t displ_v, first_v, second_v;

    displ = displacement->evaluate(info);
    displ_v = displ.value_kind() == context::symbol_value_kind::ABS ? displ.get_abs() : 0;
    if (first_par)
    {
        first = first_par->evaluate(info);
        first_v = first.value_kind() == context::symbol_value_kind::ABS ? first.get_abs() : 0;
    }
    if (second_par)
    {
        second = second_par->evaluate(info);
        second_v = second.value_kind() == context::symbol_value_kind::ABS ? second.get_abs() : 0;
    }

    if (first_par && second_par) // both defined
        return std::make_unique<checking::address_operand>(
            checking::address_state::UNRES, displ_v, first_v, second_v); // D(B1,B2)
    if (first_par) // only first defined
        return std::make_unique<checking::address_operand>(
            checking::address_state::UNRES, displ_v, first_v, 0, state); // D(B1,)
    if (second_par) // only second defined
        return std::make_unique<checking::address_operand>(
            checking::address_state::UNRES, displ_v, 0, second_v, state); // D(B) or D(,B)
    return std::make_unique<checking::address_operand>(checking::address_state::UNRES, displ_v, 0, 0, state); // D(,)
}

std::unique_ptr<checking::operand> address_machine_operand::get_operand_value(
    expressions::mach_evaluate_info info, checking::machine_operand_type) const
{
    return get_operand_value(info);
}

void address_machine_operand::collect_diags() const
{
    collect_diags_from_child(*displacement);
    if (first_par)
        collect_diags_from_child(*first_par);
    if (second_par)
        collect_diags_from_child(*second_par);
}

assembler_operand::assembler_operand(const asm_kind kind)
    : kind(kind)
{}

expr_assembler_operand* assembler_operand::access_expr()
{
    return kind == asm_kind::EXPR ? static_cast<expr_assembler_operand*>(this) : nullptr;
}

using_instr_assembler_operand* assembler_operand::access_base_end()
{
    return kind == asm_kind::BASE_END ? static_cast<using_instr_assembler_operand*>(this) : nullptr;
}

complex_assembler_operand* assembler_operand::access_complex()
{
    return kind == asm_kind::COMPLEX ? static_cast<complex_assembler_operand*>(this) : nullptr;
}

string_assembler_operand* assembler_operand::access_string()
{
    return kind == asm_kind::STRING ? static_cast<string_assembler_operand*>(this) : nullptr;
}

//***************** expr_assembler_operand *********************

expr_assembler_operand::expr_assembler_operand(
    expressions::mach_expr_ptr expression, std::string string_value, range operand_range)
    : evaluable_operand(operand_type::ASM, std::move(operand_range))
    , assembler_operand(asm_kind::EXPR)
    , simple_expr_operand(std::move(expression))
    , value_(std::move(string_value))
{}

std::unique_ptr<checking::operand> expr_assembler_operand::get_operand_value(expressions::mach_evaluate_info info) const
{
    auto res = expression->evaluate(info);
    switch (res.value_kind())
    {
        case context::symbol_value_kind::UNDEF:
            return std::make_unique<checking::one_operand>(value_);
        case context::symbol_value_kind::ABS:
            return std::make_unique<checking::one_operand>(value_, res.get_abs());
        case context::symbol_value_kind::RELOC:
            return std::make_unique<checking::one_operand>("0", 0);
        default:
            assert(false);
            return std::make_unique<checking::empty_operand>();
    }
}

// suppress MSVC warning 'inherits via dominance'
bool expr_assembler_operand::has_dependencies(expressions::mach_evaluate_info info) const
{
    return simple_expr_operand::has_dependencies(info);
}

// suppress MSVC warning 'inherits via dominance'
bool expr_assembler_operand::has_error(expressions::mach_evaluate_info info) const
{
    return simple_expr_operand::has_error(info);
}

// suppress MSVC warning 'inherits via dominance'
std::vector<const context::resolvable*> expr_assembler_operand::get_resolvables() const
{
    return simple_expr_operand::get_resolvables();
}

void expr_assembler_operand::collect_diags() const { collect_diags_from_child(*expression); }

//***************** end_instr_machine_operand *********************

using_instr_assembler_operand::using_instr_assembler_operand(
    expressions::mach_expr_ptr base, expressions::mach_expr_ptr end, range operand_range)
    : evaluable_operand(operand_type::ASM, std::move(operand_range))
    , assembler_operand(asm_kind::BASE_END)
    , base(std::move(base))
    , end(std::move(end))
{}

bool using_instr_assembler_operand::has_dependencies(expressions::mach_evaluate_info info) const
{
    return base->get_dependencies(info).contains_dependencies() || end->get_dependencies(info).contains_dependencies();
}

bool using_instr_assembler_operand::has_error(expressions::mach_evaluate_info info) const
{
    return base->get_dependencies(info).has_error || end->get_dependencies(info).has_error;
}

std::vector<const context::resolvable*> using_instr_assembler_operand::get_resolvables() const
{
    return { &*base, &*end };
}

std::unique_ptr<checking::operand> using_instr_assembler_operand::get_operand_value(
    expressions::mach_evaluate_info info) const
{
    (void)info;
    std::vector<std::unique_ptr<checking::asm_operand>> pair;
    // pair.push_back(make_check_operand(info, *base));
    // pair.push_back(make_check_operand(info, *end));
    return std::make_unique<checking::complex_operand>("", std::move(pair));
}

void using_instr_assembler_operand::collect_diags() const
{
    collect_diags_from_child(*base);
    collect_diags_from_child(*end);
}

//***************** complex_assempler_operand *********************
complex_assembler_operand::complex_assembler_operand(
    std::string identifier, std::vector<std::unique_ptr<component_value_t>> values, range operand_range)
    : evaluable_operand(operand_type::ASM, std::move(operand_range))
    , assembler_operand(asm_kind::COMPLEX)
    , value(identifier, std::move(values), operand_range)
{}

bool complex_assembler_operand::has_dependencies(expressions::mach_evaluate_info) const { return false; }

bool complex_assembler_operand::has_error(expressions::mach_evaluate_info) const { return false; }

std::vector<const context::resolvable*> complex_assembler_operand::get_resolvables() const
{
    return std::vector<const context::resolvable*>();
}

std::unique_ptr<checking::operand> complex_assembler_operand::get_operand_value(expressions::mach_evaluate_info) const
{
    return value.create_operand();
}

void complex_assembler_operand::collect_diags() const {}

//***************** ca_operand *********************
ca_operand::ca_operand(const ca_kind kind, range operand_range)
    : operand(operand_type::CA, std::move(operand_range))
    , kind(kind)
{}

var_ca_operand* ca_operand::access_var() { return kind == ca_kind::VAR ? static_cast<var_ca_operand*>(this) : nullptr; }

const var_ca_operand* ca_operand::access_var() const
{
    return kind == ca_kind::VAR ? static_cast<const var_ca_operand*>(this) : nullptr;
}

expr_ca_operand* ca_operand::access_expr()
{
    return kind == ca_kind::EXPR ? static_cast<expr_ca_operand*>(this) : nullptr;
}

const expr_ca_operand* ca_operand::access_expr() const
{
    return kind == ca_kind::EXPR ? static_cast<const expr_ca_operand*>(this) : nullptr;
}

seq_ca_operand* ca_operand::access_seq() { return kind == ca_kind::SEQ ? static_cast<seq_ca_operand*>(this) : nullptr; }

const seq_ca_operand* ca_operand::access_seq() const
{
    return kind == ca_kind::SEQ ? static_cast<const seq_ca_operand*>(this) : nullptr;
}

branch_ca_operand* ca_operand::access_branch()
{
    return kind == ca_kind::BRANCH ? static_cast<branch_ca_operand*>(this) : nullptr;
}

const branch_ca_operand* ca_operand::access_branch() const
{
    return kind == ca_kind::BRANCH ? static_cast<const branch_ca_operand*>(this) : nullptr;
}

simple_expr_operand::simple_expr_operand(expressions::mach_expr_ptr expression)
    : expression(std::move(expression))
{}

bool simple_expr_operand::has_dependencies(expressions::mach_evaluate_info info) const
{
    return expression->get_dependencies(info).contains_dependencies();
}

bool simple_expr_operand::has_error(expressions::mach_evaluate_info info) const
{
    return expression->get_dependencies(info).has_error;
}

std::vector<const context::resolvable*> simple_expr_operand::get_resolvables() const { return { &*expression }; }

var_ca_operand::var_ca_operand(vs_ptr variable_symbol, range operand_range)
    : ca_operand(ca_kind::VAR, std::move(operand_range))
    , variable_symbol(std::move(variable_symbol))
{}

std::set<context::id_index> var_ca_operand::get_undefined_attributed_symbols(
    const expressions::evaluation_context& eval_ctx)
{
    return expressions::ca_var_sym::get_undefined_attributed_symbols_vs(variable_symbol, eval_ctx);
}

expr_ca_operand::expr_ca_operand(expressions::ca_expr_ptr expression, range operand_range)
    : ca_operand(ca_kind::EXPR, std::move(operand_range))
    , expression(std::move(expression))
{}

std::set<context::id_index> expr_ca_operand::get_undefined_attributed_symbols(
    const expressions::evaluation_context& eval_ctx)
{
    return expression->get_undefined_attributed_symbols(eval_ctx);
}

seq_ca_operand::seq_ca_operand(seq_sym sequence_symbol, range operand_range)
    : ca_operand(ca_kind::SEQ, std::move(operand_range))
    , sequence_symbol(std::move(sequence_symbol))
{}

std::set<context::id_index> seq_ca_operand::get_undefined_attributed_symbols(const expressions::evaluation_context&)
{
    return std::set<context::id_index>();
}

branch_ca_operand::branch_ca_operand(seq_sym sequence_symbol, expressions::ca_expr_ptr expression, range operand_range)
    : ca_operand(ca_kind::BRANCH, std::move(operand_range))
    , sequence_symbol(std::move(sequence_symbol))
    , expression(std::move(expression))
{}

std::set<context::id_index> branch_ca_operand::get_undefined_attributed_symbols(
    const expressions::evaluation_context& eval_ctx)
{
    return expression->get_undefined_attributed_symbols(eval_ctx);
}



macro_operand_chain::macro_operand_chain(concat_chain chain, range operand_range)
    : macro_operand(mac_kind::CHAIN, std::move(operand_range))
    , chain(std::move(chain))
{}



data_def_operand::data_def_operand(expressions::data_definition val, range operand_range)
    : evaluable_operand(operand_type::DAT, std::move(operand_range))
    , value(std::make_shared<expressions::data_definition>(std::move(val)))
{}


context::dependency_collector data_def_operand::get_length_dependencies(expressions::mach_evaluate_info info) const
{
    return value->get_length_dependencies(info);
}

context::dependency_collector data_def_operand::get_dependencies(expressions::mach_evaluate_info info) const
{
    return value->get_dependencies(info);
}

bool data_def_operand::has_dependencies(expressions::mach_evaluate_info info) const
{
    return value->get_dependencies(info).contains_dependencies();
}

bool data_def_operand::has_error(expressions::mach_evaluate_info info) const
{
    return value->get_dependencies(info).has_error;
}

template<typename... args>
std::vector<const context::resolvable*> resolvable_list(const args&... expr)
{
    std::vector<const context::resolvable*> list;
    (
        [&](auto&& x) {
            if (x)
                list.push_back(x.get());
        }(expr),
        ...);
    return list;
}

std::vector<const context::resolvable*> data_def_operand::get_resolvables() const
{
    std::vector<const context::resolvable*> res =
        resolvable_list(value->dupl_factor, value->length, value->exponent, value->scale);
    if (value->nominal_value)
    {
        auto exprs = value->nominal_value->access_exprs();
        if (exprs)
        {
            for (const auto& e : exprs->exprs)
            {
                if (std::holds_alternative<expressions::mach_expr_ptr>(e))
                    res.push_back(std::get<expressions::mach_expr_ptr>(e).get());
                else
                {
                    const expressions::address_nominal& addr = std::get<expressions::address_nominal>(e);
                    res.push_back(addr.base.get());
                    res.push_back(addr.displacement.get());
                }
            }
        }
    }

    return res;
}

std::unique_ptr<checking::operand> data_def_operand::get_operand_value(expressions::mach_evaluate_info info) const
{
    auto op = std::make_unique<checking::data_definition_operand>();

    op->dupl_factor = value->evaluate_dupl_factor(info);
    op->type.value = value->type;
    op->type.rng = value->type_range;
    op->extension.present = value->extension != '\0';
    op->extension.value = value->extension;
    op->extension.rng = value->extension_range;
    op->length = value->evaluate_length(info);
    op->scale = value->evaluate_scale(info);
    op->exponent = value->evaluate_exponent(info);

    op->nominal_value = value->evaluate_nominal_value(info);
    return op;
}

void data_def_operand::collect_diags() const { collect_diags_from_child(*value); }

string_assembler_operand::string_assembler_operand(std::string value, range operand_range)
    : evaluable_operand(operand_type::ASM, std::move(operand_range))
    , assembler_operand(asm_kind::STRING)
    , value(std::move(value))
{}

bool string_assembler_operand::has_dependencies(expressions::mach_evaluate_info) const { return false; }

bool string_assembler_operand::has_error(expressions::mach_evaluate_info) const { return false; }

std::vector<const context::resolvable*> string_assembler_operand::get_resolvables() const
{
    return std::vector<const context::resolvable*>();
}

std::unique_ptr<checking::operand> string_assembler_operand::get_operand_value(expressions::mach_evaluate_info) const
{
    return std::make_unique<checking::one_operand>("'" + value + "'");
}

void string_assembler_operand::collect_diags() const {}

macro_operand_string::macro_operand_string(std::string value, const range operand_range)
    : macro_operand(mac_kind::STRING, operand_range)
    , value(std::move(value))
{}

macro_operand_chain* macro_operand::access_chain()
{
    return kind == mac_kind::CHAIN ? static_cast<macro_operand_chain*>(this) : nullptr;
}

macro_operand_string* macro_operand::access_string()
{
    return kind == mac_kind::STRING ? static_cast<macro_operand_string*>(this) : nullptr;
}

macro_operand::macro_operand(mac_kind kind, range operand_range)
    : operand(operand_type::MAC, std::move(operand_range))
    , kind(kind)
{}

} // namespace hlasm_plugin::parser_library::semantics
